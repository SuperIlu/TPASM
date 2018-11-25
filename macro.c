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


// Handle creation and manipulation of macros, and other text block related functions

#include	"include.h"

static SYM_TABLE
	*macroSymbols;											// macro symbol list is kept here

void DestroyTextBlockLines(TEXT_BLOCK *block)
{
	TEXT_LINE
		*tempLine;

	while(block->firstLine)								// get rid of lines of text
	{
		tempLine=block->firstLine->next;
		DisposePtr(block->firstLine);
		block->firstLine=tempLine;
	}
}

bool AddLineToTextBlock(TEXT_BLOCK *block,const char *line)
// Add line to block
{
	unsigned int
		length;
	bool
		fail;
	TEXT_LINE
		*textLine;

	fail=false;
	length=strlen(line);
	if((textLine=(TEXT_LINE *)NewPtr(sizeof(TEXT_LINE)+length+1)))
	{
		textLine->next=NULL;
		textLine->whereFrom.file=currentVirtualFile;
		textLine->whereFrom.fileLineNumber=currentVirtualFileLine;
		strcpy(&textLine->line[0],line);
		if(block->lastLine)
		{
			block->lastLine->next=textLine;	// link onto the end
			block->lastLine=textLine;
		}
		else
		{
			block->firstLine=block->lastLine=textLine;	// link in as first line
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

bool CreateParameterList(const char *line,unsigned int *lineIndex,TEXT_BLOCK *block)
// Parse out a list of text elements
// return false if there was a hard error
{
	char
		element[MAX_STRING];
	bool
		fail;

	fail=false;
	if(ParseFirstListElement(line,lineIndex,element))
	{
		fail=!AddLineToTextBlock(block,element);
		while(!fail&&ParseNextListElement(line,lineIndex,element))
		{
			fail=!AddLineToTextBlock(block,element);
		}
	}
	if(fail)
	{
		ReportComplaint(true,"Failed to create parameter list element\n");
	}
	return(!fail);
}

bool CreateParameterNames(const char *line,unsigned int *lineIndex,TEXT_BLOCK *block)
// Parse out a list of name elements
// return false if there was a hard error
{
	char
		element[MAX_STRING];
	bool
		fail;

	fail=false;
	if(ParseFirstNameElement(line,lineIndex,element))
	{
		fail=!AddLineToTextBlock(block,element);
		while(!fail&&ParseNextNameElement(line,lineIndex,element))
		{
			fail=!AddLineToTextBlock(block,element);
		}
	}
	if(fail)
	{
		ReportComplaint(true,"Failed to create parameter label element\n");
	}
	return(!fail);
}

MACRO_RECORD *LocateMacro(const char *name)
// Attempt to locate a macro with the given name. Return its record if found, NULL if not.
{
	MACRO_RECORD
		*record;

	if((record=(MACRO_RECORD *)STFindDataForName(macroSymbols,name)))
	{
		return(record);
	}
	return(NULL);
}

bool AttemptMacro(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// See if the next thing on the line looks like a macro invocation, if so, handle it.
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	bool
		result;
	unsigned int
		tempIndex;
	char
		string[MAX_STRING];
	MACRO_RECORD
		*record;
	TEXT_BLOCK
		paramValues;

	result=true;			// assume no hard failure
	*success=false;			// so far, no macro matched
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like a macro?
	{
		if((record=(MACRO_RECORD *)STFindDataForNameNoCase(macroSymbols,string)))
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;

			paramValues.firstLine=paramValues.lastLine=NULL;
			if((result=CreateParameterList(line,lineIndex,&paramValues)))
			{
				if(ParseComment(line,lineIndex))
				{
					listingRecord->sourceType='m';				// output a small 'm' on lines which invoke a macro
					OutputListFileLine(listingRecord,line);		// output the line first so that the macro contents follow it
					listingRecord->wantList=false;

					result=ProcessTextBlock(&record->contents,&record->parameters,&paramValues,'M');	// process text of macro back into assembly stream
				}
				else
				{
					AssemblyComplaint(NULL,true,"Ill formed macro parameters\n");
				}
			}
			DestroyTextBlockLines(&paramValues);
		}
	}
	return(result);
}

void DestroyMacro(MACRO_RECORD *macro)
// remove macro from existence
{
	DestroyTextBlockLines(&macro->parameters);
	DestroyTextBlockLines(&macro->contents);

	STRemoveEntry(macroSymbols,macro->symbol);

	if(macro->next)
	{
		macro->next->previous=macro->previous;
	}

	if(macro->previous)
	{
		macro->previous->next=macro->next;
	}
	else
	{
		macrosHead=macro->next;
	}
	DisposePtr(macro);
}

void DestroyMacros()
// remove all macros, and all symbols from the symbol table
{
	while(macrosHead)
	{
		DestroyMacro(macrosHead);
	}
}

MACRO_RECORD *CreateMacro(char *macroName)
// Create a macro record, link it to the head of the global list, create a symbol table entry for it
{
	MACRO_RECORD
		*record;

	if((record=(MACRO_RECORD *)NewPtr(sizeof(MACRO_RECORD))))
	{
		record->parameters.firstLine=NULL;
		record->parameters.lastLine=NULL;
		record->contents.firstLine=NULL;
		record->contents.lastLine=NULL;
		record->whereFrom.file=currentVirtualFile;
		record->whereFrom.fileLineNumber=currentVirtualFileLine;

		if((record->symbol=STAddEntryAtEnd(macroSymbols,macroName,record)))
		{
			record->previous=NULL;
			if((record->next=macrosHead))
			{
				record->next->previous=record;	// make reverse link
			}
			macrosHead=record;
			return(record);
		}
		DisposePtr(record);
	}
	return(NULL);
}

void UnInitMacros()
// undo what InitMacros did
{
	STDisposeSymbolTable(macroSymbols);
}

bool InitMacros()
// initialize symbol table for macros
{
	if((macroSymbols=STNewSymbolTable(0)))
	{
		return(true);
	}
	return(false);
}
