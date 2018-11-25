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

// Dump segments out in Motorola S-Record format

#include	"include.h"

static int
	dataRecordType,
	endRecordType;

static void DumpGenericLine(FILE *file,unsigned int address,unsigned char recordType,unsigned char *bytes,unsigned int numBytes)
// Create a line of motorola hex output to file
{
	unsigned int
		checkSum;
	unsigned int
		i;

	checkSum=0;
	switch(recordType)
	{
		case 0:		// all these types have 2 byte addresses
		case 1:
		case 4:
		case 5:
		case 6:
		case 9:
			fprintf(file,"S%d%02X%04X",recordType,numBytes+3,address&0xFFFF);
			checkSum+=numBytes+3;
			checkSum+=address&0xFF;
			checkSum+=(address>>8)&0xFF;
			break;
		case 2:		// these have 3 byte addresses
		case 8:
			fprintf(file,"S%d%02X%06X",recordType,numBytes+4,address&0xFFFFFF);
			checkSum+=numBytes+4;
			checkSum+=address&0xFF;
			checkSum+=(address>>8)&0xFF;
			checkSum+=(address>>16)&0xFF;
			break;
		case 3:		// these have 4 byte addresses
		case 7:
			fprintf(file,"S%d%02X%08X",recordType,numBytes+5,address);
			checkSum+=numBytes+5;
			checkSum+=address&0xFF;
			checkSum+=(address>>8)&0xFF;
			checkSum+=(address>>16)&0xFF;
			checkSum+=(address>>24)&0xFF;
			break;
	}
	for(i=0;i<numBytes;i++)
	{
		checkSum+=bytes[i];
		fprintf(file,"%02X",bytes[i]);
	}
	fprintf(file,"%02X\n",(~checkSum)&0xFF);
}

static void DumpLine(FILE *file,unsigned int address,unsigned char *bytes,unsigned int numBytes)
// Create a line of motorola hex output to file
{
	DumpGenericLine(file,address,dataRecordType,bytes,numBytes);
}

static void DumpEOF(FILE *file)
// Send an EOF record out
{
	DumpGenericLine(file,0,endRecordType,NULL,0);
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

	segment=segmentsHead;
	while(segment)
	{
		DumpSegment(file,segment);
		segment=segment->next;
	}
	DumpEOF(file);
	return(true);
}

static bool OutputSegments16(FILE *file)
// output segments with S1 records (16 bit addresses)
{
	dataRecordType=1;
	endRecordType=9;
	return(OutputSegments(file));
}

static bool OutputSegments32(FILE *file)
// output segments with S3 records (32 bit addresses)
{
	dataRecordType=3;
	endRecordType=7;
	return(OutputSegments(file));
}

// output file types handled here (the constuctors for these variables link them to the global
// list of output file types that the assembler knows how to handle)

static OUTPUTFILE_TYPE
	outputFileType32("srec32","Motorola S-Record format segment dump (32 bit)",false,OutputSegments32),
	outputFileType16("srec","Motorola S-Record format segment dump (16 bit)",false,OutputSegments16);
