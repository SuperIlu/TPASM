//	Copyright (C) 1999-2012 Core Technologies.
//
//	This file is part of tpasm.
//
//	tpasm is free software; you can redistribute it and/or modify
//	it under the terms of the tpasm LICENSE AGREEMENT.
//
//	tpasm is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	tpasm LICENSE AGREEMENT for more details.
//
//	You should have received a copy of the tpasm LICENSE AGREEMENT
//	along with tpasm; see the file "LICENSE.TXT".

// Handle interface to all supported output file types
// NOTE: this has an interface similar to the processor support code.
// It allows the list of output file routines to be created at
// run time.

#include	"include.h"

static OUTPUTFILE_TYPE
	*topOutputFileType=NULL;		// list of symbol file types (created at run time)

static SYM_TABLE
	*outputFileTypeSymbols;			// symbol table of names of all supported generators

struct OUTPUT_RECORD
{
	OUTPUTFILE_TYPE
		*type;						// tells the type to output
	OUTPUT_RECORD
		*nextRecord;				// points to the next type of output file to generate
	char
		outputName[1];				// variable length string which contains the file name to output to
};

static OUTPUT_RECORD
	*firstOutputRecord,				// linked list of output records
	*lastOutputRecord;

bool DumpOutputFiles()
// dump output files for each type requested
// if there is a problem, complain and return false
{
	bool
		fail;
	OUTPUT_RECORD
		*currentRecord;
	FILE
		*file;

	fail=false;
	currentRecord=firstOutputRecord;
	while(currentRecord&&!fail)
	{
		if((file=currentRecord->type->binary?OpenBinaryOutputFile(&currentRecord->outputName[0]):OpenTextOutputFile(&currentRecord->outputName[0])))
		{
			fail=!currentRecord->type->outputFileGenerateFunction(file);
			currentRecord->type->binary?CloseBinaryOutputFile(file):CloseTextOutputFile(file);
		}
		else
		{
			fail=true;
		}
		currentRecord=currentRecord->nextRecord;
	}
	return(!fail);
}

bool SelectOutputFileType(char *typeName,char *outputName)
// select the given output file type for generation
// If the type does not exist, or there is some other problem, complain and
// return false
{
	OUTPUTFILE_TYPE
		*type;
	OUTPUT_RECORD
		*record;

	if((type=(OUTPUTFILE_TYPE *)STFindDataForName(outputFileTypeSymbols,typeName)))
	{
		if((record=(OUTPUT_RECORD *)NewPtr(sizeof(OUTPUT_RECORD)+strlen(outputName)+1)))	// create a record and link it to the list
		{
			record->type=type;
			record->nextRecord=NULL;
			strcpy(&record->outputName[0],outputName);
			if(lastOutputRecord)
			{
				lastOutputRecord->nextRecord=record;
				lastOutputRecord=record;
			}
			else
			{
				firstOutputRecord=lastOutputRecord=record;
			}
			return(true);
		}
		else
		{
			ReportComplaint(true,"Failed to allocate output file record\n");
		}
	}
	else
	{
		ReportComplaint(true,"Invalid output file type: %s\n",typeName);
	}
	return(false);
}

void UnInitOutputFileGenerate()
// undo what InitOutputFileGenerate did
{
	OUTPUT_RECORD
		*currentRecord,
		*nextRecord;

	currentRecord=firstOutputRecord;		// get rid of the list of files to generate
	while(currentRecord)
	{
		nextRecord=currentRecord->nextRecord;
		DisposePtr(currentRecord);
		currentRecord=nextRecord;
	}
	STDisposeSymbolTable(outputFileTypeSymbols);
}

bool InitOutputFileGenerate()
// initialize output file generation selection
{
	bool
		fail;
	OUTPUTFILE_TYPE
		*type;

	fail=false;
	firstOutputRecord=lastOutputRecord=NULL;		// no symbol files to create
	if((outputFileTypeSymbols=STNewSymbolTable(10)))
	{
		type=topOutputFileType;
		while(type&&!fail)
		{
			if((type->symbol=STAddEntryAtEnd(outputFileTypeSymbols,type->name,type)))
			{
				type=type->nextType;
			}
			else
			{
				ReportComplaint(true,"Failed to add output file type %s to list\n",type->name);
				fail=true;
			}
		}
		if(!fail)
		{
			return(true);
		}
		STDisposeSymbolTable(outputFileTypeSymbols);
	}
	else
	{
		ReportComplaint(true,"Failed to create symbol table for output file types\n");
	}
	return(false);
}

void DumpOutputFileTypeInformation(FILE *file)
// tell what output file types are supported by the assembler
{
	OUTPUTFILE_TYPE
		*type;

	type=topOutputFileType;
	while(type)
	{
		fprintf(file,"%-20s%s\n",type->name,type->description);
		type=type->nextType;
	}
}

// Constructors used to link output file generators to global list at run-time

OUTPUTFILE_TYPE::OUTPUTFILE_TYPE(const char *formatName,const char *formatDescription,bool isBinary,OutputFileGenerateFunction *generateFunction)
// Use this to add a given symbol file type
// to the global list
{
	name=formatName;
	description=formatDescription;
	binary=isBinary;
	outputFileGenerateFunction=generateFunction;

	if((nextType=topOutputFileType))				// link to next one
	{
		topOutputFileType->previousType=this;		// link next one to this one
	}
	previousType=NULL;								// none previous to this
	topOutputFileType=this;							// point at this one
}

OUTPUTFILE_TYPE::~OUTPUTFILE_TYPE()
// When a output type goes out of scope, get rid of it here
{
	if(nextType)
	{
		nextType->previousType=previousType;	// point next's previous to our previous
	}
	if(previousType)
	{
		previousType->nextType=nextType;		// point previous' next to our next
	}
	else
	{
		topOutputFileType=nextType;				// if no previous, then we were the top, so set top to our next
	}
}
