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


extern unsigned int
	numAllocatedPointers;
extern const char
	*programName;
extern const char
	*sourceFileName;
extern const char
	*listFileName;
extern FILE
	*listFile;
extern const char
	*defaultProcessorName;
extern bool
	infoOnly;

extern unsigned int
	includeDepth;
extern unsigned int
	passCount;
extern unsigned int
	maxPasses;
extern unsigned int
	numUnresolvedLabels,
	numModifiedLabels;
extern bool
	intermediatePass;
extern unsigned int
	numBytesGenerated;
extern char
	scope[MAX_STRING];
extern unsigned int
	scopeCount;
extern unsigned int
	scopeValue;

extern unsigned int
	blockDepth;

extern CONTEXT_RECORD
	*contextStack;

extern TEXT_BLOCK
	*collectingBlock;

extern ALIAS_RECORD
	*aliasesHead;

extern MACRO_RECORD
	*macrosHead;

extern SYM_TABLE
	*fileNameSymbols;

extern SYM_TABLE_NODE
	*currentFile,
	*currentVirtualFile;

extern LABEL_RECORD
	*labelsHead;

extern unsigned int
	currentFileLine;
extern unsigned int
	currentVirtualFileLine;

extern bool
	strictPseudo;

extern unsigned int
	errorCount,
	warningCount;
extern bool
	displayWarnings,
	displayDiagnostics;

extern bool
	stopParsing;
extern bool
	outputListing;
extern bool
	outputListingExpansions;

extern SEGMENT_RECORD
	*currentSegment,
	*segmentsHead,
	*segmentsTail;
