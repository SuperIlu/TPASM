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


#include	"include.h"

static int CompareLabels(const void *i,const void *j)
// compare the labels given by the entries i and j
// This is called by qsort
{
	LABEL_RECORD
		**recordA,
		**recordB;

	recordA=(LABEL_RECORD **)i;
	recordB=(LABEL_RECORD **)j;
	return(strcmp(STNodeName((*recordA)->symbol),STNodeName((*recordB)->symbol)));
}

static bool OutputSymbols(FILE *file)
// Dump symbol information in sunplus style
{
	unsigned int
		i;
	LABEL_RECORD
		*label;
	unsigned int	
		numLabels;
	LABEL_RECORD
		**sortArray;
	unsigned int
		moduleLength;

	numLabels=NumLabels();

	if((sortArray=(LABEL_RECORD **)NewPtr(sizeof(LABEL_RECORD *)*numLabels)))
	{
		label=labelsHead;
		for(i=0;i<numLabels;i++)		// make array of pointers to labels
		{
			sortArray[i]=label;
			label=label->next;
		}

		qsort(sortArray,numLabels,sizeof(LABEL_RECORD *),CompareLabels);	// sort the array (not that it matters here)


		fputc(0xfe,file);						// start of Microtek symbol file (0xfe)

		fputc(strlen(sourceFileName),file);		// length of source file name
		fprintf(file,"%s",sourceFileName);		// source file name

		moduleLength=2;								// figure out how big the module is (+2 for some reason)
		for(i=0;i<numLabels;i++)
		{
			label=sortArray[i];
			if(label->resolved)
			{
				moduleLength += 1 + strlen(STNodeName(label->symbol)) + 2;	// add length of this entry
			}
		}

		fputc((moduleLength>>16)&0xff,file);
		fputc((moduleLength>>8)&0xff,file);
		fputc(moduleLength&0xff,file);			// three-byte module length (big-endian byte order)
		fputc(0x02,file);						// size of symbol address (2 = 16 bit)

		for(i=0;i<numLabels;i++)					// make array of pointers to labels
		{
			label=sortArray[i];
			if(label->resolved)
			{
				fputc(strlen(STNodeName(label->symbol)),file);	// length of the symbol name
				fprintf(file,"%s",STNodeName(label->symbol));	// symbol name itself
				fputc(label->value&0xff,file);				// symbol value (2 bytes)
				fputc((label->value>>8)&0xff,file);
			}
		}

		fputc(0xfe,file);
		fputc(0xff,file);						// end of Microtek symbol file (0xff)

		DisposePtr(sortArray);
	}

	return(true);
}

// output file types handled here (the constuctors for these variables link them to the global
// list of output file types that the assembler knows how to handle)

static OUTPUTFILE_TYPE
	outputFileType("sunplus","Sunplus format symbol listing",true,OutputSymbols);
