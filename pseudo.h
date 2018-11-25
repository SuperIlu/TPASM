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

typedef bool PseudoHandlerFunction(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

struct PSEUDO_OPCODE
{
	const char
		*name;
	PseudoHandlerFunction
		*function;
};

bool AttemptGlobalPseudoOpcode(const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success);
void UnInitGlobalPseudoOpcodes();
bool InitGlobalPseudoOpcodes();
