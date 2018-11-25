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


typedef bool OutputFileGenerateFunction(FILE *file);

class OUTPUTFILE_TYPE
{
public:
	const char
		*name;
	const char
		*description;
	bool
		binary;

	SYM_TABLE_NODE
		*symbol;			// set up when the type is added to the list of types

	OutputFileGenerateFunction
		*outputFileGenerateFunction;

	OUTPUTFILE_TYPE
		*previousType,		// used to link the list of symbol file output objects together
		*nextType;

	OUTPUTFILE_TYPE(const char *formatName,const char *formatDescription,bool isBinary,OutputFileGenerateFunction *generateFunction);
	~OUTPUTFILE_TYPE();
};

bool DumpOutputFiles();
bool SelectOutputFileType(char *typeName,char *outputName);
void UnInitOutputFileGenerate();
bool InitOutputFileGenerate();
void DumpOutputFileTypeInformation(FILE *file);
