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

static void vReportDiagnostic(const char *format,va_list args)
// Display diagnostic messages
{
	if(displayDiagnostics)
	{
		vfprintf(stderr,format,args);
	}
}

void ReportDiagnostic(const char *format,...)
// Display diagnostic messages
{
	va_list
		args;

	va_start(args,format);
	vReportDiagnostic(format,args);				// do actual reporting
	va_end(args);
}

static void vReportMessage(const char *fileName,unsigned int lineNumber,const char *type,const char *format,va_list args)
// report messages (errors, warnings, etc...)
// type is a string which is descriptive of the message type
// This will report to the stderr.
// NOTE: this will report during any pass of assembly
{
	unsigned int
		length;

	if(fileName)
	{
		length=strlen(fileName);
		if(length<16)			// work out how much to space over to the error message
		{
			length=16-length;
		}
		else
		{
			length=0;
		}
		fprintf(stderr,"%s:%-6d%*s:",fileName,lineNumber,length,"");
	}
	else
	{
		fprintf(stderr,"<command line>         :");
	}
	fprintf(stderr,"%s: ",type);
	vfprintf(stderr,format,args);
}

static void ReportMessage(const char *fileName,unsigned int lineNumber,const char *type,const char *format,...)
// report messages (errors, warnings, etc...)
// type is a string which is descriptive of the message type
// This will report to the stderr.
// NOTE: this will report during any pass of assembly
{
	va_list
		args;

	va_start(args,format);
	vReportMessage(fileName,lineNumber,type,format,args);
	va_end(args);
}

static void ReportVirtualPosition()
// If the current file location and virtual position do not match up, report
// the virtual position
{
	if(currentVirtualFile&&(currentFile!=currentVirtualFile||currentFileLine!=currentVirtualFileLine))
	{
		ReportMessage(STNodeName(currentVirtualFile),currentVirtualFileLine,"^      ","Previous message generated indirectly from here\n");
	}
}

static void vReportComplaint(WHERE_FROM *whereFrom,bool isError,const char *format,va_list args)
// This will report to the stderr.
// NOTE: this will report during any pass of assembly
// NOTE: whereFrom can be passed as NULL, in which case, the current
// location will be reported
// if isError is true, the comlpaint will be reported as an error,
// otherwise as a warning
{
	if(isError||displayWarnings)
	{
		if(whereFrom)
		{
			vReportMessage(whereFrom->file?STNodeName(whereFrom->file):NULL,whereFrom->fileLineNumber,isError?"error  ":"warning",format,args);
		}
		else
		{
			vReportMessage(currentFile?STNodeName(currentFile):NULL,currentFileLine,isError?"error  ":"warning",format,args);
			ReportVirtualPosition();
		}
		if(listFile&&outputListing)
		{
			if(isError)
			{
				fprintf(listFile,"**** ERROR:   ");
			}
			else
			{
				fprintf(listFile,"**** WARNING: ");
			}
			vfprintf(listFile,format,args);
		}
		if(isError)
		{
			errorCount++;
		}
		else
		{
			warningCount++;
		}
	}
}

static void vReportSupplement(WHERE_FROM *whereFrom,const char *format,va_list args)
// This will report to the stderr.
// NOTE: this will report during any pass of assembly
// NOTE: whereFrom can be passed as NULL, in which case, the current
// location will be reported
{
	if(whereFrom)
	{
		vReportMessage(whereFrom->file?STNodeName(whereFrom->file):NULL,whereFrom->fileLineNumber,"       ",format,args);
	}
	else
	{
		vReportMessage(currentFile?STNodeName(currentFile):NULL,currentFileLine,"       ",format,args);
		ReportVirtualPosition();
	}
}

void ReportComplaint(bool isError,const char *format,...)
// if a hard error (failed to allocate memory, or open file, etc), or warning occurs while
// assembling, report it immediately with this function
// This will report to the stderr.
// NOTE: this will report during any pass of assembly
{
	va_list
		args;

	va_start(args,format);
	vReportComplaint(NULL,isError,format,args);
	va_end(args);
}

void AssemblyComplaint(WHERE_FROM *whereFrom,bool isError,const char *format,...)
// Report assembly errors or warnings
// This will report to the stderr, as well as to the listing file if there is one
// NOTE: this only reports during the final pass of assembly
{
	va_list
		args;

	if(!intermediatePass)
	{
		va_start(args,format);
		vReportComplaint(whereFrom,isError,format,args);
		va_end(args);
	}
}

void AssemblySupplement(WHERE_FROM *whereFrom,const char *format,...)
// Report supplementary information about an error or warning
// This will report to the stderr, as well as to the listing file if there is one
// NOTE: this only reports during the final pass of assembly
{
	va_list
		args;

	if(!intermediatePass)
	{
		va_start(args,format);
		vReportSupplement(whereFrom,format,args);
		va_end(args);
	}
}

void CreateListStringValue(LISTING_RECORD *listingRecord,int value,bool unresolved)
// Create the listing output string for a value
{
	if(!unresolved)
	{
		sprintf(listingRecord->listObjectString,"(%08X)",value);
	}
	else
	{
		sprintf(listingRecord->listObjectString,"(???????\?)");	// see that '\'?? It's there because of the brilliant individual who invented trigraphs
	}
}

