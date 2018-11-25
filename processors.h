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


typedef bool InitFamilyFunction();
typedef void UnInitFamilyFunction();
typedef bool SelectProcessorFunction(class PROCESSOR *processor);
typedef void DeselectProcessorFunction(class PROCESSOR *processor);
typedef bool AttemptProcessorPseudoOpcodeFunction(const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success);
typedef bool AttemptProcessorOpcodeFunction(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success);

class PROCESSOR_FAMILY
{
public:
	const char
		*name;

	InitFamilyFunction
		*initFamilyFunction;
	UnInitFamilyFunction
		*uninitFamilyFunction;

	SelectProcessorFunction
		*selectProcessorFunction;
	DeselectProcessorFunction
		*deselectProcessorFunction;

	AttemptProcessorPseudoOpcodeFunction
		*attemptPseudoOpcodeFunction;

	AttemptProcessorOpcodeFunction
		*attemptOpcodeFunction;

	class PROCESSOR
		*firstProcessor,		// keeps list of processors in this family
		*lastProcessor;

	PROCESSOR_FAMILY
		*previousFamily,		// used to link the list of families together
		*nextFamily;

	PROCESSOR_FAMILY(const char *processorName,InitFamilyFunction *initFamily,UnInitFamilyFunction *unInitFamily,SelectProcessorFunction *selectProcessor,DeselectProcessorFunction *deselectProcessor,AttemptProcessorPseudoOpcodeFunction *attemptPseudoOpcode,AttemptProcessorOpcodeFunction *attemptOpcode);
	~PROCESSOR_FAMILY();
};

class PROCESSOR
{
public:
	PROCESSOR_FAMILY
		*family;
	SYM_TABLE_NODE
		*symbol;				// set up when processor is added to symbol table
	const char
		*name;
	const void
		*processorData;			// local data that each processor type knows about

	PROCESSOR
		*previousProcessor,		// links processors to their families
		*nextProcessor;

	PROCESSOR(PROCESSOR_FAMILY *processorFamily,const char *familyName,const void *familyData);
	~PROCESSOR();
};

bool AttemptProcessorPseudoOpcode(const char *line,unsigned int *lineIndex,PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success);
bool AttemptProcessorOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success);
bool SelectProcessor(const char *processorName,bool *found);
void UnInitProcessors();
bool InitProcessors();
void DumpProcessorInformation(FILE *file);
