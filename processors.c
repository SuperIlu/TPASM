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


// Handle interface to all supported processors
// NOTE: this uses a little more of C++ than the rest of the code because
// it is very handy to be able to build the list of processors at run time
// based on which processor modules have been linked to the executable.
// Doing this keeps ALL code for a new processor local to the source file
// which supports it.

#include	"include.h"

static PROCESSOR_FAMILY
	*topProcessorFamily=NULL;		// list of processor families (created at run time)

static PROCESSOR
	*currentProcessor;

static SYM_TABLE
	*processorSymbols;				// symbol table of names of all supported processors

bool AttemptProcessorPseudoOpcode(const char *line,unsigned int *lineIndex,PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success)
// Try to find and handle processor specific pseudo-ops.
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	*success=false;
	if(currentProcessor)
	{
		return(currentProcessor->family->attemptPseudoOpcodeFunction(line,lineIndex,lineLabel,listingRecord,success));
	}
	return(true);
}

bool AttemptProcessorOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// Try to find and handle processor specific opcodes.
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	*success=false;
	if(currentProcessor)
	{
		return(currentProcessor->family->attemptOpcodeFunction(line,lineIndex,listingRecord,success));
	}
	return(true);
}

static void CreateProcessorLabelName(PROCESSOR *processor,char *labelName)
// create the name of the label used for the given processor
{
	char
		*name;

	sprintf(labelName,"__%s",processor->name);
	name=labelName;
	while(*name)
	{
		*name=toupper(*name);
		name++;
	}
}

static void UnAssignProcessorLabel(PROCESSOR *processor)
// get rid of a label for the passed processor
{
	char
		labelName[MAX_STRING];

	CreateProcessorLabelName(processor,labelName);
	UnAssignSetConstant(labelName);
}

static bool AssignProcessorLabel(PROCESSOR *processor)
// Create a label for the passed processor
{
	char
		labelName[MAX_STRING];

	CreateProcessorLabelName(processor,labelName);
	return(AssignSetConstant(labelName,1,true));
}

bool SelectProcessor(const char *processorName,bool *found)
// select the processor based on the passed name
// NOTE: if processorName is passed in as a zero length string,
// the current processor will be set to NULL
// NOTE: this creates an assembler set label of the processor name (in all uppercase)
// preceded by two underscores.
// NOTE: this only returns false on "hard" errors
{
	unsigned int
		i;
	char
		convertedName[MAX_STRING];
	void
		*resultValue;
	bool
		fail;

	fail=false;
	*found=false;
	if(currentProcessor)
	{
		UnAssignProcessorLabel(currentProcessor);
		currentProcessor->family->deselectProcessorFunction(currentProcessor);	// let this processor know it is being deselected
		currentProcessor=NULL;
	}
	if(!fail)
	{
		i=0;
		while(processorName[i])							// make name lower case
		{
			convertedName[i]=tolower(processorName[i]);
			i++;
		}
		convertedName[i]='\0';
		if(i)
		{
			if((resultValue=STFindDataForName(processorSymbols,convertedName)))
			{
				currentProcessor=(PROCESSOR *)resultValue;
				*found=true;
				if(AssignProcessorLabel(currentProcessor))
				{
					currentProcessor->family->selectProcessorFunction(currentProcessor);	// let this processor know it has been selected
				}
				else
				{
					fail=true;
				}
			}
		}
		else
		{
			*found=true;								// empty name can always be found
		}
	}
	return(!fail);
}

static void UnInitProcessorFamily(PROCESSOR_FAMILY *family)
// Uninitialize a processor family
{
	PROCESSOR
		*processor;

	processor=family->lastProcessor;
	while(processor)
	{
		STRemoveEntry(processorSymbols,processor->symbol);
		processor=processor->previousProcessor;
	}
	family->uninitFamilyFunction();
}

static bool InitProcessorFamily(PROCESSOR_FAMILY *family)
// Call the processor family's init function, and add each
// processor in the family's symbols to the table
{
	bool
		fail;
	PROCESSOR
		*processor;

	fail=false;
	if(family->initFamilyFunction())
	{
		processor=family->firstProcessor;
		while(processor&&!fail)
		{
			if((processor->symbol=STAddEntryAtEnd(processorSymbols,processor->name,processor)))
			{
				processor=processor->nextProcessor;
			}
			else
			{
				fail=true;
			}
		}
		if(!fail)
		{
			return(true);
		}
		processor=processor->previousProcessor;
		while(processor)
		{
			STRemoveEntry(processorSymbols,processor->symbol);
			processor=processor->previousProcessor;
		}
		family->uninitFamilyFunction();
	}
	return(false);
}

void UnInitProcessors()
// undo what InitProcessors did
{
	PROCESSOR_FAMILY
		*family;

	family=topProcessorFamily;
	while(family&&family->nextFamily)	// race to the bottom so we can un-init in reverse order
	{
		family=family->nextFamily;
	}
	while(family)
	{
		UnInitProcessorFamily(family);
		family=family->previousFamily;
	}
	STDisposeSymbolTable(processorSymbols);
}

