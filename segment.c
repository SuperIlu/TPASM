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


// Handle creation and manipulation of segments

#include	"include.h"

static SYM_TABLE
	*segmentSymbols;											// segment symbol list is kept here

static CODE_PAGE *FindCodePage(SEGMENT_RECORD *segment,unsigned int address)
// find the code page of address, or the one immediately before it in the linked
// list of code pages hanging off segment
// NOTE: if the address is earlier than any code page within segment,
// NULL will be returned
// NOTE: this should probably be made faster
{
	CODE_PAGE
		*currentPage,
		*bestPage;

	bestPage=NULL;
	currentPage=segment->firstPage;

	if(segment->pageCache&&segment->pageCache->address<=address)
	{
		currentPage=segment->firstPage;
	}

	while(currentPage&&currentPage->address<=address)
	{
		bestPage=currentPage;
		currentPage=currentPage->next;
	}
	segment->pageCache=bestPage;
	return(bestPage);
}

static void DestroyCodePage(SEGMENT_RECORD *segment,CODE_PAGE *page)
// unlink page from segment, and destroy it
{
	if(page->next)
	{
		page->next->previous=page->previous;
	}
	if(page->previous)
	{
		page->previous->next=page->next;
	}
	else
	{
		segment->firstPage=page->next;
	}
	if(segment->pageCache==page)			// update the cache if needed
	{
		segment->pageCache=page->previous;
	}
	DisposePtr(page);
}

static CODE_PAGE *CreateCodePage(SEGMENT_RECORD *segment,CODE_PAGE *pageBefore)
// create a new code page
// link it to segment after pageBefore
// NOTE: pageBefore can be passed in as NULL to link it to the start of the
// segment
// NOTE: the code page's usage map is initialized to be completely unused
// NOTE: if this fails, it will return NULL
{
	CODE_PAGE
		*page;
	int
		i;

	if((page=(CODE_PAGE *)NewPtr(sizeof(CODE_PAGE))))
	{
		for(i=0;i<32;i++)
		{
			page->usageMap[i]=0;
		}
		if(pageBefore)
		{
			page->next=pageBefore->next;
			page->previous=pageBefore;
			pageBefore->next=page;
		}
		else
		{
			if((page->next=segment->firstPage))
			{
				page->next->previous=page;
			}
			page->previous=NULL;
			segment->firstPage=page;
		}
		return(page);
	}
	return(NULL);
}

bool AddBytesToSegment(SEGMENT_RECORD *segment,unsigned int address,unsigned char *bytes,unsigned int numBytes)
// add bytes to segment at address
// if there is a problem, report it, and return false
// NOTE: this manages creating code pages, and linking them to segment
// in the proper location.
{
	CODE_PAGE
		*page;
	unsigned int
		baseAddress;
	unsigned int
		index;
	bool
		fail;

	fail=false;
	while(numBytes&&!fail)
	{
		page=FindCodePage(segment,address);		// get the nearest page to the one we want to write into
		baseAddress=address&~0xFF;						// mask off low part of address
		if(!page||(page->address!=baseAddress))	// see if a new page needs to be created
		{
			if((page=CreateCodePage(segment,page)))
			{
				page->address=baseAddress;			// set up the base address for this page
			}
			else
			{
				ReportComplaint(true,"Failed to create code page\n");
				fail=true;
			}
		}
		if(!fail)										// make sure there's a good page to use
		{
			index=address&0xFF;
			while(numBytes&&(index<0x100))				// copy bytes into the page
			{
				if(page->usageMap[index>>3]&(1<<(index&7)))
				{
					AssemblyComplaint(NULL,false,"Overwriting address 0x%08X in segment '%s'\n",address,STNodeName(segment->symbol));
				}
				page->usageMap[index>>3]|=(1<<(index&7));	// update the usage map
				page->pageData[index]=*bytes;		// drop the bytes into the page
				bytes++;
				index++;
				address++;
				numBytes--;
			}
		}
	}
	return(!fail);
}

bool AddSpaceToSegment(SEGMENT_RECORD *segment,unsigned int address,unsigned int numBytes)
// add bytes of empty space to segment at address
// Call this in response to a DS (define space) command
{
	if(segment->sawDS)
	{
		if(segment->minDS>address)
		{
			segment->minDS=address;
		}
		if(segment->maxDS<address+numBytes-1)
		{
			segment->maxDS=address+numBytes-1;
		}
	}
	else
	{
		if(numBytes)
		{
			segment->sawDS=true;
			segment->minDS=address;
			segment->maxDS=address+numBytes-1;
		}
	}
	return(true);
}

