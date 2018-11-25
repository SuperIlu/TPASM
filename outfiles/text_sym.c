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

static OUTPUTFILE_TYPE
	outputFileType("text","Textual symbol file listing",false,OutputTextSymbols);
