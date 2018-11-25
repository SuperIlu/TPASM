//-----------------------------------------------------------------------------
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
//
//-----------------------------------------------------------------------------

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols,
	*regNameSymbols;				// register names

// enumerated addressing modes

enum
{
	OT_IMPLIED=0,					// no operands
	OT_IMMEDIATE8,					// one byte immediate operand
	OT_IMMEDIATE16,					// two byte immediate operand
	OT_DIRECT,						// one byte direct
	OT_EXTENDED,					// two byte absolute
	OT_RELATIVE8,					// one byte relative offset
	OT_RELATIVE16,					// two byte relative offset
	OT_INDEXED,						// indexed (many forms)
	OT_REGREG,						// 2 8 or 16 bit registers
	OT_STACKS,						// stacked register list for S
	OT_STACKU,						// stacked register list for U
	OT_NUM							// number of addressing modes
};


// masks for the various addressing modes

#define	M_IMPLIED 					(1<<OT_IMPLIED)
#define	M_IMMEDIATE8				(1<<OT_IMMEDIATE8)
#define	M_IMMEDIATE16				(1<<OT_IMMEDIATE16)
#define	M_DIRECT					(1<<OT_DIRECT)
#define	M_EXTENDED					(1<<OT_EXTENDED)
#define	M_RELATIVE8					(1<<OT_RELATIVE8)
#define	M_RELATIVE16				(1<<OT_RELATIVE16)
#define	M_INDEXED					(1<<OT_INDEXED)
#define	M_REGREG					(1<<OT_REGREG)
#define	M_STACKS					(1<<OT_STACKS)
#define	M_STACKU					(1<<OT_STACKU)


// register definitions
struct REGDEF
{
	const char
		*name;
	bool
		is16Bit;
	unsigned char
		exgIndex,		// bit field when used in EXG/TFR instruction
		stackMask,		// mask when used in stack instruction
		indexMask;		// mask used when register is used for indexed addressing
};

enum					// register enumerations (must line up as indices with list below)
{
	R_D=0,
	R_X,
	R_Y,
	R_U,
	R_S,
	R_PC,
	R_PCR,
	R_A,
	R_B,
	R_CC,
	R_DP,
	NUM_REGDEFS
};

static REGDEF
	RegDefs[NUM_REGDEFS]=
	{
		{"d",true,0x00,0x06,0x0B},		// push both A and B
		{"x",true,0x01,0x10,0x00},
		{"y",true,0x02,0x20,0x20},
		{"u",true,0x03,0x40,0x40},		// S and U are interchanged depending on which stack things are being pushed/pulled
		{"s",true,0x04,0x40,0x60},
		{"pc",true,0x05,0x80,0x0C},
		{"pcr",true,0x05,0x80,0x0C},	// PC is also sometimes referred to this way
		{"a",false,0x08,0x02,0x06},
		{"b",false,0x09,0x04,0x05},
		{"cc",false,0x0A,0x01,0x00},
		{"dp",false,0x0B,0x08,0x00},
	};

struct INDEXEDADDRESS
{
	bool
		indirect;					// if true, indexed address is indirect, otherwise its direct
	bool
		offsetIsConstant;			// if true, offset is constant value, otherwise it is a register
	int
		constantOffset;				// if offset is constant value, this is the value
	bool
		constantOffsetUnresolved;	// tells if the constant offset has been resolved
	unsigned int
		offsetRegister;				// if offset is register, this tells which one
	int
		incdecCount;				// 0 for no predecrement/postIncrement, -1 for predec, -2 for 2*predec, 1 for postinc, 2 for 2*postinc
	bool
		haveIndex;					// if true, an index register was specified, false if none specified
	unsigned int
		indexRegister;				// register enumeration which tells the index register
};


#define	MAXREGLISTSIZE	64			// number of registers allowed in a list

struct REGLIST
{
	unsigned char
		numRegs;					// tells how many registers are in the list
	unsigned char
		registers[MAXREGLISTSIZE];	// the list of registers
};


typedef unsigned short opcode_t;

struct OPCODE
{
	const char
		*name;
	unsigned int
		typeMask;
	opcode_t						// opcodes are treated as two bytes to accomodate 'prebyte'.  if high byte is zero the opcode is actually one byte
		baseOpcode[OT_NUM];
};

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"db",		HandleDB},
		{"dc.b",	HandleDB},
		{"dw",		HandleBEDW},		// words are big endian
		{"dc.w",	HandleBEDW},
		{"ds",		HandleDS},
		{"ds.b",	HandleDS},
		{"ds.w",	HandleDSW},
		{"incbin",	HandleIncbin},
	};

#define	______		0					// unused opcode

