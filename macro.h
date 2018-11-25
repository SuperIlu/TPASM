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


void DestroyTextBlockLines(TEXT_BLOCK *block);
bool AddLineToTextBlock(TEXT_BLOCK *block,const char *line);
bool CreateParameterList(const char *line,unsigned int *lineIndex,TEXT_BLOCK *block);
bool CreateParameterNames(const char *line,unsigned int *lineIndex,TEXT_BLOCK *block);
MACRO_RECORD *LocateMacro(const char *name);
bool AttemptMacro(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *result);
void DestroyMacro(MACRO_RECORD *macro);
void DestroyMacros();
MACRO_RECORD *CreateMacro(char *macroName);
void UnInitMacros();
bool InitMacros();
