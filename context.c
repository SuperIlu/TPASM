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


// Handle assembly context management subroutines

#include	"include.h"

bool PushContextRecord(unsigned int contextBytes)
// Create a new context record, push it onto the stack
// If there was a problem, return false
{
	CONTEXT_RECORD
		*record;

	if((record=(CONTEXT_RECORD *)NewPtr(sizeof(CONTEXT_RECORD)+contextBytes)))
	{
		record->contextType=0;
		record->active=false;
		record->whereFrom.file=currentVirtualFile;
		record->whereFrom.fileLineNumber=currentVirtualFileLine;
		record->next=contextStack;
		record->flush=NULL;
		contextStack=record;
		return(true);
	}
	return(false);
}

void PopContextRecord(void)
// Pull a context record off the stack and destroy it
// NOTE: if the stack is empty, do nothing
{
	CONTEXT_RECORD
		*tempRecord;

	if(contextStack)
	{
		tempRecord=contextStack->next;
		DisposePtr(contextStack);
		contextStack=tempRecord;
	}
}

void FlushContextRecords()
// Get rid of any lingering if context records, report errors
{
	while(contextStack)
	{
		if(contextStack->next&&contextStack->next->active)		// only report if this context is in an active context
		{
			switch(contextStack->contextType)
			{
				case CT_ROOT:
					break;
				case CT_IF:
					AssemblyComplaint(&contextStack->whereFrom,true,"Unterminated conditional\n");
					break;
				case CT_SWITCH:
					AssemblyComplaint(&contextStack->whereFrom,true,"Unterminated switch\n");
					break;
				case CT_MACRO:
					AssemblyComplaint(&contextStack->whereFrom,true,"Unterminated macro\n");
					break;
				case CT_REPEAT:
					AssemblyComplaint(&contextStack->whereFrom,true,"Unterminated repeat\n");
					break;
				default:
					AssemblyComplaint(&contextStack->whereFrom,true,"Unterminated context\n");
					break;
			}
		}
		if(contextStack->flush)
		{
			contextStack->flush(contextStack);				// flush this
		}
		PopContextRecord();
	}
}