SEGMENT_RECORD *MatchSegment(const char *segment)
// try to match segment against the list of segments which currently exist
{
	void
		*resultValue;

	if((resultValue=STFindDataForName(segmentSymbols,segment)))
	{
		return((SEGMENT_RECORD *)resultValue);
	}
	return(NULL);
}

void DestroySegment(SEGMENT_RECORD *segment)
// remove segment from existence
{
	while(segment->firstPage)				// get rid of code page list
	{
		DestroyCodePage(segment,segment->firstPage);
	}

	STRemoveEntry(segmentSymbols,segment->symbol);

	if(segment->next)
	{
		segment->next->previous=segment->previous;
	}
	else
	{
		segmentsTail=segment->previous;
	}

	if(segment->previous)
	{
		segment->previous->next=segment->next;
	}
	else
	{
		segmentsHead=segment->next;
	}
	DisposePtr(segment);
}

void DestroySegments()
// remove all segments
{
	while(segmentsHead)
	{
		DestroySegment(segmentsHead);
	}
}

SEGMENT_RECORD *CreateSegment(const char *segmentName,bool generateOutput)
// Create a segment record, link it into the end of the global list,
// create a symbol table entry for it
{
	SEGMENT_RECORD
		*record;

	if((record=(SEGMENT_RECORD *)NewPtr(sizeof(SEGMENT_RECORD))))
	{
		record->generateOutput=generateOutput;
		record->pageCache=NULL;
		record->firstPage=NULL;
		record->currentPC=0;
		record->codeGenOffset=0;

		record->sawDS=false;
		record->minDS=0;
		record->maxDS=0;

		if((record->symbol=STAddEntryAtEnd(segmentSymbols,segmentName,record)))
		{
			if((record->previous=segmentsTail))
			{
				segmentsTail->next=record;
			}
			else
			{
				segmentsHead=record;
			}
			segmentsTail=record;		// this record is the new tail
			record->next=NULL;		// no next for this one
			return(record);
		}
		DisposePtr(record);
	}
	return(NULL);
}

static void GetSegmentMinMax(SEGMENT_RECORD *segment,unsigned int *minAddress,unsigned int *maxAddress)
// Work out the minimum and maximum addresses occupied by segment
{
	CODE_PAGE
		*page;
	int
		i;

	*minAddress=0;
	*maxAddress=0;

	if((page=segment->firstPage))
	{
		i=0;
		while(i<256&&(page->usageMap[i>>3]&(1<<(i&7)))==0)
		{
			i++;
		}
		*minAddress=page->address+i;

		while(page->next)
		{
			page=page->next;
		}

		i=255;
		while(i>0&&(page->usageMap[i>>3]&(1<<(i&7)))==0)
		{
			i--;
		}
		*maxAddress=page->address+i;
	}

	if(segment->sawDS)
	{
		if((!segment->firstPage)||(segment->minDS<*minAddress))
		{
			*minAddress=segment->minDS;
		}
		if((!segment->firstPage)||(segment->maxDS>*maxAddress))
		{
			*maxAddress=segment->maxDS;
		}
	}
}

static void DumpSegmentListing(FILE *file,SEGMENT_RECORD *segment)
// Create a listing of the given segment, and
// what memory ranges it spanned
{
	unsigned int
		minAddress,
		maxAddress;

	GetSegmentMinMax(segment,&minAddress,&maxAddress);
	fprintf(file,"%08X  %08X  %s\n",minAddress,maxAddress,STNodeName(segment->symbol));
}

void DumpSegmentsListing(FILE *file)
// Create a listing of which segments were specified, and
// what memory ranges they spanned
{
	SEGMENT_RECORD
		*segment;

	fprintf(file,"Segment Listing\n");
	fprintf(file,"MinAddr   MaxAddr   Segment\n");
	fprintf(file,"--------  --------  -------\n");
	segment=segmentsHead;
	while(segment)
	{
		DumpSegmentListing(file,segment);
		segment=segment->next;
	}
}

void UnInitSegments()
// undo what InitSegments did
{
	STDisposeSymbolTable(segmentSymbols);
}

bool InitSegments()
// initialize symbol table for segments
{
	if((segmentSymbols=STNewSymbolTable(100)))
	{
		return(true);
	}
	return(false);
}