static OPCODE
	Opcodes[]=
	{
//																								  imp    imm8   imm16   dir    ext   rel8  rel16   ind   regreg stacks stacku
		{"abx",		M_IMPLIED,																	{0x003A,______,______,______,______,______,______,______,______,______,______}},
		{"adca",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0089,______,0x0099,0x00B9,______,______,0x00A9,______,______,______}},
		{"adcb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C9,______,0x00D9,0x00F9,______,______,0x00E9,______,______,______}},
		{"adda",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x008B,______,0x009B,0x00BB,______,______,0x00AB,______,______,______}},
		{"addb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00CB,______,0x00DB,0x00FB,______,______,0x00EB,______,______,______}},
		{"addd",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x00C3,0x00D3,0x00F3,______,______,0x00E3,______,______,______}},
		{"anda",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0084,______,0x0094,0x00B4,______,______,0x00A4,______,______,______}},
		{"andb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C4,______,0x00D4,0x00F4,______,______,0x00E4,______,______,______}},
		{"andcc",	M_IMMEDIATE8,																{______,0x001C,______,______,______,______,______,______,______,______,______}},
		{"asl",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0008,0x0078,______,______,0x0068,______,______,______}},
		{"asla",	M_IMPLIED,																	{0x0048,______,______,______,______,______,______,______,______,______,______}},
		{"aslb",	M_IMPLIED,																	{0x0058,______,______,______,______,______,______,______,______,______,______}},
		{"asr",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0007,0x0077,______,______,0x0067,______,______,______}},
		{"asra",	M_IMPLIED,																	{0x0047,______,______,______,______,______,______,______,______,______,______}},
		{"asrb",	M_IMPLIED,																	{0x0057,______,______,______,______,______,______,______,______,______,______}},
		{"bcc",		M_RELATIVE8,																{______,______,______,______,______,0x0024,______,______,______,______,______}},
		{"bcs",		M_RELATIVE8,																{______,______,______,______,______,0x0025,______,______,______,______,______}},
		{"beq",		M_RELATIVE8,																{______,______,______,______,______,0x0027,______,______,______,______,______}},
		{"bge",		M_RELATIVE8,																{______,______,______,______,______,0x002C,______,______,______,______,______}},
		{"bgt",		M_RELATIVE8,																{______,______,______,______,______,0x002E,______,______,______,______,______}},
		{"bhi",		M_RELATIVE8,																{______,______,______,______,______,0x0022,______,______,______,______,______}},
		{"bhs",		M_RELATIVE8,																{______,______,______,______,______,0x0024,______,______,______,______,______}},
		{"bita",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0085,______,0x0095,0x00B5,______,______,0x00A5,______,______,______}},
		{"bitb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C5,______,0x00D5,0x00F5,______,______,0x00E5,______,______,______}},
		{"ble",		M_RELATIVE8,																{______,______,______,______,______,0x002F,______,______,______,______,______}},
		{"blo",		M_RELATIVE8,																{______,______,______,______,______,0x0025,______,______,______,______,______}},
		{"bls",		M_RELATIVE8,																{______,______,______,______,______,0x0023,______,______,______,______,______}},
		{"blt",		M_RELATIVE8,																{______,______,______,______,______,0x002D,______,______,______,______,______}},
		{"bmi",		M_RELATIVE8,																{______,______,______,______,______,0x002B,______,______,______,______,______}},
		{"bne",		M_RELATIVE8,																{______,______,______,______,______,0x0026,______,______,______,______,______}},
		{"bpl",		M_RELATIVE8,																{______,______,______,______,______,0x002A,______,______,______,______,______}},
		{"bra",		M_RELATIVE8,																{______,______,______,______,______,0x0020,______,______,______,______,______}},
		{"brn",		M_RELATIVE8,																{______,______,______,______,______,0x0021,______,______,______,______,______}},
		{"bsr",		M_RELATIVE8,																{______,______,______,______,______,0x008D,______,______,______,______,______}},
		{"bvc",		M_RELATIVE8,																{______,______,______,______,______,0x0028,______,______,______,______,______}},
		{"bvs",		M_RELATIVE8,																{______,______,______,______,______,0x0029,______,______,______,______,______}},
		{"clr",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x000F,0x007F,______,______,0x006F,______,______,______}},
		{"clra",	M_IMPLIED,																	{0x004F,______,______,______,______,______,______,______,______,______,______}},
		{"clrb",	M_IMPLIED,																	{0x005F,______,______,______,______,______,______,______,______,______,______}},
		{"cmpa",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0081,______,0x0091,0x00B1,______,______,0x00A1,______,______,______}},
		{"cmpb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C1,______,0x00D1,0x00F1,______,______,0x00E1,______,______,______}},
		{"cmpd",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x1083,0x1093,0x10B3,______,______,0x10A3,______,______,______}},
		{"cmps",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x118C,0x119C,0x11BC,______,______,0x11AC,______,______,______}},
		{"cmpu",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x1183,0x1193,0x11B3,______,______,0x11A3,______,______,______}},
		{"cmpx",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x008C,0x009C,0x00BC,______,______,0x00AC,______,______,______}},
		{"cmpy",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x108C,0x109C,0x10BC,______,______,0x10AC,______,______,______}},
		{"com",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0003,0x0073,______,______,0x0063,______,______,______}},
		{"coma",	M_IMPLIED,																	{0x0043,______,______,______,______,______,______,______,______,______,______}},
		{"comb",	M_IMPLIED,																	{0x0053,______,______,______,______,______,______,______,______,______,______}},
		{"cwai",	M_IMPLIED,																	{0x003C,______,______,______,______,______,______,______,______,______,______}},
		{"daa",		M_IMPLIED,																	{0x0019,______,______,______,______,______,______,______,______,______,______}},
		{"dec",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x000A,0x007A,______,______,0x006A,______,______,______}},
		{"deca",	M_IMPLIED,																	{0x004A,______,______,______,______,______,______,______,______,______,______}},
		{"decb",	M_IMPLIED,																	{0x005A,______,______,______,______,______,______,______,______,______,______}},
		{"eora",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0088,______,0x0098,0x00B8,______,______,0x00A8,______,______,______}},
		{"eorb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C8,______,0x00D8,0x00F8,______,______,0x00E8,______,______,______}},
		{"exg",		M_REGREG,																	{______,______,______,______,______,______,______,______,0x001E,______,______}},
		{"inc",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x000C,0x007C,______,______,0x006C,______,______,______}},
		{"inca",	M_IMPLIED,																	{0x004C,______,______,______,______,______,______,______,______,______,______}},
		{"incb",	M_IMPLIED,																	{0x005C,______,______,______,______,______,______,______,______,______,______}},
		{"jmp",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x000E,0x007E,______,______,0x006E,______,______,______}},
		{"jsr",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x009D,0x00BD,______,______,0x00AD,______,______,______}},
		{"lbcc",	M_RELATIVE16,																{______,______,______,______,______,______,0x1024,______,______,______,______}},
		{"lbcs",	M_RELATIVE16,																{______,______,______,______,______,______,0x1025,______,______,______,______}},
		{"lbeq",	M_RELATIVE16,																{______,______,______,______,______,______,0x1027,______,______,______,______}},
		{"lbge",	M_RELATIVE16,																{______,______,______,______,______,______,0x102C,______,______,______,______}},
		{"lbgt",	M_RELATIVE16,																{______,______,______,______,______,______,0x102E,______,______,______,______}},
		{"lbhi",	M_RELATIVE16,																{______,______,______,______,______,______,0x1022,______,______,______,______}},
		{"lbhs",	M_RELATIVE16,																{______,______,______,______,______,______,0x1024,______,______,______,______}},
		{"lble",	M_RELATIVE16,																{______,______,______,______,______,______,0x102F,______,______,______,______}},
		{"lblo",	M_RELATIVE16,																{______,______,______,______,______,______,0x1025,______,______,______,______}},
		{"lbls",	M_RELATIVE16,																{______,______,______,______,______,______,0x1023,______,______,______,______}},
		{"lblt",	M_RELATIVE16,																{______,______,______,______,______,______,0x102D,______,______,______,______}},
		{"lbmi",	M_RELATIVE16,																{______,______,______,______,______,______,0x102B,______,______,______,______}},
		{"lbne",	M_RELATIVE16,																{______,______,______,______,______,______,0x1026,______,______,______,______}},
		{"lbpl",	M_RELATIVE16,																{______,______,______,______,______,______,0x102A,______,______,______,______}},
		{"lbra",	M_RELATIVE16,																{______,______,______,______,______,______,0x1020,______,______,______,______}},
		{"lbrn",	M_RELATIVE16,																{______,______,______,______,______,______,0x1021,______,______,______,______}},
		{"lbsr",	M_RELATIVE16,																{______,______,______,______,______,______,0x108D,______,______,______,______}},
		{"lbvc",	M_RELATIVE16,																{______,______,______,______,______,______,0x1028,______,______,______,______}},
		{"lbvs",	M_RELATIVE16,																{______,______,______,______,______,______,0x1029,______,______,______,______}},
		{"lda",		M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0086,______,0x0096,0x00B6,______,______,0x00A6,______,______,______}},
		{"ldb",		M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C6,______,0x00D6,0x00F6,______,______,0x00E6,______,______,______}},
		{"ldd",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x00CC,0x00DC,0x00FC,______,______,0x00EC,______,______,______}},
		{"lds",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x10CE,0x10DE,0x10FE,______,______,0x10EE,______,______,______}},
		{"ldu",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x00CE,0x00DE,0x00FE,______,______,0x00EE,______,______,______}},
		{"ldx",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x008E,0x009E,0x00BE,______,______,0x00AE,______,______,______}},
		{"ldy",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x108E,0x109E,0x10BE,______,______,0x10AE,______,______,______}},
		{"leas",	M_INDEXED,																	{______,______,______,______,______,______,______,0x0032,______,______,______}},
		{"leau",	M_INDEXED,																	{______,______,______,______,______,______,______,0x0033,______,______,______}},
		{"leax",	M_INDEXED,																	{______,______,______,______,______,______,______,0x0030,______,______,______}},
		{"leay",	M_INDEXED,																	{______,______,______,______,______,______,______,0x0031,______,______,______}},
		{"lsl",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0008,0x0078,______,______,0x0068,______,______,______}},
		{"lsla",	M_IMPLIED,																	{0x0048,______,______,______,______,______,______,______,______,______,______}},
		{"lslb",	M_IMPLIED,																	{0x0058,______,______,______,______,______,______,______,______,______,______}},
		{"lsr",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0004,0x0074,______,______,0x0064,______,______,______}},
		{"lsra",	M_IMPLIED,																	{0x0044,______,______,______,______,______,______,______,______,______,______}},
		{"lsrb",	M_IMPLIED,																	{0x0054,______,______,______,______,______,______,______,______,______,______}},
		{"mul",		M_IMPLIED,																	{0x003D,______,______,______,______,______,______,______,______,______,______}},
		{"neg",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0000,0x0070,______,______,0x0060,______,______,______}},
		{"nega",	M_IMPLIED,																	{0x0040,______,______,______,______,______,______,______,______,______,______}},
		{"negb",	M_IMPLIED,																	{0x0050,______,______,______,______,______,______,______,______,______,______}},
		{"nop",		M_IMPLIED,																	{0x0012,______,______,______,______,______,______,______,______,______,______}},
		{"ora",		M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x008A,______,0x009A,0x00BA,______,______,0x00AA,______,______,______}},
		{"orb",		M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00CA,______,0x00DA,0x00FA,______,______,0x00EA,______,______,______}},
		{"orcc",	M_IMMEDIATE8,																{______,0x001A,______,______,______,______,______,______,______,______,______}},
		{"pshs",	M_STACKS,																	{______,______,______,______,______,______,______,______,______,0x0034,______}},
		{"pshu",	M_STACKU,																	{______,______,______,______,______,______,______,______,______,______,0x0036}},
		{"puls",	M_STACKS,																	{______,______,______,______,______,______,______,______,______,0x0035,______}},
		{"pulu",	M_STACKU,																	{______,______,______,______,______,______,______,______,______,______,0x0037}},
		{"rol",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0009,0x0079,______,______,0x0069,______,______,______}},
		{"rola",	M_IMPLIED,																	{0x0049,______,______,______,______,______,______,______,______,______,______}},
		{"rolb",	M_IMPLIED,																	{0x0059,______,______,______,______,______,______,______,______,______,______}},
		{"ror",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0006,0x0076,______,______,0x0066,______,______,______}},
		{"rora",	M_IMPLIED,																	{0x0046,______,______,______,______,______,______,______,______,______,______}},
		{"rorb",	M_IMPLIED,																	{0x0056,______,______,______,______,______,______,______,______,______,______}},
		{"rti",		M_IMPLIED,																	{0x003B,______,______,______,______,______,______,______,______,______,______}},
		{"rts",		M_IMPLIED,																	{0x0039,______,______,______,______,______,______,______,______,______,______}},
		{"sbca",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0082,______,0x0092,0x00B2,______,______,0x00A2,______,______,______}},
		{"sbcb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C2,______,0x00D2,0x00F2,______,______,0x00E2,______,______,______}},
		{"sex",		M_IMPLIED,																	{0x001D,______,______,______,______,______,______,______,______,______,______}},
		{"sta",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x0097,0x00B7,______,______,0x00A7,______,______,______}},
		{"stb",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x00D7,0x00F7,______,______,0x00E7,______,______,______}},
		{"std",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x00DD,0x00FD,______,______,0x00ED,______,______,______}},
		{"sts",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x10DF,0x10FF,______,______,0x10EF,______,______,______}},
		{"stu",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x00DF,0x00FF,______,______,0x00EF,______,______,______}},
		{"stx",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x009F,0x00BF,______,______,0x00AF,______,______,______}},
		{"sty",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x109F,0x10BF,______,______,0x10AF,______,______,______}},
		{"suba",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x0080,______,0x0090,0x00B0,______,______,0x00A0,______,______,______}},
		{"subb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED,									{______,0x00C0,______,0x00D0,0x00F0,______,______,0x00E0,______,______,______}},
		{"subd",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED,								{______,______,0x0083,0x0093,0x00B3,______,______,0x00A3,______,______,______}},
		{"swi",		M_IMPLIED,																	{0x003F,______,______,______,______,______,______,______,______,______,______}},
		{"swi2",	M_IMPLIED,																	{0x103F,______,______,______,______,______,______,______,______,______,______}},
		{"swi3",	M_IMPLIED,																	{0x113F,______,______,______,______,______,______,______,______,______,______}},
		{"sync",	M_IMPLIED,																	{0x0013,______,______,______,______,______,______,______,______,______,______}},
		{"tfr",		M_REGREG,																	{______,______,______,______,______,______,______,______,0x001F,______,______}},
		{"tst",		M_DIRECT|M_EXTENDED|M_INDEXED,												{______,______,______,0x000D,0x007D,______,______,0x006D,______,______,______}},
		{"tsta",	M_IMPLIED,																	{0x004D,______,______,______,______,______,______,______,______,______,______}},
		{"tstb",	M_IMPLIED,																	{0x005D,______,______,______,______,______,______,______,______,______,______}},
	};
