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


void CloseTextOutputFile(FILE *file);
FILE *OpenTextOutputFile(const char *name);
void CloseBinaryOutputFile(FILE *file);
FILE *OpenBinaryOutputFile(const char *name);
void CloseSourceFile(FILE *file);
FILE *OpenSourceFile(const char *name,bool huntForIt,SYM_TABLE_NODE **fileNameSymbol);
bool AddIncludePath(const char *pathName);
void UnInitFiles();
bool InitFiles();