void OutputListFileHeader(time_t timeVal)
// Dump the header information to the list file
{
	if(listFile)
	{
		timeVal=time(NULL);
		fprintf(listFile,"tpasm %s		Assembling on %s",VERSION,ctime(&timeVal));
		fprintf(listFile,"\n");
		fprintf(listFile,"Source File: %s\n",sourceFileName);
		fprintf(listFile,"\n");
		fprintf(listFile,"Line  Loc      Object/(Value) T	Source\n");
		fprintf(listFile,"----- -------- -------------- -	------\n");
	}
}

static void GetNextWrapIndex(char *listObjectString,unsigned int *startIndex,unsigned int *endIndex)
// look through the listObjectString, try to find good wrap points, return them
// NOTE: startIndex is passed in as the place to start working
// both startIndex and endIndex are modified
// startIndex will point to the end of the string when there is nothing
// more to print
{
	unsigned int
		bestEndIndex;

	while(listObjectString[*startIndex]&&listObjectString[*startIndex]==' ')	// move through all the white at the start
	{
		(*startIndex)++;
	}
	bestEndIndex=(*endIndex)=(*startIndex);
	while(listObjectString[*endIndex]&&((*endIndex)-(*startIndex))<14)
	{
		if(listObjectString[*endIndex]==' ')
		{
			bestEndIndex=*endIndex;
		}
		(*endIndex)++;
	}
	if(listObjectString[*endIndex]==' ')									// check for space at the very end
	{
		bestEndIndex=*endIndex;
	}
	if(bestEndIndex==(*startIndex)||!listObjectString[*endIndex])			// if nothing good found, or at end, then set best to the end
	{
		bestEndIndex=(*endIndex);
	}
	(*endIndex)=bestEndIndex;												// this is the end
}

void OutputListFileLine(LISTING_RECORD *listingRecord,const char *sourceLine)
// Output the listing line
// NOTE: this may generate more than one line if listingRecord->listObjectString is long
// the string will be wrapped (preferably on space boundaries) and written on
// subsequent lines
{
	unsigned int
		startIndex,
		endIndex;

	if(!intermediatePass&&listFile&&outputListing&&listingRecord->wantList)
	{
		startIndex=0;
		GetNextWrapIndex(listingRecord->listObjectString,&startIndex,&endIndex);
		fprintf(listFile,"%-5d %08X %-14.*s %c	%s\n",listingRecord->lineNumber,listingRecord->listPC,endIndex-startIndex,&listingRecord->listObjectString[startIndex],listingRecord->sourceType,sourceLine);

		startIndex=endIndex;
		GetNextWrapIndex(listingRecord->listObjectString,&startIndex,&endIndex);			// move to the next spot
		while(listingRecord->listObjectString[startIndex])									// if more to print, then do it
		{
			fprintf(listFile,"               %-14.*s %c\n",endIndex-startIndex,&listingRecord->listObjectString[startIndex],listingRecord->sourceType);
			startIndex=endIndex;
			GetNextWrapIndex(listingRecord->listObjectString,&startIndex,&endIndex);
		}
	}
}

void OutputListFileStats(unsigned int totalTime)
// Dump stats information to the list file
{
	unsigned int
		hours,minutes,seconds;

	hours=totalTime/3600;
	totalTime-=(hours*3600);
	minutes=totalTime/60;
	totalTime-=(minutes*60);
	seconds=totalTime;
	ReportDiagnostic("\n");
	ReportDiagnostic("Assembled in %d passes\n",passCount+1);
	ReportDiagnostic("\n");
	ReportDiagnostic("Total assembly time %02d:%02d:%02d\n",hours,minutes,seconds);
	ReportDiagnostic("Total Errors:   %d\n",errorCount);
	ReportDiagnostic("Total Warnings: %d\n",warningCount);

	if(listFile)
	{
		fprintf(listFile,"\n");
		fprintf(listFile,"Assembled in %d passes\n",passCount+1);
		fprintf(listFile,"\n");
		fprintf(listFile,"Total assembly time %02d:%02d:%02d\n",hours,minutes,seconds);
		fprintf(listFile,"Total Errors:   %d\n",errorCount);
		fprintf(listFile,"Total Warnings: %d\n",warningCount);
	}
}

void OutputListFileSegments()
// Dump segment information to the list file
{
	if(listFile)
	{
		fprintf(listFile,"\n");
		DumpSegmentsListing(listFile);
	}
}

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

bool OutputTextSymbols(FILE *file)
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

		qsort(sortArray,numLabels,sizeof(LABEL_RECORD *),CompareLabels);	// sort the array

		fprintf(file,"Symbol Table Listing\n");
		fprintf(file,"Value    U Name\n");
		fprintf(file,"-------- - ----\n");

		for(i=0;i<numLabels;i++)		// make array of pointers to labels
		{
			label=sortArray[i];
			if(label->resolved)
			{
				fprintf(file,"%08X %c %s\n",label->value,(label->refCount==0)?'*':' ',STNodeName(label->symbol));
			}
			else
			{
				fprintf(file,"????????   %s\n",STNodeName(label->symbol));
			}
		}

		DisposePtr(sortArray);
	}
	return(true);
}

void OutputListFileLabels()
// Dump symbol information to the list file
{
	if(listFile)
	{
		fprintf(listFile,"\n");
		OutputTextSymbols(listFile);
	}
}