bool InitProcessors()
// initialize symbol table for processor selection, and initialize all the processor family handler functions
{
	bool
		fail;
	PROCESSOR_FAMILY
		*family;

	currentProcessor=NULL;				// no processor chosen as current
	fail=false;
	if((processorSymbols=STNewSymbolTable(100)))
	{
		family=topProcessorFamily;
		while(family&&!fail)
		{
			if(InitProcessorFamily(family))
			{
				family=family->nextFamily;
			}
			else
			{
				ReportComplaint(true,"Failed to initialize processor family: %s\n",family->name);
				fail=true;
			}
		}
		if(!fail)
		{
			return(true);
		}
		family=family->previousFamily;
		while(family)
		{
			UnInitProcessorFamily(family);
			family=family->previousFamily;
		}
		STDisposeSymbolTable(processorSymbols);
	}
	else
	{
		ReportComplaint(true,"Failed to create symbol table for processors\n");
	}
	return(false);
}

static void DumpProcessorFamilyMembers(FILE *file,PROCESSOR_FAMILY *family)
// dump out the members of the given processor family
{
	int
		longestMember;
	PROCESSOR
		*processor;
	int
		length,
		sumLength;

	// first run through and find the longest name
	longestMember=0;
	processor=family->firstProcessor;
	while(processor)
	{
		length=strlen(processor->name);
		if(length>longestMember)
		{
			longestMember=length;
		}
		processor=processor->nextProcessor;
	}

	// now print out the names
	processor=family->firstProcessor;
	while(processor)
	{
		length=strlen(processor->name);
		fprintf(file,"  %s",processor->name);		// print the first item on the row, spaced in
		sumLength=(2+longestMember+1)+(longestMember+1);	// account for item just printed, and item about to be printed
		processor=processor->nextProcessor;
		while(processor&&(sumLength<80))
		{
			fprintf(file,"%*s%s",(longestMember-length)+1,"",processor->name);	// space over, and print the next entry
			sumLength+=longestMember+1;
			length=strlen(processor->name);			// get length of this one for next time
			processor=processor->nextProcessor;
		}
		fprintf(file,"\n");
	}
}

void DumpProcessorInformation(FILE *file)
// tell what processors and families are supported by the assembler
{
	PROCESSOR_FAMILY
		*family;

	family=topProcessorFamily;
	while(family)
	{
		fprintf(file,"%s\n",family->name);
		DumpProcessorFamilyMembers(file,family);
		family=family->nextFamily;
	}
}

// Constructors used to link processor functions to global list at run-time

PROCESSOR_FAMILY::PROCESSOR_FAMILY(const char *processorName,InitFamilyFunction *initFamily,UnInitFamilyFunction *unInitFamily,SelectProcessorFunction *selectProcessor,DeselectProcessorFunction *deselectProcessor,AttemptProcessorPseudoOpcodeFunction *attemptPseudoOpcode,AttemptProcessorOpcodeFunction *attemptOpcode)
// Use this to add a given processor family
// to the global list
{
	name=processorName;
	initFamilyFunction=initFamily;
	uninitFamilyFunction=unInitFamily;

	selectProcessorFunction=selectProcessor;
	deselectProcessorFunction=deselectProcessor;

	attemptPseudoOpcodeFunction=attemptPseudoOpcode;
	attemptOpcodeFunction=attemptOpcode;

	firstProcessor=NULL;
	lastProcessor=NULL;

	if((nextFamily=topProcessorFamily))				// link to next one
	{
		topProcessorFamily->previousFamily=this;	// link next one to this one
	}
	previousFamily=NULL;							// none previous to this
	topProcessorFamily=this;						// point at this one
}

PROCESSOR_FAMILY::~PROCESSOR_FAMILY()
// When a processor family goes out of scope, get rid of it here
// NOTE: all processors in this family must have already been destroyed
{
	if(nextFamily)
	{
		nextFamily->previousFamily=previousFamily;	// point next's previous to our previous
	}
	if(previousFamily)
	{
		previousFamily->nextFamily=nextFamily;		// point previous' next to our next
	}
	else
	{
		topProcessorFamily=nextFamily;				// if no previous, then we were the top, so set top to our next
	}
}

PROCESSOR::PROCESSOR(PROCESSOR_FAMILY *processorFamily,const char *familyName,const void *familyData)
// Use this to construct a processor which will be added to the
// list or processors supported by the assembler
{
	family=processorFamily;
	symbol=NULL;
	name=familyName;
	processorData=familyData;

	// link to end of processors in the family
	nextProcessor=NULL;
	if((previousProcessor=family->lastProcessor))
	{
		previousProcessor->nextProcessor=this;
	}
	else
	{
		family->firstProcessor=this;					// first in the family
	}
	family->lastProcessor=this;
}

PROCESSOR::~PROCESSOR()
// When a processor goes out of scope, get rid of it here 
{
	if(nextProcessor)
	{
		nextProcessor->previousProcessor=previousProcessor;	// point next's previous to our previous
	}
	else
	{
		family->lastProcessor=previousProcessor;
	}

	if(previousProcessor)
	{
		previousProcessor->nextProcessor=nextProcessor;		// point previous' next to our next
	}
	else
	{
		family->firstProcessor=nextProcessor;				// if no previous, then we were the top, so set top to our next
	}
}
