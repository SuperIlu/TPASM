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


// Assembler label management

#include	"include.h"

static SYM_TABLE
	*labelSymbols;								// symbol table

unsigned int NumLabels()
// Count up the number of labels
{
	LABEL_RECORD
		*label;
	unsigned int
		numLabels;

	label=labelsHead;
	numLabels=0;
	while(label)
	{
		numLabels++;
		label=label->next;
	}
	return(numLabels);
}

LABEL_RECORD *LocateLabel(const char *labelName,bool bumpRefCount)
// Given a label name, return its record (if it can be located)
// If there is no label, return NULL
// If bumpRefCount is true, increment the refCount for this label
{
	LABEL_RECORD
		*resultValue;

	if((resultValue=(LABEL_RECORD *)STFindDataForName(labelSymbols,labelName)))
	{
		if(bumpRefCount)
		{
			resultValue->refCount++;
		}
		return(resultValue);
	}
	return(NULL);
}

void DestroyLabel(LABEL_RECORD *label)
// remove label from the global label table
{
	STRemoveEntry(labelSymbols,label->symbol);

	if(label->next)
	{
		label->next->previous=label->previous;
	}

	if(label->previous)
	{
		label->previous->next=label->next;
	}
	else
	{
		labelsHead=label->next;
	}
	DisposePtr(label);
}

static void ReportLabelDefinitionLocation(LABEL_RECORD *labelRecord)
// Report as a supplementary message, the location where labelRecord
// was defined
{
	AssemblySupplement(&labelRecord->whereFrom,"Previous definition was here\n");
}

LABEL_RECORD *CreateLabel(const char *labelName,int value,unsigned int type,unsigned int passCount,bool resolved)
// Create a new label in the global table
// If there is a problem, report it, and return NULL
{
	LABEL_RECORD
		*newRecord;

	if(!LocateLabel(labelName,false))
	{
		if((newRecord=(LABEL_RECORD *)NewPtr(sizeof(LABEL_RECORD))))
		{
			if((newRecord->symbol=STAddEntryAtEnd(labelSymbols,labelName,newRecord)))
			{
				newRecord->value=value;
				newRecord->type=type;
				newRecord->passCount=passCount;
				newRecord->resolved=resolved;
				newRecord->refCount=0;
				newRecord->whereFrom.file=currentVirtualFile;
				newRecord->whereFrom.fileLineNumber=currentVirtualFileLine;
				newRecord->previous=NULL;
				if((newRecord->next=labelsHead))
				{
					labelsHead->previous=newRecord;
				}
				labelsHead=newRecord;
				return(newRecord);
			}
			else
			{
				ReportComplaint(true,"Failed to create symbol table entry (Out of memory)\n");
			}
			DisposePtr(newRecord);
		}
		else
		{
			ReportComplaint(true,"Failed to create label (Out of memory)\n");
		}
	}
	else
	{
		ReportComplaint(true,"Label '%s' already exists\n",labelName);
	}
	return(NULL);
}

