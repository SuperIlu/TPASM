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

// This creates an entry that points to the function which already exists in
// the assembler to create textual symbol file listings

// output file types handled here (the constuctors for these variables link them to the global
// list of output file types that the assembler knows how to handle)

static int CompareLbl(const void *i,const void *j)
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

bool GetTextSymbols(FILE *file)
// Dump symbol information in a textual way to file
{
	LABEL_RECORD
		*label;
	unsigned int
		i,
		numLabels;
	LABEL_RECORD
		**sortArray;

	numLabels=NumLabels();
	if((sortArray=(LABEL_RECORD **)NewPtr(sizeof(LABEL_RECORD *)*numLabels)))
	{
		label=labelsHead;
		for(i=0;i<numLabels;i++)		// make array of pointers to labels
		{
			sortArray[i]=label;
			label=label->next;
		}

		qsort(sortArray,numLabels,sizeof(LABEL_RECORD *),CompareLbl);	// sort the array

		for(i=0;i<numLabels;i++)		// make array of pointers to labels
		{
			label=sortArray[i];
			if((label->resolved) && (strstr(STNodeName(label->symbol), "@") == NULL) && (strcmp(STNodeName(label->symbol), "__Z80")))
			{
				fprintf(file,"%s:    .EQU    $%04X\n",STNodeName(label->symbol), label->value);
			}
		}

		DisposePtr(sortArray);
	}
	return(true);
}

static OUTPUTFILE_TYPE
	outputFileType("incl","Textual symbol include file",false,GetTextSymbols);

