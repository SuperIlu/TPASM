//	Copyright (C) 2000-2012 Core Technologies.
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

// Dump segments out in Intel hex format

#include	"include.h"

#define DATARECORD			0
#define ENDRECORD			1
#define EXTADDRESS			2	// gets an extra 4 bits of address (idiotic segment concept)
#define EXTLINEARADDRESS	4	// gets an extra 16 bits of address

static unsigned int
	lastAddressHigh;			// keeps track of the upper 16 bits of address when dumping intel hex records


static void DumpGenericLine(FILE *file,unsigned int address,unsigned char recordType,unsigned char *bytes,unsigned int numBytes)
// Create a line of intel hex output to file
{
	unsigned int
		checkSum;
	unsigned int
		i;

	fprintf(file,":%02X%04X%02X",numBytes,address&0xFFFF,recordType);
	checkSum=0;
	for(i=0;i<numBytes;i++)
	{
		checkSum+=bytes[i];
		fprintf(file,"%02X",bytes[i]);
	}
	checkSum+=(address&0xFF)+((address>>8)&0xFF)+numBytes+recordType;
	fprintf(file,"%02X\n",(-checkSum)&0xFF);
}

static void DumpLine(FILE *file,unsigned int address,unsigned char *bytes,unsigned int numBytes)
// Create a line of intel hex output to file
{
	DumpGenericLine(file,address,DATARECORD,bytes,numBytes);
}

void DumpLinearAddress(FILE *file,unsigned int address)
// dump out a linear address extension record for address
{
	unsigned char
		extendedAddressBuffer[2];

	extendedAddressBuffer[0]=address>>24;
	extendedAddressBuffer[1]=address>>16;
	DumpGenericLine(file,0,EXTLINEARADDRESS,extendedAddressBuffer,2);
}

void DumpEOR(FILE *file)
// Dump an end of record marker out to file
{
	DumpGenericLine(file,0,ENDRECORD,NULL,0);
}

static void DumpSegment(FILE *file,SEGMENT_RECORD *segment)
// create a hex dump of segment to file
// NOTE: this is careful to dump only the bytes which are marked as used
// within segment
// if segment is marked as not generating output, nothing will be dumped
{
	CODE_PAGE
		*page;
	unsigned int
		i,j,k;

	if(segment->generateOutput)
	{
		page=segment->firstPage;
		while(page)
		{
			if((page->address&0xFFFF0000)!=lastAddressHigh)
			{
				lastAddressHigh=page->address&0xFFFF0000;		// remember the last high address bits
				DumpLinearAddress(file,lastAddressHigh);	// spit out upper address bits
			}
			for(i=0;i<256;i+=16)
			{
				j=0;
				while(j<16)
				{
					while(j<16&&!(page->usageMap[(i+j)>>3]&(1<<(j&7))))	// skip over empty space
					{
						j++;
					}
					if(j<16)
					{
						k=j;
						while(k<16&&(page->usageMap[(i+k)>>3]&(1<<(k&7))))	// count up used space
						{
							k++;
						}
						DumpLine(file,page->address+i+j,&page->pageData[i+j],k-j);	// dump out used bytes
						j=k;												// move forward
					}
				}
			}
			page=page->next;
		}
	}
}

static bool OutputSegments(FILE *file)
// create a hex dump of all segments to file
// NOTE: this is careful to dump only the bytes which are marked as used
// within each segment
// Segments marked as not generating output will not be dumped
// NOTE: Segments will be dumped in reverse order of creation
// this should probably be changed to dump in the order of creation
{
	SEGMENT_RECORD
		*segment;

	lastAddressHigh=0;				// initialize upper address bits
	segment=segmentsHead;
	while(segment)
	{
		DumpSegment(file,segment);
		segment=segment->next;
	}
	DumpEOR(file);
	return(true);
}

// output file types handled here (the constuctors for these variables link them to the global
// list of output file types that the assembler knows how to handle)

static OUTPUTFILE_TYPE
	outputFileType("intel","Intel format segment dump",false,OutputSegments);