//																								  imp    imm8   imm16   dir    ext   rel8  rel16   ind   regreg stacks stacku

static bool ParseImmediatePreamble(const char *line,unsigned int *lineIndex)
// Expect a pound sign, step over one if found
{
	if(line[*lineIndex]=='#')			// does this look like a '#'?
	{
		(*lineIndex)++;						// step over it
		return(true);
	}
	return(false);
}

static bool ParseRegister(const char *line,unsigned int *lineIndex,unsigned int *registerIndex)
// see if next thing on the line is a register name
// return true if so, false otherwise
{
	char
		registerName[MAX_STRING];
	unsigned int
		outputIndex;
	unsigned int
		localIndex;
	REGDEF
		*locatedRegister;

	localIndex=*lineIndex;						// keep index here for a while
	outputIndex=0;

	while(IsLabelChar(line[localIndex]))		// registers are a subset of labels
	{
		registerName[outputIndex++]=tolower(line[localIndex++]);
	}
	registerName[outputIndex]='\0';
	if(outputIndex&&(locatedRegister=(REGDEF *)STFindDataForName(regNameSymbols,registerName)))	// see if what we got looks like a register name
	{
		*registerIndex=(locatedRegister-&RegDefs[0]);	// get the register index
		*lineIndex=localIndex;
		return(true);
	}
	return(false);
}