bool AssignLabel(const PARSED_LABEL *parsedLabel,int value)
// A line label has been encountered.
// This is called to assign a label, check what is happening,
// set flags, and complain as needed.
// If there is a problem assigning the label (hard error), return false
{
	LABEL_RECORD
		*oldLabel;
	bool
		fail;

	fail=false;
	if((oldLabel=LocateLabel(parsedLabel->name,false)))			// see if there is a definition for this already
	{
		if(oldLabel->type==LF_LABEL)				// make sure it is a label, and not a constant
		{
			if(oldLabel->passCount<passCount)		// verify we have not seen this label so far this pass
			{
				oldLabel->passCount=passCount;		// update the passcount
				if(!parsedLabel->isLocal)						// if not local, then update the current scope
				{
					strcpy(scope,parsedLabel->name);
				}
				if(oldLabel->value!=value)	// see if value is changing between passes
				{
					ReportDiagnostic("Modified label: '%s' (old=%08X, new=%08X)\n",parsedLabel->name,oldLabel->value,value);
					numModifiedLabels++;			// it is, so remember that we have modified a label
					oldLabel->value=value;	// re-assign new value
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Label '%s' already defined\n",parsedLabel->name);
				ReportLabelDefinitionLocation(oldLabel);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"Label '%s' already defined as a constant\n",parsedLabel->name);
			ReportLabelDefinitionLocation(oldLabel);
		}
	}
	else
	{
		if(!parsedLabel->isLocal)					// if not local, then update the current scope
		{
			strcpy(scope,parsedLabel->name);
		}
		if(!CreateLabel(parsedLabel->name,value,LF_LABEL,passCount,true))
		{
			fail=true;
		}
	}
	return(!fail);
}

bool AssignConstant(const char *name,int value,bool resolved)
// Assign a constant.
// return false if there is a hard error
{
	LABEL_RECORD
		*oldLabel;
	bool
		fail;

	fail=false;
	if((oldLabel=LocateLabel(name,false)))				// see if there is a definition for this already
	{
		switch(oldLabel->type)
		{
			case LF_EXT_CONST:
				if(oldLabel->resolved&&resolved)			// make sure both are resolved so the compare below is meaningful
				{
					if(oldLabel->value!=value)		// only complain if the value is changing
					{
						AssemblyComplaint(NULL,true,"Label '%s' already defined as constant of different value\n",name);
						ReportLabelDefinitionLocation(oldLabel);
					}
				}
				break;
			case LF_CONST:
				if(oldLabel->passCount<passCount)			// verify we have not seen this label so far this pass
				{
					oldLabel->passCount=passCount;			// update the passcount
					if(oldLabel->resolved)
					{
						if(resolved)							// was resolved and is resolved
						{
							if(oldLabel->value!=value)	// see if value is changing between passes
							{
								ReportDiagnostic("Modified label: '%s' (old=%08X, new=%08X)\n",name,oldLabel->value,value);
								numModifiedLabels++;			// it is, so remember that we have modified a label
								oldLabel->value=value;	// re-assign new value
							}
						}
						else
						{
// ### this should not happen
//							oldLabel->resolved=false;			// going from resolved to unresolved???
//							numModifiedLabels++;				// it is, so remember that we have modified a label
						}
					}
					else
					{
						if((oldLabel->resolved=resolved))		// resolve it now if we can
						{
							oldLabel->value=value;		// assign the current value
							numModifiedLabels++;				// remember it was messed with
						}
					}
				}
				else	// see same label being defined again in the current pass
				{
					if(oldLabel->resolved)
					{
						if(!resolved||(oldLabel->value!=value))	// only complain if the value is changing
						{
							AssemblyComplaint(NULL,true,"Label '%s' already defined\n",name);
							ReportLabelDefinitionLocation(oldLabel);
						}
					}
					else
					{
						oldLabel->resolved=resolved;		// resolve it now if we can
						oldLabel->value=value;		// assign the current value
						numModifiedLabels++;				// remember it was messed with
					}
				}
				break;
			case LF_SET:
				AssemblyComplaint(NULL,true,"Label '%s' already defined with 'set'\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			case LF_LABEL:
				AssemblyComplaint(NULL,true,"Label '%s' already defined as a label\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			default:
				break;
		}
	}
	else
	{
		if(!CreateLabel(name,value,LF_CONST,passCount,resolved))
		{
			fail=true;
		}
	}
	return(!fail);
}

void UnAssignSetConstant(const char *name)
// remove a constant that was assigned by AssignSetConstant
// return false if there is a hard error
{
	LABEL_RECORD
		*oldLabel;

	if((oldLabel=LocateLabel(name,false)))				// see if there is a definition for this already
	{
		switch(oldLabel->type)
		{
			case LF_EXT_CONST:
				AssemblyComplaint(NULL,true,"Label '%s' defined as constant cannot be unset\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			case LF_CONST:
				AssemblyComplaint(NULL,true,"Label '%s' defined with 'equ' cannot be unset\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			case LF_SET:
				DestroyLabel(oldLabel);
				break;
			case LF_LABEL:
				AssemblyComplaint(NULL,true,"Label '%s' defined as a label cannot be unset\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			default:
				break;
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Label '%s' not defined\n",name);
	}
}

bool AssignSetConstant(const char *name,int value,bool resolved)
// Assign a set constant (one which can be changed by other
// set instructions).
// return false if there is a hard error
{
	LABEL_RECORD
		*oldLabel;
	bool
		fail;

	fail=false;
	if((oldLabel=LocateLabel(name,false)))				// see if there is a definition for this already
	{
		switch(oldLabel->type)
		{
			case LF_EXT_CONST:
				AssemblyComplaint(NULL,true,"Label '%s' already defined as a constant\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			case LF_CONST:
				AssemblyComplaint(NULL,true,"Label '%s' already defined with 'equ'\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			case LF_SET:
				oldLabel->passCount=passCount;				// update the passcount
				oldLabel->value=value;				// update the value (do not update numModifiedLabels, since these are ALLOWED to change)
				oldLabel->resolved=resolved;				// remember if it is resolved or not
				break;
			case LF_LABEL:
				AssemblyComplaint(NULL,true,"Label '%s' already defined as a label\n",name);
				ReportLabelDefinitionLocation(oldLabel);
				break;
			default:
				break;
		}
	}
	else
	{
		if(!CreateLabel(name,value,LF_SET,passCount,resolved))
		{
			fail=true;
		}
	}
	return(!fail);
}

void UnInitLabels()
// undo what InitLabels did
{
	while(labelsHead)
	{
		DestroyLabel(labelsHead);							// get rid of all labels
	}
	STDisposeSymbolTable(labelSymbols);
}

bool InitLabels()
// initialize label table for assembler labels
{
	labelsHead=NULL;

	if((labelSymbols=STNewSymbolTable(0)))
	{
		return(true);
	}
	return(false);
}
