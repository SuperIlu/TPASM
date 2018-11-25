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


// Handle matching and processing of assembler pseudo-ops
// NOTE: assembler pseudo-ops are those that can be considered processor
// independant.

#include	"include.h"

static SYM_TABLE
	*strictPseudoOpcodeSymbols,
	*loosePseudoOpcodeSymbols;

// prototypes for the handler functions which need to be declared before we can build
// the opcode table

static bool HandleInclude(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleSeg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleSegU(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleOrg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleROrg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleAlign(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleAlias(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleUnAlias(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEqu(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleSet(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleUnSet(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleMacro(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEndMacro(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleIf(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleIfdef(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleIfndef(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleElse(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEndIf(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleSwitch(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleCase(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleBreak(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEndSwitch(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleRepeat(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEndRepeat(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleError(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleWarning(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleMessage(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleList(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleNoList(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleExpand(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleNoExpand(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleProcessor(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEnd(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

// All strict pseudo-ops begin with a . to keep them from colliding with opcodes for various
// processors.
static PSEUDO_OPCODE
	strictPseudoOpcodes[]=
	{
		{".include",	HandleInclude},
		{".seg",		HandleSeg},
		{".segu",		HandleSegU},
		{".seg.u",		HandleSegU},
		{".org",		HandleOrg},
		{".rorg",		HandleROrg},
		{".align",		HandleAlign},
		{".alias",		HandleAlias},
		{".unalias",	HandleUnAlias},
		{".equ",		HandleEqu},
		{".set",		HandleSet},
		{".unset",		HandleUnSet},
		{".macro",		HandleMacro},
		{".endm",		HandleEndMacro},
		{".if",			HandleIf},
		{".ifdef",		HandleIfdef},
		{".ifndef",		HandleIfndef},
		{".else",		HandleElse},
		{".endif",		HandleEndIf},
		{".switch",		HandleSwitch},
		{".case",		HandleCase},
		{".break",		HandleBreak},
		{".ends",		HandleEndSwitch},
		{".repeat",		HandleRepeat},
		{".endr",		HandleEndRepeat},
		{".error",		HandleError},
		{".warning",	HandleWarning},
		{".messg",		HandleMessage},
		{".message",	HandleMessage},
		{".list",		HandleList},
		{".nolist",		HandleNoList},
		{".expand",		HandleExpand},
		{".noexpand",	HandleNoExpand},
		{".processor",	HandleProcessor},
		{".end",		HandleEnd},
	};

// To stay compatible with old versions of tpasm, allow the loose versions
// unless told not to.
static PSEUDO_OPCODE
	loosePseudoOpcodes[]=
	{
		{"include",		HandleInclude},
		{"seg",			HandleSeg},
		{"segu",		HandleSegU},
		{"seg.u",		HandleSegU},
		{"org",			HandleOrg},
		{"rorg",		HandleROrg},
		{"align",		HandleAlign},
		{"alias",		HandleAlias},
		{"unalias",		HandleUnAlias},
		{"equ",			HandleEqu},
		{"set",			HandleSet},
		{"unset",		HandleUnSet},
		{"macro",		HandleMacro},
		{"endm",		HandleEndMacro},
		{"if",			HandleIf},
		{"ifdef",		HandleIfdef},
		{"ifndef",		HandleIfndef},
		{"else",		HandleElse},
		{"endif",		HandleEndIf},
		{"switch",		HandleSwitch},
		{"case",		HandleCase},
		{"break",		HandleBreak},
		{"ends",		HandleEndSwitch},
		{"repeat",		HandleRepeat},
		{"endr",		HandleEndRepeat},
		{"error",		HandleError},
		{"warning",		HandleWarning},
		{"messg",		HandleMessage},
		{"message",		HandleMessage},
		{"list",		HandleList},
		{"nolist",		HandleNoList},
		{"expand",		HandleExpand},
		{"noexpand",	HandleNoExpand},
		{"processor",	HandleProcessor},
		{"end",			HandleEnd},
	};


static bool HandleInclude(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// include the given file into the input stream
// If there is a problem, report it and return false
{
	bool
		fail;
	char
		includeName[MAX_STRING];
	unsigned int
		stringLength;
	bool
		hunt;

	fail=false;
	if(contextStack->active)			// if not active, includes are just ignored, since they should not change context anyway
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		hunt=false;
		if(ParseQuotedString(line,lineIndex,'"','"',includeName,&stringLength)||(hunt=true,ParseQuotedString(line,lineIndex,'<','>',includeName,&stringLength)))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				OutputListFileLine(listingRecord,line);	// output the line first so that the include contents follow it
				listingRecord->wantList=false;

				fail=!ProcessSourceFile(includeName,hunt);		// go handle this file now
			}
			else
			{
				AssemblyComplaint(NULL,true,"Missing or incorrectly formatted include file name\n");
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"Missing or incorrectly formatted include file name\n");
		}
	}
	return(!fail);
}

static bool HandleSeg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// create new segment, or select old one
{
	char
		element[MAX_STRING];
	unsigned int
		stringLength;
	SEGMENT_RECORD
		*segment;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',element,&stringLength)||ParseFirstListElement(line,lineIndex,element))	// see which processor is being chosen
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				if((segment=MatchSegment(element)))		// see if we can find an existing segment with this name
				{
					currentSegment=segment;
					if(!currentSegment->generateOutput)
					{
						AssemblyComplaint(NULL,false,"Segment is uninitialized (your reference here is for an initialized segment)\n");
					}
				}
				else
				{
					if((segment=CreateSegment(element,true)))		// create the new segment
					{
						currentSegment=segment;
					}
					else
					{
						ReportComplaint(true,"Failed to create segment\n");
						fail=true;
					}
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleSegU(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// create new uninitialized segment, or select old one
{
	char
		element[MAX_STRING];
	unsigned int
		stringLength;
	SEGMENT_RECORD
		*segment;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',element,&stringLength)||ParseFirstListElement(line,lineIndex,element))	// see which processor is being chosen
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				if((segment=MatchSegment(element)))		// see if we can find an existing segment with this name
				{
					currentSegment=segment;
					if(currentSegment->generateOutput)
					{
						AssemblyComplaint(NULL,false,"Segment is initialized (your reference here is for an uninitialized segment)\n");
					}
				}
				else
				{
					if((segment=CreateSegment(element,false)))	// create the new segment
					{
						currentSegment=segment;
					}
					else
					{
						ReportComplaint(true,"Failed to create segment\n");
						fail=true;
					}
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleOrg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Org statement, set PC within the current segment
// If there is a problem, report it and return false
{
	int
		address;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(ParseExpression(line,lineIndex,&address,&unresolved))
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				if(currentSegment)
				{
					if(!unresolved)								// if unresolved, then just ignore org for the moment (it will get resolved later)
					{
						currentSegment->currentPC=address;		// ORG to the given address
						if(lineLabel)
						{
							fail=!AssignLabel(lineLabel,currentSegment->currentPC+currentSegment->codeGenOffset);	// assign this label (after the ORG)
						}
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' outside of a segment\n",opcodeName);
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleROrg(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Relocatable org statement, set the offset between the PC (place where code is being
// written out, and place where code is being generated (offset is normally 0)
// If there is a problem, report it and return false
{
	int
		address;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(ParseExpression(line,lineIndex,&address,&unresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				if(currentSegment)
				{
					if(!unresolved)								// if unresolved, then just ignore org for the moment (it will get resolved later)
					{
						currentSegment->codeGenOffset=address-currentSegment->currentPC;
						if(lineLabel)
						{
							fail=!AssignLabel(lineLabel,currentSegment->currentPC+currentSegment->codeGenOffset);	// assign this label (after the RORG)
						}
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' outside of a segment\n",opcodeName);
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleAlign(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Align the PC to a boundary which is given
// if 0 or 1 are given, do nothing
// If there is a problem, report it and return false
{
	int
		value;
	bool
		unresolved;
	unsigned int
		offset;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				if(lineLabel)
				{
					ReportDisallowedLabel(lineLabel);
				}

				if(currentSegment)
				{
					if(!unresolved)								// if unresolved, then just ignore org for the moment (it will get resolved later)
					{
						if(value>0)
						{
							offset=currentSegment->currentPC%value;
							if(offset)
							{
								currentSegment->currentPC+=value-offset;
							}
						}
						else
						{
							AssemblyComplaint(NULL,false,"Bad value for align (%d) ignored\n",value);
						}
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' outside of segment\n",opcodeName);
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleAlias(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// ALIAS statement, set definition
// If there is a hard error, return false
{
	ALIAS_RECORD
		*aliasMatch;
	char
		aliasValue[MAX_STRING];
	unsigned int
		stringLength;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseFirstNameElement(line,lineIndex,aliasValue)||ParseQuotedString(line,lineIndex,'"','"',aliasValue,&stringLength))	// think of operands as a list of one element, or as a quoted string
				{
					if(ParseComment(line,lineIndex))				// must be at the end of the line now
					{
						if(!(aliasMatch=MatchAlias(lineLabel->name)))		// make sure it does not already exist
						{
							if(!CreateAlias(lineLabel->name,aliasValue))	// create the alias, complain if there's trouble
							{
								ReportComplaint(true,"Failed to create alias record\n");
								fail=true;
							}
						}
						else
						{
							AssemblyComplaint(NULL,true,"Alias '%s' already defined\n",lineLabel);
							AssemblySupplement(&aliasMatch->whereFrom,"Conflicting alias was defined here\n");
						}
					}
					else
					{
						ReportBadOperands();
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"Bad alias replacement declaration\n");
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleUnAlias(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// get rid of an alias which currently exists
// If there is a hard error, return false
{
	ALIAS_RECORD
		*aliasMatch;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseComment(line,lineIndex))				// must be at the end of the line now
				{
					if((aliasMatch=MatchAlias(lineLabel->name)))		// does this alias already exist?
					{
						DestroyAlias(aliasMatch);				// get rid of the alias
					}
					else
					{
						AssemblyComplaint(NULL,true,"'%s' not defined as an alias\n",lineLabel);
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleEqu(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// EQU statement, set label
// If there is a hard error, return false
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseExpression(line,lineIndex,&value,&unresolved))
				{
					if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
					{
						if(!intermediatePass)
						{
							CreateListStringValue(listingRecord,value,unresolved);
						}
						fail=!AssignConstant(lineLabel->name,value,!unresolved);	// assign it (even if it is unresolved, since this way we can remember that there was an attempt to equate it)
					}
					else
					{
						ReportBadOperands();
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleSet(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// SET statement, set label
// If there is a problem, report it and return false
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseExpression(line,lineIndex,&value,&unresolved))
				{
					if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
					{
						if(!intermediatePass)
						{
							CreateListStringValue(listingRecord,value,unresolved);
						}
						fail=!AssignSetConstant(lineLabel->name,value,!unresolved);	// assign it (even if it is unresolved, since this way we can remember that there was an attempt to equate it)
					}
					else
					{
						ReportBadOperands();
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleUnSet(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// UNSET statement, clear previously existing set label
// If there is a problem, report it and return false
{
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseComment(line,lineIndex))			// make sure there's nothing else on the line
				{
					UnAssignSetConstant(lineLabel->name);	// unassign it
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}


static bool HandleMacro(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// A macro is being defined
// If there is a problem, report it and return false
// NOTE: even if the macro command was ill formed, we still enter the macro
// context (with contextStack->active=false). This generates the least confusing error output.
{
	bool
		fail;
	MACRO_RECORD
		*buildingMacro,
		*macroMatch;
	char
		macroName[MAX_STRING];
	unsigned int
		i;
	bool
		wasActive;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(0))
	{
		contextStack->contextType=CT_MACRO;
		contextStack->active=false;
		if(wasActive)
		{
			if(lineLabel)
			{
				if(!lineLabel->isLocal)
				{
					i=0;
					while(lineLabel->name[i])					// make macro name lower case for later case insensitive matching
					{
						macroName[i]=tolower(lineLabel->name[i]);
						i++;
					}
					macroName[i]='\0';

					if(!(macroMatch=LocateMacro(macroName)))		// does this macro already exist?
					{
						if((buildingMacro=CreateMacro(macroName)))
						{
							if(CreateParameterNames(line,lineIndex,&buildingMacro->parameters))
							{
								collectingBlock=&buildingMacro->contents;	// start collecting the text here (even if parameters are bad)
								if(!ParseComment(line,lineIndex))
								{
									AssemblyComplaint(NULL,true,"Ill formed macro parameters\n");
								}
							}
							else
							{
								DestroyMacro(buildingMacro);	// failed to create parameter list
								fail=true;
							}
						}
						else
						{
							ReportComplaint(true,"Failed to create macro record\n");
							fail=true;
						}
					}
					else
					{
						AssemblyComplaint(NULL,true,"Macro '%s' already defined\n",macroName);
						AssemblySupplement(&macroMatch->whereFrom,"Conflicting macro was defined here\n");
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleEndMacro(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Process end of macro definition
// If there is a problem, report it and return false
{
	bool
		fail;

	fail=false;
	if(contextStack->contextType==CT_MACRO)
	{
		PopContextRecord();
		if(contextStack->active)						// parent was active, therefore we were collecting text into this macro
		{
			collectingBlock=NULL;						// stop collecting the block, we have the macro now
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(!ParseComment(line,lineIndex))		// make sure there's nothing else on the line
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' outside of 'macro'\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleIf(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Check condition, execute code immediately following if true, or after the optional else
// if false.
// If there is a problem, report it and return false
// NOTE: if the expression is undefined, neither the if, or the else
// part will be executed. The "if" statement will remain undefined until the last
// pass which defines it, or until it cannot be defined
{
	int
		value;
	bool
		unresolved;
	bool
		fail;
	bool
		wasActive;
	IF_CONTEXT
		*ifContext;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(sizeof(IF_CONTEXT)))					// change context
	{
		contextStack->contextType=CT_IF;
		contextStack->active=wasActive;

		ifContext=(IF_CONTEXT *)&contextStack->contextData[0];
		ifContext->sawElse=false;
		ifContext->wantElse=false;

		if(contextStack->active)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseExpression(line,lineIndex,&value,&unresolved))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					if(!intermediatePass)
					{
						CreateListStringValue(listingRecord,value,unresolved);
					}
					if(!unresolved)
					{
						if(value==0)							// use the code below the "if"
						{
							contextStack->active=false;			// go inactive, go active later on else if currently active
							ifContext->wantElse=true;
						}
					}
					else
					{
						contextStack->active=false;				// unresolved, so take neither path
					}
				}
				else
				{
					ReportBadOperands();
					contextStack->active=false;
				}
			}
			else
			{
				ReportBadOperands();
				contextStack->active=false;
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleIfdef(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Check to see if a label is defined, execute code immediately following if true,
// or after the optional else if false.
// If there is a problem, report it and return false
{
	bool
		fail;
	bool
		wasActive;
	IF_CONTEXT
		*ifContext;
	char
		label[MAX_STRING];
	bool
		local;
	LABEL_RECORD
		*labelRecord;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(sizeof(IF_CONTEXT)))					// change context
	{
		contextStack->contextType=CT_IF;
		contextStack->active=wasActive;

		ifContext=(IF_CONTEXT *)&contextStack->contextData[0];
		ifContext->sawElse=false;
		ifContext->wantElse=false;

		if(contextStack->active)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseLabelString(line,lineIndex,&label[0],&local))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					if((labelRecord=LocateLabel(label,!intermediatePass)))	// if we found it, it's defined (even if it is not resolved)
					{
						// already active, so no more needs to be done here
					}
					else
					{
						contextStack->active=false;				// not defined, so take else path
						ifContext->wantElse=true;
					}
				}
				else
				{
					ReportBadOperands();
					contextStack->active=false;
				}
			}
			else
			{
				ReportBadOperands();
				contextStack->active=false;
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleIfndef(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Check to see if a label is not defined, execute code immediately following if true,
// or after the optional else if false.
// If there is a problem, report it and return false
{
	bool
		fail;
	bool
		wasActive;
	IF_CONTEXT
		*ifContext;
	char
		label[MAX_STRING];
	bool
		local;
	LABEL_RECORD
		*labelRecord;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(sizeof(IF_CONTEXT)))					// change context
	{
		contextStack->contextType=CT_IF;
		contextStack->active=wasActive;

		ifContext=(IF_CONTEXT *)&contextStack->contextData[0];
		ifContext->sawElse=false;
		ifContext->wantElse=false;

		if(contextStack->active)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseLabelString(line,lineIndex,&label[0],&local))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					if(!(labelRecord=LocateLabel(label,!intermediatePass)))
					{
						// already active, so no more needs to be done here
					}
					else
					{
						contextStack->active=false;				// was defined, so want to take else path if there is one
						ifContext->wantElse=true;
					}
				}
				else
				{
					ReportBadOperands();
					contextStack->active=false;
				}
			}
			else
			{
				ReportBadOperands();
				contextStack->active=false;
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleElse(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Else pseudo-op handling
{
	IF_CONTEXT
		*ifContext;

	if(contextStack->contextType==CT_IF)
	{
		ifContext=(IF_CONTEXT *)&contextStack->contextData[0];
		if(!ifContext->sawElse)
		{
			ifContext->sawElse=true;
			if(contextStack->active||ifContext->wantElse)
			{
				contextStack->active=ifContext->wantElse;
				if(lineLabel)
				{
					ReportDisallowedLabel(lineLabel);
				}

				if(!ParseComment(line,lineIndex))					// make sure there's nothing else on the line
				{
					ReportBadOperands();
				}
			}
		}
		else
		{
			if(contextStack->active)
			{
				AssemblyComplaint(NULL,true,"'%s' already seen\n",opcodeName);
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' without 'if'\n",opcodeName);
		}
	}
	return(true);
}

static bool HandleEndIf(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Endif pseudo-op handling
{
	if(contextStack->contextType==CT_IF)
	{
		PopContextRecord();
		if(contextStack->active)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(!ParseComment(line,lineIndex))	// make sure there's nothing else on the line
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' without 'if'\n",opcodeName);
		}
	}
	return(true);
}

static bool HandleSwitch(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Evaluate an expression, find a case to match it, assemble code under the case
// that it matches
// If there is a problem, report it and return false
// NOTE: if the expression is undefined, no case will be executed
// if a case's expression is undefined, it will be considered a non-match
// NOTE: a switch is inactive until the first case that matches it
// NOTE: the switch goes inactive when a break is encountered
{
	int
		value;
	bool
		unresolved;
	bool
		fail;
	bool
		wasActive;
	SWITCH_CONTEXT
		*switchContext;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(sizeof(SWITCH_CONTEXT)))				// change context
	{
		contextStack->contextType=CT_SWITCH;
		contextStack->active=false;								// switch immediately goes inactive

		switchContext=(SWITCH_CONTEXT *)&contextStack->contextData[0];
		switchContext->wantCase=false;
		switchContext->unresolved=true;
		switchContext->value=0;

		if(wasActive)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseExpression(line,lineIndex,&value,&unresolved))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					if(!intermediatePass)
					{
						CreateListStringValue(listingRecord,value,unresolved);
					}
					if(!unresolved)
					{
						switchContext->wantCase=true;
						switchContext->unresolved=false;
						switchContext->value=value;
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleCase(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Case pseudo-op handling
{
	int
		value;
	bool
		unresolved;
	SWITCH_CONTEXT
		*switchContext;

	if(contextStack->contextType==CT_SWITCH)
	{
		switchContext=(SWITCH_CONTEXT *)&contextStack->contextData[0];

		if(switchContext->wantCase)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseExpression(line,lineIndex,&value,&unresolved))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					if(!intermediatePass)
					{
						CreateListStringValue(listingRecord,value,unresolved);
					}
					if(!unresolved)
					{
						if(value==switchContext->value)
						{
							contextStack->active=true;			// case is active, start assembling
						}
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' without 'switch'\n",opcodeName);
		}
	}
	return(true);
}

static bool HandleBreak(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Break pseudo-op handling (part of switch)
{
	SWITCH_CONTEXT
		*switchContext;

	if(contextStack->contextType==CT_SWITCH)
	{
		switchContext=(SWITCH_CONTEXT *)&contextStack->contextData[0];
		contextStack->active=false;							// honor the break

		if(switchContext->wantCase)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(!ParseComment(line,lineIndex))			// make sure there's nothing else on the line
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' without 'switch'\n",opcodeName);
		}
	}
	return(true);
}

static bool HandleEndSwitch(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// End of switch statement
{
	if(contextStack->contextType==CT_SWITCH)
	{
		PopContextRecord();
		if(contextStack->active)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(!ParseComment(line,lineIndex))	// make sure there's nothing else on the line
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' without 'switch'\n",opcodeName);
		}
	}
	return(true);
}

static void RepeatFlush(CONTEXT_RECORD *record)
// if an unterminated repeat context lingers at the end of assembly, this will get
// called to clean it up
{
	REPEAT_CONTEXT
		*repeatContext;

	repeatContext=(REPEAT_CONTEXT *)&contextStack->contextData[0];
	DestroyTextBlockLines(&repeatContext->textBlock);
}

static bool HandleRepeat(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// A section of code which may be repeated is being defined
// If there is a problem, report it and return false
// This works by collecting the text up until the end opcode is seen, then
// entering it into the stream as many times as requested
{
	bool
		fail;
	int
		value;
	bool
		unresolved;
	bool
		wasActive;
	REPEAT_CONTEXT
		*repeatContext;

	fail=false;

	wasActive=contextStack->active;
	if(PushContextRecord(sizeof(REPEAT_CONTEXT)))
	{
		contextStack->contextType=CT_REPEAT;
		contextStack->active=false;
		contextStack->flush=RepeatFlush;
		repeatContext=(REPEAT_CONTEXT *)&contextStack->contextData[0];
		repeatContext->numTimes=0;
		repeatContext->textBlock.firstLine=repeatContext->textBlock.lastLine=NULL;	// no lines to be repeated
		
		if(wasActive)
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(ParseExpression(line,lineIndex,&value,&unresolved))
			{
				if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
				{
					if(!intermediatePass)
					{
						CreateListStringValue(listingRecord,value,unresolved);
					}
					if(!unresolved)
					{
						if(value>=0)
						{
							repeatContext->numTimes=value;
						}
						else
						{
							AssemblyComplaint(NULL,false,"Negative repeat count (assuming 0)\n");
						}
						collectingBlock=&repeatContext->textBlock;	// start collecting the text here
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
	}
	else
	{
		ReportComplaint(true,"Failed to create context record\n");
		fail=true;
	}
	return(!fail);
}

static bool HandleEndRepeat(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Process end of repeated code (enter the code back into the stream if the repeat count is > 0)
// If there is a problem, report it and return false
{
	bool
		fail;
	unsigned int
		i;
	REPEAT_CONTEXT
		*repeatContext;

	fail=false;
	if(contextStack->contextType==CT_REPEAT)
	{
		if(contextStack->next->active)					// want to repeat within active parent context?
		{
			contextStack->active=true;					// set our context active now so stuff below executes
			collectingBlock=NULL;						// stop collecting the block, we have the repeated text now

			repeatContext=(REPEAT_CONTEXT *)&contextStack->contextData[0];

			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if(!ParseComment(line,lineIndex))		// make sure there's nothing else on the line
			{
				ReportBadOperands();
			}

			OutputListFileLine(listingRecord,line);	// output the line first so that the repeat contents follow it
			listingRecord->wantList=false;

			for(i=0;!fail&&(i<repeatContext->numTimes);i++)
			{
				fail=!ProcessTextBlock(&repeatContext->textBlock,NULL,NULL,'R');
			}
		}
		RepeatFlush(contextStack);						// flush out saved repeat lines
		PopContextRecord();								// get rid of this context record
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"'%s' outside of 'repeat'\n",opcodeName);
		}
	}
	return(!fail);
}

static bool HandleError(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Report user generated error
{
	char
		errorString[MAX_STRING];
	unsigned int
		stringLength;

	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',errorString,&stringLength))
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				AssemblyComplaint(NULL,true,"%s\n",errorString);
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleWarning(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Report user generated warning
{
	char
		warningString[MAX_STRING];
	unsigned int
		stringLength;

	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',warningString,&stringLength))
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				AssemblyComplaint(NULL,false,"%s\n",warningString);
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleMessage(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Report user generated message
{
	char
		messageString[MAX_STRING];
	unsigned int
		stringLength;

	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',messageString,&stringLength))
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				AssemblySupplement(NULL,"%s\n",messageString);
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleList(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// turn on output listing
{
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
		{
			if(!outputListing)
			{
				listingRecord->wantList=false;				// do not output the list command itself if we weren't listing before
			}
			outputListing=true;
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleNoList(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// turn off output listing
{
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
		{
			outputListing=false;
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleExpand(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// turn on macro expansion in listing
{
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
		{
			outputListingExpansions=true;
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleNoExpand(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// turn off macro expansion in listing
{
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
		{
			outputListingExpansions=false;
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

static bool HandleProcessor(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// select processor to assemble for
{
	char
		element[MAX_STRING];
	unsigned int
		stringLength;
	bool
		fail;
	bool
		found;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseQuotedString(line,lineIndex,'"','"',element,&stringLength)||ParseFirstListElement(line,lineIndex,element))	// see which processor is being chosen
		{
			if(ParseComment(line,lineIndex))					// make sure there's nothing else on the line
			{
				if(SelectProcessor(element,&found))
				{
					if(!found)
					{
						AssemblyComplaint(NULL,true,"Unrecognized processor '%s'\n",element);
					}
				}
				else
				{
					fail=true;
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandleEnd(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// We have been told to stop parsing input
{
	if(contextStack->active)
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}

		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			stopParsing=true;							// tell main loop to stop processing the current file
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(true);
}

bool AttemptGlobalPseudoOpcode(const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success)
// See if the next thing on the line looks like a global pseudo-op.
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	unsigned int
		tempIndex;
	char
		string[MAX_STRING];
	PSEUDO_OPCODE
		*opcode;

	*success=false;
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=(PSEUDO_OPCODE *)STFindDataForNameNoCase(strictPseudoOpcodeSymbols,string))||((!strictPseudo)&&(opcode=(PSEUDO_OPCODE *)STFindDataForNameNoCase(loosePseudoOpcodeSymbols,string))))			// matches global pseudo-op?
		{
			*success=true;										// remember we saw something
			*lineIndex=tempIndex;								// actually push forward on the line
			return(opcode->function(string,line,lineIndex,lineLabel,listingRecord));
		}
	}
	return(true);
}

void UnInitGlobalPseudoOpcodes()
// undo what InitGlobalPseudoOpcodes did
{
	STDisposeSymbolTable(loosePseudoOpcodeSymbols);
	STDisposeSymbolTable(strictPseudoOpcodeSymbols);
}

bool InitGlobalPseudoOpcodes()
// initialize symbol table for assembler pseudo-ops
{
	unsigned int
		i;
	bool
		fail;

	fail=false;
	if((strictPseudoOpcodeSymbols=STNewSymbolTable(elementsof(strictPseudoOpcodes))))
	{
		for(i=0;!fail&&(i<elementsof(strictPseudoOpcodes));i++)
		{
			if(!STAddEntryAtEnd(strictPseudoOpcodeSymbols,strictPseudoOpcodes[i].name,&strictPseudoOpcodes[i]))
			{
				fail=true;
			}
		}
		if(!fail)
		{
			if((loosePseudoOpcodeSymbols=STNewSymbolTable(elementsof(loosePseudoOpcodes))))
			{
				for(i=0;!fail&&(i<elementsof(loosePseudoOpcodes));i++)
				{
					if(!STAddEntryAtEnd(loosePseudoOpcodeSymbols,loosePseudoOpcodes[i].name,&loosePseudoOpcodes[i]))
					{
						fail=true;
					}
				}
				if(!fail)
				{
					return(true);
				}
				STDisposeSymbolTable(loosePseudoOpcodeSymbols);
			}
		}
		STDisposeSymbolTable(strictPseudoOpcodeSymbols);
	}
	return(false);
}