// opcode handling for 6809

enum
{
	POT_VALUE,			// an unadorned value (an address of some sort)
	POT_IMMEDIATE,		// an immediate value
	POT_INDEXED,		// an indexed operand
	POT_REGLIST,		// a register list
};

static bool ParseImmediate(const char *line,unsigned int *lineIndex,int *value,bool *unresolved)
// Attempt to parse an immediate value. If one can be parsed, update lineIndex, value, and unresolved,
// then return true
// Otherwise, leave lineIndex unmodified, and return false
{
	unsigned int
		initialIndex;

	initialIndex=*lineIndex;
	if(ParseImmediatePreamble(line,lineIndex))
	{
		if(ParseExpression(line,lineIndex,value,unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				return(true);
			}
		}
	}
	*lineIndex=initialIndex;		// put this back if parsing fails
	return(false);
}

static bool ParseRegisterList(const char *line,unsigned int *lineIndex,REGLIST *registerList)
// Attempt to parse a register list. If one can be parsed, update lineIndex, value, and unresolved,
// then return true
// Otherwise, leave lineIndex unmodified, and return false
{
	unsigned int
		initialIndex;
	unsigned int
		registerIndex;
	bool
		done;
	bool
		fail;

	initialIndex=*lineIndex;
	registerList->numRegs=0;
	fail=false;
	done=false;
	while(!fail&&!done)
	{
		if(ParseRegister(line,lineIndex,&registerIndex))
		{
			if(registerList->numRegs<MAXREGLISTSIZE)	// do not overflow
			{
				registerList->registers[registerList->numRegs]=(unsigned char)registerIndex;
				registerList->numRegs++;
			}
			if(!ParseComment(line,lineIndex))		// if non-end of line, then we must see a separator
			{
				if(!ParseCommaSeparator(line,lineIndex))	// get the separator
				{
					fail=true;
				}
			}
			else
			{
				done=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	if(!fail)
	{
		return(true);
	}
	*lineIndex=initialIndex;							// put this back if parsing fails
	return(false);
}

static bool ParseOffsetRegister(const char *line,unsigned int *lineIndex,unsigned int *registerIndex)
// Parse a register that can be used for the offset of an indexed addressing mode
{
	unsigned int
		initialIndex;

	initialIndex=*lineIndex;

	if(ParseRegister(line,lineIndex,registerIndex))	// see if a register can be parsed
	{
		if((*registerIndex==R_D)||(*registerIndex==R_A)||(*registerIndex==R_B))	// make sure it is valid as an offset register
		{
			return(true);
		}
	}
	*lineIndex=initialIndex;
	return(false);
}

static bool ParseIndexRegister(const char *line,unsigned int *lineIndex,unsigned int *registerIndex,int *incdecCount)
// Parse a register that can be used for the index of an indexed addressing mode, also
// handle pre-decrement and post-increment
{
	unsigned int
		initialIndex;

	*incdecCount=0;
	initialIndex=*lineIndex;
	while((*incdecCount>=-1)&&!ParseComment(line,lineIndex)&&(line[*lineIndex]=='-'))
	{
		(*lineIndex)++;
		(*incdecCount)--;
	}
	ParseComment(line,lineIndex);
	if(ParseRegister(line,lineIndex,registerIndex))	// see if a register can be parsed
	{
		if((*registerIndex==R_X)||(*registerIndex==R_Y)||(*registerIndex==R_U)||(*registerIndex==R_S)||(*registerIndex==R_PC))
		{
			if(*incdecCount==0)							// if no predecrement, allow postincrement
			{
				while((*incdecCount<=1)&&!ParseComment(line,lineIndex)&&(line[*lineIndex]=='+'))
				{
					(*lineIndex)++;
					(*incdecCount)++;
				}
				return(true);
			}
			else		// had predecrement, so done
			{
				return(true);
			}
		}
	}
	*lineIndex=initialIndex;
	return(false);
}

static bool ParseIndexedNonIndirect(const char *line,unsigned int *lineIndex,INDEXEDADDRESS *indexedAddress)
// Attempt to parse an indexed address, but do not pay attention to indirection.
// Also, return true as soon as the end of the indexed address is encountered -- even
// if not at the end of the line.
{
	unsigned int
		initialIndex;
	unsigned int
		registerIndex;
	bool
		fail;
	bool
		done;

	indexedAddress->haveIndex=false;
	initialIndex=*lineIndex;
	fail=done=false;
	if(ParseCommaSeparator(line,lineIndex))		// an initial ',' is allowed, it means that the index is immediate 0
	{
		indexedAddress->offsetIsConstant=true;
		indexedAddress->constantOffset=0;
		indexedAddress->constantOffsetUnresolved=false;
	}
	else if(ParseOffsetRegister(line,lineIndex,&registerIndex))	// if not an initial ',' then try to get an offset register
	{
		if(!ParseComment(line,lineIndex))			// must not see end of the line at this point
		{
			if(ParseCommaSeparator(line,lineIndex))	// must have the ',' following the offset register
			{
				indexedAddress->offsetIsConstant=false;
				indexedAddress->offsetRegister=registerIndex;
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else if(ParseIndexRegister(line,lineIndex,&indexedAddress->indexRegister,&indexedAddress->incdecCount))	// try to get just plain index register
	{
		indexedAddress->offsetIsConstant=true;
		indexedAddress->constantOffsetUnresolved=false;
		indexedAddress->constantOffset=0;
		indexedAddress->haveIndex=true;
		done=true;
	}
	else if(ParseExpression(line,lineIndex,&indexedAddress->constantOffset,&indexedAddress->constantOffsetUnresolved))	// otherwise, see if expression
	{
		indexedAddress->offsetIsConstant=true;
		if(!ParseComment(line,lineIndex))			// if not at the end, then expect a register
		{
			if(ParseCommaSeparator(line,lineIndex))	// must have the ',' following the offset expression
			{
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			done=true;									// saw an expression, but no index register (this is valid)
		}
	}
	else
	{
		fail=true;
	}
	if(!fail&&!done)
	{
		if(!ParseComment(line,lineIndex))			// must not see end of the line at this point
		{
			if(ParseIndexRegister(line,lineIndex,&indexedAddress->indexRegister,&indexedAddress->incdecCount))	// try to get index register
			{
				indexedAddress->haveIndex=true;			// got one
			}
			else
			{
				fail=true;
			}
		}
	}

	if(!fail)
	{
		return(true);									// got indexed address!
	}
	*lineIndex=initialIndex;							// put this back if parsing fails
	return(false);
}

static bool ParseIndexed(const char *line,unsigned int *lineIndex,INDEXEDADDRESS *indexedAddress)
// Attempt to parse an indexed address. If one can be parsed, update lineIndex, and indexedAddress,
// then return true
// Otherwise, leave lineIndex unmodified, and return false
{
	unsigned int
		initialIndex;
	bool
		fail;

	initialIndex=*lineIndex;
	indexedAddress->indirect=false;
	fail=false;

	if(line[*lineIndex]=='[')						// indirect?
	{
		(*lineIndex)++;
		indexedAddress->indirect=true;					// remember that
	}
	if(!ParseComment(line,lineIndex))				// must not see end of the line at this point
	{
		if(ParseIndexedNonIndirect(line,lineIndex,indexedAddress))	// try to get indexed address
		{
			if(indexedAddress->indirect)				// if indirect, then we also need the closing ']'
			{
				if(!ParseComment(line,lineIndex))
				{
					if(line[*lineIndex]==']')
					{
						(*lineIndex)++;
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}

	if(!fail)
	{
		if(ParseComment(line,lineIndex))				// need to see end of line now
		{
			return(true);
		}
	}
	*lineIndex=initialIndex;							// put this back if parsing fails
	return(false);
}

static bool ParseAbsolute(const char *line,unsigned int *lineIndex,int *value,bool *unresolved)
// Attempt to parse an absolute value. If one can be parsed, update lineIndex, value, and unresolved,
// then return true
// Otherwise, leave lineIndex unmodified, and return false
{
	unsigned int
		initialIndex;

	initialIndex=*lineIndex;
	if(ParseExpression(line,lineIndex,value,unresolved))
	{
		if(ParseComment(line,lineIndex))
		{
			return(true);
		}
	}
	*lineIndex=initialIndex;		// put this back if parsing fails
	return(false);
}

static bool ParsePossibleOperands(const char *line,unsigned int *lineIndex,unsigned int typeMask,unsigned int *operandType,int *value,bool *unresolved,INDEXEDADDRESS *indexedAddress,REGLIST *registerList)
// passed a line and offset to the operands, attempt to parse
// out one of the generic operand types based on the explicit operand types
// allowed for the current opcode
// If something is successfully parsed, then return true, else return false
// NOTE: if the returned type is POT_VALUE or POT_IMMEDIATE, then the result is contained
// in value, and unresolved.
// If the returned type is POT_INDEXED, the result is contained in indexedAddress.
// If the returned type is POT_REGLIST, the result is contained in registerList.
{
	if(typeMask&(M_IMMEDIATE8|M_IMMEDIATE16))		// see if some sort of immediate value is allowed
	{
		if(ParseImmediate(line,lineIndex,value,unresolved))
		{
			*operandType=POT_IMMEDIATE;
			return(true);
		}
	}
 	if(typeMask&(M_REGREG|M_STACKS|M_STACKU))		// see if some sort of register list is allowed (this takes precedence over indexed addressing)
	{
		if(ParseRegisterList(line,lineIndex,registerList))
		{
			*operandType=POT_REGLIST;
			return(true);
		}
	}
	if(typeMask&M_INDEXED)							// see if indexed addressing is allowed
	{
		if(ParseIndexed(line,lineIndex,indexedAddress))
		{
			if(!indexedAddress->haveIndex&&!indexedAddress->indirect&&indexedAddress->offsetIsConstant)	// convert non-index register operand into a value
			{
				if(typeMask&(M_DIRECT|M_EXTENDED|M_RELATIVE8|M_RELATIVE16))		// see if some sort of absolute value is allowed
				{
					*operandType=POT_VALUE;
					*value=indexedAddress->constantOffset;
					*unresolved=indexedAddress->constantOffsetUnresolved;
					return(true);
				}
			}
			else
			{
				*operandType=POT_INDEXED;
				return(true);
			}
		}
	}
	else if(typeMask&(M_DIRECT|M_EXTENDED|M_RELATIVE8|M_RELATIVE16))		// see if some sort of absolute value is allowed (only check if not indexed, otherwise he tests it)
	{
		if(ParseAbsolute(line,lineIndex,value,unresolved))
		{
			*operandType=POT_VALUE;
			return(true);
		}
	}
	return(false);
}

static bool WriteOpcode(opcode_t opcode,LISTING_RECORD *listingRecord)
// Write the passed opcode out -- pay attention to the "prebyte"
// and write it only if it is non-zero
{
	bool
		fail;
	unsigned char
		prebyte;

	fail=false;

	prebyte=(opcode>>8)&0xff;
	if(prebyte)					// if it has a pre-byte, put out both bytes
	{
		fail=!GenerateByte(prebyte,listingRecord);
	}
	if(!fail)
	{
		fail=!GenerateByte(opcode&0xff,listingRecord);
	}
	return(!fail);
}

static bool HandleDirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with direct mode output only
// return false only on 'hard' errors
{
	bool
		fail;

	fail=false;
	CheckUnsignedByteRange(value,true,true);	// make sure the value is in range
	if(WriteOpcode(opcode->baseOpcode[OT_DIRECT],listingRecord))
	{
		fail=!GenerateByte(value,listingRecord);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleExtended(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with extended mode output only
// return false only on 'hard' errors
{
	bool
		fail;

	fail=false;
	CheckUnsignedWordRange(value,true,true);	// make sure the value is in range
	if(WriteOpcode(opcode->baseOpcode[OT_EXTENDED],listingRecord))
	{
		if(GenerateByte(value>>8,listingRecord))
		{
			fail=!GenerateByte(value&0xFF,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleDirectOrExtended(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a direct or extended address has been parsed
// work out which, and generate code
// return false only on 'hard' errors
{
	if((value>=0)&&(value<256))
	{
		return(HandleDirect(opcode,value,unresolved,listingRecord));
	}
	else
	{
		return(HandleExtended(opcode,value,unresolved,listingRecord));
	}
}

static bool HandleRelative8(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a relative 8 bit operand has been parsed
// return false only on 'hard' errors
{
	bool
		fail;
	int
		offset;

	if(WriteOpcode(opcode->baseOpcode[OT_RELATIVE8],listingRecord))
	{
		offset=0;
		if(!unresolved&&currentSegment)
		{
			offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset+1);
			Check8RelativeRange(offset,true,true);
		}
		fail=!GenerateByte(offset,listingRecord);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleRelative16(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a relative 16 bit operand has been parsed
// return false only on 'hard' errors
{
	bool
		fail;
	int
		offset;

	if(WriteOpcode(opcode->baseOpcode[OT_RELATIVE16],listingRecord))
	{
		offset=0;
		if(!unresolved&&currentSegment)
		{
			offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset+2);
			Check16RelativeRange(offset,true,true);
		}
		if(GenerateByte(offset>>8,listingRecord))
		{
			fail=!GenerateByte(offset&0xFF,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleRelative(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a relative operand (8 or 16 bit) has been parsed
// return false only on 'hard' errors
{
	int
		offset;

	offset=0;
	if(!unresolved&&currentSegment)
	{
		offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset+2);			// see if 8 bit offset will suffice
	}
	if(offset>=-128&&offset<128)
	{
		return(HandleRelative8(opcode,value,unresolved,listingRecord));
	}
	else
	{
		return(HandleRelative16(opcode,value,unresolved,listingRecord));
	}
}

static bool HandleValue(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an operand which is just a value has been parsed as the operand
// Look at the addressing modes allowed for this opcode, and treat the value as
// needed
// return false only on 'hard' errors
{
	if((opcode->typeMask&(M_DIRECT|M_EXTENDED))==(M_DIRECT|M_EXTENDED))					// see if ambiguous direct/extended
	{
		return(HandleDirectOrExtended(opcode,value,unresolved,listingRecord));			// go resolve the ambiguity
	}
	else if(opcode->typeMask&M_DIRECT)
	{
		return(HandleDirect(opcode,value,unresolved,listingRecord));
	}
	else if(opcode->typeMask&M_EXTENDED)
	{
		return(HandleExtended(opcode,value,unresolved,listingRecord));
	}
	else if((opcode->typeMask&(M_RELATIVE8|M_RELATIVE16))==(M_RELATIVE8|M_RELATIVE16))		// see if ambiguous relative
	{
		return(HandleRelative(opcode,value,unresolved,listingRecord));
	}
	else if(opcode->typeMask&M_RELATIVE8)
	{
		return(HandleRelative8(opcode,value,unresolved,listingRecord));
	}
	else if(opcode->typeMask&M_RELATIVE16)
	{
		return(HandleRelative16(opcode,value,unresolved,listingRecord));
	}
	return(true);		// this should not occur
}

static bool HandleImmediate8(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an immediate 8 bit addressing mode was located
// return false only on 'hard' errors
{
	bool
		fail;

	fail=false;
	CheckByteRange(value,true,true);
	if(WriteOpcode(opcode->baseOpcode[OT_IMMEDIATE8],listingRecord))
	{
		fail=!GenerateByte(value,listingRecord);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleImmediate16(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an immediate 16 bit addressing mode was located
// return false only on 'hard' errors
{
	bool
		fail;

	fail=false;
	CheckWordRange(value,true,true);
	if(WriteOpcode(opcode->baseOpcode[OT_IMMEDIATE16],listingRecord))
	{
		if(GenerateByte(value>>8,listingRecord))
		{
			fail=!GenerateByte(value&0xFF,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleImmediate(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an ambiguous immediate addressing mode was located
// return false only on 'hard' errors
{
	if(value>=-128&&value<256)
	{
		return(HandleImmediate8(opcode,value,unresolved,listingRecord));
	}
	else
	{
		return(HandleImmediate16(opcode,value,unresolved,listingRecord));
	}
}

static bool HandleImmediateValue(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an immediate value was located, dispatch to the handler based on the
// addressing modes allowed by the insn
// return false only on 'hard' errors
{
	if((opcode->typeMask&(M_IMMEDIATE8|M_IMMEDIATE16))==(M_IMMEDIATE8|M_IMMEDIATE16))	// see if ambiguous
	{
		return(HandleImmediate(opcode,value,unresolved,listingRecord));	// go resolve the ambiguity
	}
	else if(opcode->typeMask&M_IMMEDIATE8)
	{
		return(HandleImmediate8(opcode,value,unresolved,listingRecord));
	}
	else if(opcode->typeMask&M_IMMEDIATE16)
	{
		return(HandleImmediate16(opcode,value,unresolved,listingRecord));
	}
	return(true);		// this should not occur
}
	
static bool HandleIndexed(OPCODE *opcode,INDEXEDADDRESS *indexedAddress,LISTING_RECORD *listingRecord)
// an indexed addressing mode was located, deal with it
// NOTE: this is difficult to follow because there are lots of cases...
// return false only on 'hard' errors
{
	bool
		fail;
	unsigned char
		postByte;					// generate the postbyte for the indexed addressing mode in here
	bool
		offsetBytes;				// tells if offset bytes should be appended

	postByte=0x80;				// initialize (assume it is not a 5-bit constant offset)
	offsetBytes=false;
	if(indexedAddress->indirect)	// set indirect bit
	{
		postByte|=0x10;
	}
	if(indexedAddress->haveIndex)	// see if there's an index register
	{
		if((indexedAddress->indexRegister!=R_PC)&&(indexedAddress->indexRegister!=R_PCR))
		{
			switch(indexedAddress->indexRegister)
			{
				case R_X:
				case R_Y:
				case R_U:
				case R_S:
					postByte|=RegDefs[indexedAddress->indexRegister].indexMask;	// set up bits for indexing with this register
					if(indexedAddress->offsetIsConstant)
					{
						if(indexedAddress->constantOffset==0)	// 0 constant offset is handled differently
						{
							switch(indexedAddress->incdecCount)
							{
								case -2:
									postByte|=0x03;
									break;
								case -1:
									postByte|=0x02;
									if(indexedAddress->indirect)
									{
										AssemblyComplaint(NULL,true,"Predecrement by 1 with indirect is not allowed\n");
									}
									break;
								case 0:
									postByte|=0x04;				// 0 offset, 0 inc/dec
									break;
								case 1:
									postByte|=0x00;
									if(indexedAddress->indirect)
									{
										AssemblyComplaint(NULL,true,"Postincrement by 1 with indirect is not allowed\n");
									}
									break;
								case 2:
									postByte|=0x01;
									break;
								default:
									AssemblyComplaint(NULL,true,"Invalid predecrement/postincrement\n");
									break;
							}
						}
						else		// non-zero offset
						{
							if(indexedAddress->incdecCount==0)
							{
								if(!indexedAddress->indirect&&indexedAddress->constantOffset>=-16&&indexedAddress->constantOffset<16)	// does the offset fit in 5 bits? (only works if not indirect)
								{
									postByte&=0x7F;				// knock off the high bit set at the start
									postByte|=indexedAddress->constantOffset&0x1F;	// offset built into postbyte
								}
								else
								{
									postByte|=0x08;				// external offset
									offsetBytes=true;			// need offset bytes
								}
							}
							else
							{
								AssemblyComplaint(NULL,true,"Predecrement/postincrement not allowed with non-zero offset\n");
							}
						}
					}
					else			// register based offset
					{
						if((indexedAddress->offsetRegister==R_A)||(indexedAddress->offsetRegister==R_B)||(indexedAddress->offsetRegister==R_D))
						{
							if(indexedAddress->incdecCount==0)
							{
								postByte|=RegDefs[indexedAddress->offsetRegister].indexMask;	// set up bits for offsetting with this register
							}
							else
							{
								AssemblyComplaint(NULL,true,"Predecrement/postincrement and register offsets are not allowed together\n");
							}
						}
						else
						{
							AssemblyComplaint(NULL,true,"Invalid offset register\n");
						}
					}
					break;
				default:
					AssemblyComplaint(NULL,true,"Invalid index register\n");
					break;
			}
		}
		else					// PC based indexed address
		{
			if(indexedAddress->incdecCount==0)
			{
				if(indexedAddress->offsetIsConstant)
				{
					postByte|=RegDefs[indexedAddress->indexRegister].indexMask;		// set up bits for indexing with this register
					offsetBytes=true;			// need offset bytes
				}
				else
				{
					AssemblyComplaint(NULL,true,"PC relative indexing does not allow register offset\n");
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"PC relative indexing does not allow predecrement/postincrement\n");
			}
		}
	}
	else
	{
		if((indexedAddress->indirect)&&(indexedAddress->offsetIsConstant))	// if no index register, then must be indirect, and constant
		{
			postByte|=0x1F;
			offsetBytes=true;	// this mode requires offset bytes
		}
		else
		{
			AssemblyComplaint(NULL,true,"Invalid indexed address\n");
		}
	}

	// if an offset is needed, see if it can be 8 or 16 bits, and adjust the postbyte as needed
	if(offsetBytes)
	{
		if(!((indexedAddress->constantOffset>=-128)&&(indexedAddress->constantOffset<128)))	// see if it is not an 8 bit offset
		{
			postByte|=0x01;		// 16 bit offset
		}
	}

	// write out the instruction
	fail=false;

	if(WriteOpcode(opcode->baseOpcode[OT_INDEXED],listingRecord))
	{
		if(GenerateByte(postByte,listingRecord))
		{
			if(offsetBytes)
			{
				if(postByte&0x01)	// see if 16 bit offset
				{
					fail=!GenerateByte(indexedAddress->constantOffset>>8,listingRecord);
				}
				if(!fail)
				{
					fail=!GenerateByte(indexedAddress->constantOffset&0xFF,listingRecord);
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleStackS(OPCODE *opcode,REGLIST *regList,LISTING_RECORD *listingRecord)
// Look at the list of registers that were parsed, and make an S stack bit-field out
// of them.
// return false only on 'hard' errors
{
	bool
		fail;
	unsigned char
		bitField;
	unsigned int
		i;

	bitField=0;
	fail=false;
	for(i=0;i<regList->numRegs;i++)
	{
		if(regList->registers[i]!=R_S)
		{
			bitField|=RegDefs[regList->registers[i]].stackMask;
		}
		else
		{
			AssemblyComplaint(NULL,true,"'S' is not a valid register here\n");
		}
	}
	if(WriteOpcode(opcode->baseOpcode[OT_STACKS],listingRecord))
	{
		fail=!GenerateByte(bitField,listingRecord);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleStackU(OPCODE *opcode,REGLIST *regList,LISTING_RECORD *listingRecord)
// Look at the list of registers that were parsed, and make a U stack bit-field out
// of them.
// return false only on 'hard' errors
{
	bool
		fail;
	unsigned char
		bitField;
	unsigned int
		i;

	bitField=0;
	fail=false;
	for(i=0;i<regList->numRegs;i++)
	{
		if(regList->registers[i]!=R_U)
		{
			bitField|=RegDefs[regList->registers[i]].stackMask;
		}
		else
		{
			AssemblyComplaint(NULL,true,"'U' is not a valid register here\n");
		}
	}
	if(WriteOpcode(opcode->baseOpcode[OT_STACKU],listingRecord))
	{
		fail=!GenerateByte(bitField,listingRecord);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool HandleRegReg(OPCODE *opcode,REGLIST *regList,LISTING_RECORD *listingRecord)
// Work out register to register operand
// return false only on 'hard' errors
{
	bool
		fail;
	unsigned char
		reg0,
		reg1;

	fail=false;
	if(regList->numRegs==2)
	{
		reg0=regList->registers[0];
		reg1=regList->registers[1];
		if((RegDefs[reg0].is16Bit&&RegDefs[reg1].is16Bit)||(!RegDefs[reg0].is16Bit&&!RegDefs[reg1].is16Bit))
		{
			if(WriteOpcode(opcode->baseOpcode[OT_REGREG],listingRecord))
			{
				fail=!GenerateByte((RegDefs[reg0].exgIndex<<4)|RegDefs[reg1].exgIndex,listingRecord);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"Registers must be same width\n");
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Incorrect number of registers specified\n");
	}
	return(!fail);
}

static bool HandleRegList(OPCODE *opcode,REGLIST *regList,LISTING_RECORD *listingRecord)
// an addressing mode which requires a list of registers was located and parsed, deal with it
// return false only on 'hard' errors
{
	if(opcode->typeMask&M_STACKS)
	{
		return(HandleStackS(opcode,regList,listingRecord));
	}
	if(opcode->typeMask&M_STACKU)
	{
		return(HandleStackU(opcode,regList,listingRecord));
	}
	else if(opcode->typeMask&M_REGREG)
	{
		return(HandleRegReg(opcode,regList,listingRecord));
	}
	return(true);
}

static OPCODE *MatchOpcode(const char *string)
// match opcodes for this processor, return NULL if none matched
{
	return((OPCODE *)STFindDataForNameNoCase(opcodeSymbols,string));
}

static bool AttemptOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// look at the type of opcode available, parse operands as allowed
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	bool
		result;
	unsigned int
		tempIndex;
	char
		string[MAX_STRING];
	OPCODE
		*opcode;
	unsigned int
		operandType;
	int
		value;
	bool
		unresolved;
	INDEXEDADDRESS
		indexedAddress;
	REGLIST
		regList;

	result=true;					// no hard failure yet
	*success=false;					// no match yet
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=MatchOpcode(string)))						// found opcode?
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;
			if(!ParseComment(line,lineIndex))
			{
				if(ParsePossibleOperands(line,lineIndex,opcode->typeMask,&operandType,&value,&unresolved,&indexedAddress,&regList))
				{
					switch(operandType)
					{
						case POT_VALUE:
							result=HandleValue(opcode,value,unresolved,listingRecord);
							break;
						case POT_IMMEDIATE:
							result=HandleImmediateValue(opcode,value,unresolved,listingRecord);
							break;
						case POT_INDEXED:
							result=HandleIndexed(opcode,&indexedAddress,listingRecord);
							break;
						case POT_REGLIST:
							result=HandleRegList(opcode,&regList,listingRecord);
							break;
					}
				}
				else
				{
					ReportBadOperands();	// could not make sense out of the operands given the allowable addressing modes
				}
			}
			else	// hit end of line with no operands
			{
				if(opcode->typeMask&M_IMPLIED)
				{
					result=WriteOpcode(opcode->baseOpcode[OT_IMPLIED],listingRecord);
				}
				else
				{
					ReportBadOperands();
				}
			}
		}
	}
	return(result);
}

static bool AttemptPseudoOpcode(const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool *success)
// See if the next thing on the line looks like a pseudo-op.
// If this matches anything, it will set success true.
// If there's some sort of hard failure, this will return false
{
	bool
		result;
	unsigned int
		tempIndex;
	char
		string[MAX_STRING];
	PSEUDO_OPCODE
		*opcode;

	result=true;					// no hard failure yet
	*success=false;					// no match yet
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=(PSEUDO_OPCODE *)STFindDataForNameNoCase(pseudoOpcodeSymbols,string)))		// matches global pseudo-op?
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;
			result=opcode->function(string,line,lineIndex,lineLabel,listingRecord);
		}
	}
	return(result);
}

static bool SelectProcessor(PROCESSOR *processor)
// A processor in this family is being selected to assemble with
{
	return(true);
}

static void DeselectProcessor(PROCESSOR *processor)
// A processor in this family is being deselected
{
}

static void UnInitFamily()
// undo what InitFamily did
{
	STDisposeSymbolTable(regNameSymbols);
	STDisposeSymbolTable(opcodeSymbols);
	STDisposeSymbolTable(pseudoOpcodeSymbols);
}

static bool InitFamily()
// initialize symbol tables
{
	unsigned int
		i;
	bool
		fail;

	fail=false;
	if((pseudoOpcodeSymbols=STNewSymbolTable(elementsof(pseudoOpcodes))))
	{
		for(i=0;!fail&&(i<elementsof(pseudoOpcodes));i++)
		{
			if(!STAddEntryAtEnd(pseudoOpcodeSymbols,pseudoOpcodes[i].name,&pseudoOpcodes[i]))
			{
				fail=true;
			}
		}
		if(!fail)
		{
			if((opcodeSymbols=STNewSymbolTable(elementsof(Opcodes))))
			{
				for(i=0;!fail&&(i<elementsof(Opcodes));i++)
				{
					if(!STAddEntryAtEnd(opcodeSymbols,Opcodes[i].name,&Opcodes[i]))
					{
						fail=true;
					}
				}
				if(!fail)
				{
					if((regNameSymbols=STNewSymbolTable(elementsof(RegDefs))))
					{
						for(i=0;!fail&&(i<elementsof(RegDefs));i++)
						{
							if(!STAddEntryAtEnd(regNameSymbols,RegDefs[i].name,&RegDefs[i]))
							{
								fail=true;
							}
						}
						if(!fail)
						{
							return(true);
						}
						STDisposeSymbolTable(regNameSymbols);
					}
				}
				STDisposeSymbolTable(opcodeSymbols);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Motorola 6809",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"6809",NULL),
	};
