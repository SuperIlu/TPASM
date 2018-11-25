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


bool ProcessLineLocationLabel(const PARSED_LABEL *parsedLabel);
bool ProcessTextBlock(TEXT_BLOCK *block,TEXT_BLOCK *substitutionList,TEXT_BLOCK *substitutionText,char sourceType);
bool ProcessSourceFile(const char *fileName,bool huntForIt);
int main(int argc,char *argv[]);
