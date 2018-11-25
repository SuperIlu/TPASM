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


void ReportDiagnostic(const char *format,...);
void ReportComplaint(bool isError,const char *format,...);
void AssemblyComplaint(WHERE_FROM *whereFrom,bool isError,const char *format,...);
void AssemblySupplement(WHERE_FROM *whereFrom,const char *format,...);
void CreateListStringValue(LISTING_RECORD *listingRecord,int value,bool unresolved);
void OutputListFileHeader(time_t timeVal);
void OutputListFileLine(LISTING_RECORD *listingRecord,const char *sourceLine);
void OutputListFileStats(unsigned int totalTime);
void OutputListFileSegments();
bool OutputTextSymbols(FILE *file);
void OutputListFileLabels();
