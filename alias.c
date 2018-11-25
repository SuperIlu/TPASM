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


// Handle creation and manipulation of operand aliases

#include	"include.h"

static SYM_TABLE
	*aliasSymbols;						// alias symbol list is kept here

ALIAS_RECORD *MatchAlias(const char *operand)
// try to match operand against the list of aliases
{
	void
		*resultValue;

	if((resultValue=STFindDataForName(aliasSymbols,operand)))
	{
		return((ALIAS_RECORD *)resultValue);
	}
	return(NULL);
}

bool HandleAliasMatches(char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// Scans the line for anything matching an "alias" and replaces it with the
// contents of that alias.  This is called after any initial label on the line
// has been processed.
// NOTE: this will detect when the output would grow too long, and complain.
// NOTE: this is called often, so it attempts to minimize copying data unless a
// substiution is actually going to take place
// NOTE: for more speed, this checks to see if any aliases have even been defined
// before attempting any substitutions.
{
	ALIAS_RECORD
		*record;
	char
		substitutionText[MAX_STRING],
		token[MAX_STRING];
	unsigned int
		writeBackIndex,					// place where data will be written back to output line if substitution is needed
		subOutIndex,					// tells where the next data gets written into the substitutionText array
		tempIndex,						// temporary index
		skipIndex,						// keeps track of where we are currently in the input data
		tokenIndex,						// used to copy data to token
		inIndex;						// keeps an index to the start of source data to be written into the substitutionText array
	bool
		haveSubstitution;
	bool
		tooLong;

	if(aliasesHead)							// skip all of this if there are no aliases defined
	{
		writeBackIndex=*lineIndex;			// keep track of where we are now (do not modify lineIndex)
		SkipWhiteSpace(line,&writeBackIndex);	// skip over any initial whitespace
		inIndex=skipIndex=writeBackIndex;	// begin reading from here
		subOutIndex=0;						// start writing into substitution array here
		tooLong=false;
		haveSubstitution=false;				// so far, no substitution is needed
		while(!tooLong&&!ParseComment(line,&skipIndex))	// keep going until end of line, or substitution text too long
		{
			if(IsLabelChar(line[skipIndex]))	// start of a token?
			{
				tempIndex=skipIndex;		// remember where we are, so if there's a match, we can copy up to this point
				tokenIndex=0;
				do
				{
					token[tokenIndex++]=line[skipIndex++];	// scoop up the token
				} while(IsLabelChar(line[skipIndex]));

				token[tokenIndex]='\0';		// terminate the token so we can do the match

				if((record=MatchAlias(token)))	// see if an alias can be found which matches this token
				{
					if((subOutIndex+(tempIndex-inIndex)+record->contentLength)<MAX_STRING)	// make sure the whole thing fits
					{
						memcpy(&substitutionText[subOutIndex],&line[inIndex],tempIndex-inIndex);	// copy the data up to this substitution
						subOutIndex+=tempIndex-inIndex;			// add to the output index
						memcpy(&substitutionText[subOutIndex],&record->contents[0],record->contentLength);	// copy in the substituted text
						subOutIndex+=record->contentLength;	// move over for that too
						inIndex=skipIndex;	// next copy starts from here
					}
					else
					{
						tooLong=true;		// had a problem
					}
					haveSubstitution=true;	// found something, so remember it
				}
			}
			else
			{
				skipIndex++;				// move through all the non-label junk
			}
		}
		if(haveSubstitution&&!tooLong)		// see if there was a substitution -- if so, update the line
		{
			if(writeBackIndex+subOutIndex+(skipIndex-inIndex)+1<MAX_STRING)		// make sure there's room in the output for the substitution
			{
				listingRecord->sourceType='a';
				OutputListFileLine(listingRecord,line);	// output the original line
				listingRecord->sourceType='A';
				memmove(&line[writeBackIndex+subOutIndex],&line[inIndex],skipIndex-inIndex);	// move over the stuff past the end of the last substitution
				memcpy(&line[writeBackIndex],substitutionText,subOutIndex);	// blast in the substitution text
				line[writeBackIndex+subOutIndex+(skipIndex-inIndex)]='\0';	// re-terminate the line
			}
			else
			{
				tooLong=true;
			}
		}
		if(tooLong)
		{
			AssemblyComplaint(NULL,true,"Alias substitution too long\n");
		}
	}
	return(true);							// this never fails hard
}

void DestroyAlias(ALIAS_RECORD *alias)
// remove alias from existence
{
	STRemoveEntry(aliasSymbols,alias->symbol);

	if(alias->next)
	{
		alias->next->previous=alias->previous;
	}

	if(alias->previous)
	{
		alias->previous->next=alias->next;
	}
	else
	{
		aliasesHead=alias->next;
	}
	DisposePtr(alias);
}

void DestroyAliases()
// remove all aliases, and all symbols from the symbol table
{
	while(aliasesHead)
	{
		DestroyAlias(aliasesHead);
	}
}

ALIAS_RECORD *CreateAlias(const char *aliasName,const char *aliasString)
// Create an alias record, link it to the head of the global list, create a symbol table entry for it
{
	unsigned int
		length;
	ALIAS_RECORD
		*record;

	length=strlen(aliasString);
	if((record=(ALIAS_RECORD *)NewPtr(sizeof(ALIAS_RECORD)+length+1)))
	{
		record->whereFrom.file=currentVirtualFile;
		record->whereFrom.fileLineNumber=currentVirtualFileLine;

		record->contentLength=length;
		strcpy(&record->contents[0],aliasString);

		if((record->symbol=STAddEntryAtEnd(aliasSymbols,aliasName,record)))
		{
			record->previous=NULL;
			if((record->next=aliasesHead))
			{
				record->next->previous=record;	// make reverse link
			}
			aliasesHead=record;
			return(record);
		}
		DisposePtr(record);
	}
	return(NULL);
}

void UnInitAliases()
// undo what InitAliases did
{
	STDisposeSymbolTable(aliasSymbols);
}

bool InitAliases()
// initialize symbol table for aliases
{
	if((aliasSymbols=STNewSymbolTable(0)))
	{
		return(true);
	}
	return(false);
}
