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
//
//	Motorola MC68HC11 support Copyright (C) 1999 Cosmodog, Ltd.
//
//-----------------------------------------------------------------------------



// 68HC11 ADDRESSING MODE FORMS
//
//	inherent:				op
//	immediate8:				op	#ii
//	immediate16:			op	#jjkk
//	direct:					op	dd
//	extended:				op	hhll
//	indexed_x:				op	ff,x
//	indexed_y:				op	ff,y
//	relative:				op	rr
//	bit_direct:				op	dd,mm
//	bit_indexed_x:			op	ff,x,mm
//	bit_indexed_y:			op	ff,y,mm
//	bit_direct_relative:	op	dd,mm,rr
//	bit_indexed_x_relative:	op	ff,x,mm,rr
//	bit_indexed_y_relative:	op	ff,y,mm,rr
//
//	ii		8 bit immediate value
//	jjkk	16 bit immediate value (jj is upper 8 bits, kk is lower 8 bits)
//	dd		8 bit direct page address
//	hhll	16 bit address (hh is upper 8 bits, ll is lower 8 bits)
//	ff		8 bit unsigned offset
//	rr		8 bit signed offset
//	mm		8 bit mask
//
//	ff is an implied 0 if it is not included in indexed addressing modes, i.e.,
//		addd	x
//		addd	,x
//		addd	0,x
//	are all the same instruction (and are all valid)
//

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols;

// enumerated addressing modes

enum
{
	OT_INHERENT = 0,				// no operands
	OT_IMMEDIATE8,					// one byte immediate operand
	OT_IMMEDIATE16,					// two byte immediate operand
	OT_DIRECT,						// one byte direct
	OT_EXTENDED,					// two byte absolute
	OT_INDEXED_X,
	OT_INDEXED_Y,
	OT_RELATIVE,					// one byte relative offset
	OT_BIT_DIRECT,					// one byte direct, one bit mask
	OT_BIT_INDEXED_X,
	OT_BIT_INDEXED_Y,
	OT_BIT_DIRECT_RELATIVE,			// one byte direct, one bit mask
	OT_BIT_INDEXED_X_RELATIVE,
	OT_BIT_INDEXED_Y_RELATIVE,
	OT_NUM							// number of addressing modes
};


// masks for the various addressing modes

#define	M_INHERENT 						(1<<OT_INHERENT)
#define	M_IMMEDIATE8					(1<<OT_IMMEDIATE8)
#define	M_IMMEDIATE16					(1<<OT_IMMEDIATE16)
#define	M_DIRECT						(1<<OT_DIRECT)
#define	M_EXTENDED						(1<<OT_EXTENDED)
#define	M_INDEXED_X						(1<<OT_INDEXED_X)
#define	M_INDEXED_Y						(1<<OT_INDEXED_Y)
#define	M_RELATIVE						(1<<OT_RELATIVE)
#define	M_BIT_DIRECT					(1<<OT_BIT_DIRECT)
#define	M_BIT_INDEXED_X					(1<<OT_BIT_INDEXED_X)
#define	M_BIT_INDEXED_Y					(1<<OT_BIT_INDEXED_Y)
#define	M_BIT_DIRECT_RELATIVE			(1<<OT_BIT_DIRECT_RELATIVE)
#define	M_BIT_INDEXED_X_RELATIVE		(1<<OT_BIT_INDEXED_X_RELATIVE)
#define	M_BIT_INDEXED_Y_RELATIVE		(1<<OT_BIT_INDEXED_Y_RELATIVE)

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
//																								  inh    imm8   imm16   dir    ext   indx   indy   rel    bdir   binx   biny   bdr    bix    biy
		{"aba",		M_INHERENT,																	{0x001b,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"abx",		M_INHERENT,																	{0x003a,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"aby",		M_INHERENT,																	{0x183a,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"adca",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0089,______,0x0099,0x00b9,0x00a9,0x18a9,______,______,______,______,______,______,______}},
		{"adcb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c9,______,0x00d9,0x00f9,0x00e9,0x18e9,______,______,______,______,______,______,______}},
		{"adda",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x008b,______,0x009b,0x00bb,0x00ab,0x18ab,______,______,______,______,______,______,______}},
		{"addb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00cb,______,0x00db,0x00fb,0x00eb,0x18eb,______,______,______,______,______,______,______}},
		{"addd",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x00c3,0x00d3,0x00f3,0x00e3,0x18e3,______,______,______,______,______,______,______}},
		{"anda",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0084,______,0x0094,0x00b4,0x00a4,0x18a4,______,______,______,______,______,______,______}},
		{"andb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c4,______,0x00d4,0x00f4,0x00e4,0x18e4,______,______,______,______,______,______,______}},
		{"asla",	M_INHERENT,																	{0x0048,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"aslb",	M_INHERENT,																	{0x0058,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"asl",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0078,0x0068,0x1868,______,______,______,______,______,______,______}},
		{"asld",	M_INHERENT,																	{0x0005,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"asra",	M_INHERENT,																	{0x0047,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"asrb",	M_INHERENT,																	{0x0057,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"asr",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0077,0x0067,0x1867,______,______,______,______,______,______,______}},
		{"bcc",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0024,______,______,______,______,______,______}},		// same as bhs
		{"bclr",	M_BIT_DIRECT|M_BIT_INDEXED_X|M_BIT_INDEXED_Y,								{______,______,______,______,______,______,______,______,0x0015,0x001d,0x181d,______,______,______}},
		{"bcs",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0025,______,______,______,______,______,______}},		// same as blo
		{"beq",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0027,______,______,______,______,______,______}},
		{"bge",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002c,______,______,______,______,______,______}},
		{"bgt",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002e,______,______,______,______,______,______}},
		{"bhi",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0022,______,______,______,______,______,______}},
		{"bhs",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0024,______,______,______,______,______,______}},		// same as bcc
		{"bita",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0085,______,0x0095,0x00b5,0x00a5,0x18a5,______,______,______,______,______,______,______}},
		{"bitb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c5,______,0x00d5,0x00f5,0x00e5,0x18e5,______,______,______,______,______,______,______}},
		{"ble",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002f,______,______,______,______,______,______}},
		{"blo",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0025,______,______,______,______,______,______}},		// same as bcs
		{"bls",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0023,______,______,______,______,______,______}},
		{"blt",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002d,______,______,______,______,______,______}},
		{"bmi",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002b,______,______,______,______,______,______}},
		{"bne",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0026,______,______,______,______,______,______}},
		{"bpl",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x002a,______,______,______,______,______,______}},
		{"bra",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0020,______,______,______,______,______,______}},
		{"brclr",	M_BIT_DIRECT_RELATIVE|M_BIT_INDEXED_X_RELATIVE|M_BIT_INDEXED_Y_RELATIVE,	{______,______,______,______,______,______,______,______,______,______,______,0x0013,0x001f,0x181f}},
		{"brn",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0021,______,______,______,______,______,______}},
		{"brset",	M_BIT_DIRECT_RELATIVE|M_BIT_INDEXED_X_RELATIVE|M_BIT_INDEXED_Y_RELATIVE,	{______,______,______,______,______,______,______,______,______,______,______,0x0012,0x001e,0x181e}},
		{"bset",	M_BIT_DIRECT|M_BIT_INDEXED_X|M_BIT_INDEXED_Y,								{______,______,______,______,______,______,______,______,0x0014,0x001c,0x181c,______,______,______}},
		{"bsr",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x008d,______,______,______,______,______,______}},
		{"bvc",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0028,______,______,______,______,______,______}},
		{"bvs",		M_RELATIVE,																	{______,______,______,______,______,______,______,0x0029,______,______,______,______,______,______}},
		{"cba",		M_INHERENT,																	{0x0011,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"clc",		M_INHERENT,																	{0x000c,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"cli",		M_INHERENT,																	{0x000e,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"clra",	M_INHERENT,																	{0x004f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"clrb",	M_INHERENT,																	{0x005f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"clr",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x007f,0x006f,0x186f,______,______,______,______,______,______,______}},
		{"clv",		M_INHERENT,																	{0x000a,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"cmpa",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0081,______,0x0091,0x00b1,0x00a1,0x18a1,______,______,______,______,______,______,______}},
		{"cmpb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c1,______,0x00d1,0x00f1,0x00e1,0x18e1,______,______,______,______,______,______,______}},
		{"coma",	M_INHERENT,																	{0x0043,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"comb",	M_INHERENT,																	{0x0053,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"com",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0073,0x0063,0x1863,______,______,______,______,______,______,______}},
		{"cpd",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x1a83,0x1a93,0x1ab3,0x1aa3,0xcda3,______,______,______,______,______,______,______}},
		{"cpx",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x008c,0x009c,0x00bc,0x00ac,0xcdac,______,______,______,______,______,______,______}},
		{"cpy",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x188c,0x189c,0x18bc,0x1aac,0x18ac,______,______,______,______,______,______,______}},
		{"daa",		M_INHERENT,																	{0x0019,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"deca",	M_INHERENT,																	{0x004a,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"decb",	M_INHERENT,																	{0x005a,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"dec",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x007a,0x006a,0x186a,______,______,______,______,______,______,______}},
		{"des",		M_INHERENT,																	{0x0034,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"dex",		M_INHERENT,																	{0x0009,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"dey",		M_INHERENT,																	{0x1809,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"eora",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0088,______,0x0098,0x00b8,0x00a8,0x18a8,______,______,______,______,______,______,______}},
		{"eorb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c8,______,0x00d8,0x00f8,0x00e8,0x18e8,______,______,______,______,______,______,______}},
		{"fdiv",	M_INHERENT,																	{0x0003,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"idiv",	M_INHERENT,																	{0x0002,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"inca",	M_INHERENT,																	{0x004c,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"incb",	M_INHERENT,																	{0x005c,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"inc",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x007c,0x006c,0x186c,______,______,______,______,______,______,______}},
		{"ins",		M_INHERENT,																	{0x0031,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"inx",		M_INHERENT,																	{0x0008,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"iny",		M_INHERENT,																	{0x1808,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"jmp",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x007e,0x006e,0x186e,______,______,______,______,______,______,______}},
		{"jsr",		M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x009d,0x00bd,0x00ad,0x18ad,______,______,______,______,______,______,______}},
		{"ldaa",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0086,______,0x0096,0x00b6,0x00a6,0x18a6,______,______,______,______,______,______,______}},
		{"ldab",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c6,______,0x00d6,0x00f6,0x00e6,0x18e6,______,______,______,______,______,______,______}},
		{"ldd",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x00cc,0x00dc,0x00fc,0x00ec,0x18ec,______,______,______,______,______,______,______}},
		{"lds",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x008e,0x009e,0x00be,0x00ae,0x18ae,______,______,______,______,______,______,______}},
		{"ldx",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x00ce,0x00de,0x00fe,0x00ee,0xcdee,______,______,______,______,______,______,______}},
		{"ldy",		M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x18ce,0x18de,0x18fe,0x1aee,0x18ee,______,______,______,______,______,______,______}},
		{"lsla",	M_INHERENT,																	{0x0048,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"lslb",	M_INHERENT,																	{0x0058,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"lsl",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0078,0x0068,0x1868,______,______,______,______,______,______,______}},
		{"lsld",	M_INHERENT,																	{0x0005,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"lsra",	M_INHERENT,																	{0x0044,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"lsrb",	M_INHERENT,																	{0x0054,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"lsr",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0074,0x0064,0x1864,______,______,______,______,______,______,______}},
		{"lsrd",	M_INHERENT,																	{0x0004,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"mul",		M_INHERENT,																	{0x003d,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"nega",	M_INHERENT,																	{0x0040,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"negb",	M_INHERENT,																	{0x0050,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"neg",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0070,0x0060,0x1860,______,______,______,______,______,______,______}},
		{"nop",		M_INHERENT,																	{0x0001,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"oraa",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x008a,______,0x009a,0x00ba,0x00aa,0x18aa,______,______,______,______,______,______,______}},
		{"orab",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00ca,______,0x00da,0x00fa,0x00ea,0x18ea,______,______,______,______,______,______,______}},
		{"psha",	M_INHERENT,																	{0x0036,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pshb",	M_INHERENT,																	{0x0037,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pshx",	M_INHERENT,																	{0x003c,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pshy",	M_INHERENT,																	{0x183c,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pula",	M_INHERENT,																	{0x0032,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pulb",	M_INHERENT,																	{0x0033,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"pulx",	M_INHERENT,																	{0x0038,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"puly",	M_INHERENT,																	{0x1838,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"rola",	M_INHERENT,																	{0x0049,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"rolb",	M_INHERENT,																	{0x0059,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"rol",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0079,0x0069,0x1869,______,______,______,______,______,______,______}},
		{"rora",	M_INHERENT,																	{0x0046,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"rorb",	M_INHERENT,																	{0x0056,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"ror",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x0076,0x0066,0x1866,______,______,______,______,______,______,______}},
		{"rti",		M_INHERENT,																	{0x003b,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"rts",		M_INHERENT,																	{0x0039,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"sba",		M_INHERENT,																	{0x0010,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"sbca",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0082,______,0x0092,0x00b2,0x00a2,0x18a2,______,______,______,______,______,______,______}},
		{"sbcb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c2,______,0x00d2,0x00f2,0x00e2,0x18e2,______,______,______,______,______,______,______}},
		{"sec",		M_INHERENT,																	{0x000d,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"sei",		M_INHERENT,																	{0x000f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"sev",		M_INHERENT,																	{0x000b,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"staa",	M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x0097,0x00b7,0x00a7,0x18a7,______,______,______,______,______,______,______}},
		{"stab",	M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x00d7,0x00f7,0x00e7,0x18e7,______,______,______,______,______,______,______}},
		{"std",		M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x00dd,0x00fd,0x00ed,0x18ed,______,______,______,______,______,______,______}},
		{"stop",	M_INHERENT,																	{0x00cf,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"sts",		M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x009f,0x00bf,0x00af,0x18af,______,______,______,______,______,______,______}},
		{"stx",		M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x00df,0x00ff,0x00ef,0xcdef,______,______,______,______,______,______,______}},
		{"sty",		M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,								{______,______,______,0x18df,0x18ff,0x1aef,0x18ef,______,______,______,______,______,______,______}},
		{"suba",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x0080,______,0x0090,0x00b0,0x00a0,0x18a0,______,______,______,______,______,______,______}},
		{"subb",	M_IMMEDIATE8|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,0x00c0,______,0x00d0,0x00f0,0x00e0,0x18e0,______,______,______,______,______,______,______}},
		{"subd",	M_IMMEDIATE16|M_DIRECT|M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,					{______,______,0x0083,0x0093,0x00b3,0x00a3,0x18a3,______,______,______,______,______,______,______}},
		{"swi",		M_INHERENT,																	{0x003f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tab",		M_INHERENT,																	{0x0016,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tap",		M_INHERENT,																	{0x0006,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tba",		M_INHERENT,																	{0x0017,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"test",	M_INHERENT,																	{0x0000,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tpa",		M_INHERENT,																	{0x0007,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tsta",	M_INHERENT,																	{0x004d,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tstb",	M_INHERENT,																	{0x005d,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tst",		M_EXTENDED|M_INDEXED_X|M_INDEXED_Y,											{______,______,______,______,0x007d,0x006d,0x186d,______,______,______,______,______,______,______}},
		{"tsx",		M_INHERENT,																	{0x0030,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tsy",		M_INHERENT,																	{0x1830,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"txs",		M_INHERENT,																	{0x0035,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"tys",		M_INHERENT,																	{0x1835,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"wai",		M_INHERENT,																	{0x003e,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"xgdx",	M_INHERENT,																	{0x008f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
		{"xgdy",	M_INHERENT,																	{0x188f,______,______,______,______,______,______,______,______,______,______,______,______,______}},
	};
//																								  inh    imm8   imm16    dir    ext   indx   indy   rel    bdir   binx   biny   bdr    bix    biy

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

static bool ParseIndexX(const char *line,unsigned int *lineIndex)
// see if next thing on the line is index register X
// return true if so, false otherwise
{
	if(line[*lineIndex]=='X'||line[*lineIndex]=='x')
	{
		if(!IsLabelChar(line[(*lineIndex)+1]))
		{
			(*lineIndex)++;						// step over the register
			return(true);
		}
	}
	return(false);
}

static bool ParseIndexY(const char *line,unsigned int *lineIndex)
// see if next thing on the line is index register Y
// return true if so, false otherwise
{
	if(line[*lineIndex]=='Y'||line[*lineIndex]=='y')
	{
		if(!IsLabelChar(line[(*lineIndex)+1]))
		{
			(*lineIndex)++;						// step over the register
			return(true);
		}
	}
	return(false);
}

// opcode handling for 68HC11

enum
{
	POT_INDEX_X,
	POT_INDEX_Y,
	POT_IMMEDIATE,
	POT_VALUE,
};

static bool ParseOperandElement(const char *line,unsigned int *lineIndex,unsigned int *type,int *value,bool *unresolved)
// Try to parse an element of the operand and determine its type
// return true if the parsing succeeds
{
	if(ParseImmediatePreamble(line,lineIndex))		// an immediate operand?
	{
		if(ParseExpression(line,lineIndex,value,unresolved))
		{
			*type=POT_IMMEDIATE;
			return(true);
		}
	}
	else if(ParseIndexX(line,lineIndex))
	{
		*type=POT_INDEX_X;
		return(true);
	}
	else if(ParseIndexY(line,lineIndex))
	{
		*type=POT_INDEX_Y;
		return(true);
	}
	else if(ParseExpression(line,lineIndex,value,unresolved))
	{
		*type=POT_VALUE;
		return(true);
	}
	return(false);
}

static bool WriteOpcode(opcode_t opcode, LISTING_RECORD *listingRecord)
{
	bool
		fail = false;

	if(opcode & 0xff00)		// if it has a pre-byte (opcode > 0xff) put out both bytes
	{
		fail = !GenerateByte((opcode>>8)&0xff,listingRecord);
	}
	if(!fail)
	{
		fail = !GenerateByte(opcode&0xff,listingRecord);
	}
	return(!fail);
}

static bool HandleImmediate(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an immediate addressing mode was located
{
	bool
		fail=false;

	if(opcode->typeMask&M_IMMEDIATE8)			// 8-bit immediate value
	{
		CheckByteRange(value,true,true);
		fail = !WriteOpcode(opcode->baseOpcode[OT_IMMEDIATE8],listingRecord);
		if(!fail)
		{
			fail=!GenerateByte(value,listingRecord);
		}
	}
	else if(opcode->typeMask&M_IMMEDIATE16)		// 16-bit immediate value
	{
		CheckWordRange(value,true,true);
		fail = !WriteOpcode(opcode->baseOpcode[OT_IMMEDIATE16],listingRecord);
		if(!fail)
		{
			fail = !GenerateByte((value>>8)&0xff,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(value&0xff,listingRecord);
			}
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleRelative(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a relative operand has been parsed
{
	bool
		fail;
	int
		offset;

	fail = !WriteOpcode(opcode->baseOpcode[OT_RELATIVE],listingRecord);
	if(!fail)
	{
		offset=0;
		if(!unresolved&&currentSegment)
		{
			offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
			Check8RelativeRange(offset,true,true);
		}
		fail=!GenerateByte(offset,listingRecord);
	}
	return(!fail);
}

static bool HandleDirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with direct mode output only
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_DIRECT)
	{
		CheckUnsignedByteRange(value,true,true);
		fail = !WriteOpcode(opcode->baseOpcode[OT_DIRECT],listingRecord);
		if(!fail)
		{
			fail=!GenerateByte(value,listingRecord);
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleExtended(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with extended mode output only
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_EXTENDED)
	{
		CheckUnsignedWordRange(value,true,true);
		fail = !WriteOpcode(opcode->baseOpcode[OT_EXTENDED],listingRecord);
		if(!fail)
		{
			fail = !GenerateByte(value>>8,listingRecord);
			if(!fail)
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleDirectOrExtended(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a direct or extended address has been parsed
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_DIRECT)
	{
		if(((value>=0)&&(value<256))||!(opcode->typeMask&M_EXTENDED))
		{
			fail=!HandleDirect(opcode,value,unresolved,listingRecord);
		}
		else
		{
			fail=!HandleExtended(opcode,value,unresolved,listingRecord);
		}
	}
	else
	{
		fail=!HandleExtended(opcode,value,unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleSingleAddress(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a lone address has been parsed
// This means it is either a relative address (rr), direct (dd), or extended (hhll)
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_RELATIVE)
	{
		fail=!HandleRelative(opcode,value,unresolved,listingRecord);
	}
	else if(opcode->typeMask&(M_DIRECT|M_EXTENDED))
	{
		fail=!HandleDirectOrExtended(opcode,value,unresolved,listingRecord);
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleIndexedX(OPCODE *opcode,const char *line,unsigned int *lineIndex,int value1,bool unresolved,LISTING_RECORD *listingRecord)
// indexed x modes:
//  indexed_x:				op	ff,x
//  bit_indexed_x:			op	ff,x,mm
//  bit_indexed_x_relative:	op	ff,x,mm,rr
{
	bool fail = false;
	int value2;
	int value3;
	int offset;
	bool unresolved2;
	bool unresolved3;
	unsigned int elementType;

	if(ParseCommaSeparator(line,lineIndex))		// look for a second separator
	{
		if(ParseOperandElement(line,lineIndex,&elementType,&value2,&unresolved2) && (elementType == POT_VALUE) )
		{
			if(ParseCommaSeparator(line,lineIndex))	// look for a third separator
			{
				if(ParseOperandElement(line,lineIndex,&elementType,&value3,&unresolved3) && (elementType == POT_VALUE) &&
					ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_INDEXED_X_RELATIVE )
				{
					fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_INDEXED_X_RELATIVE],listingRecord);
					if(!fail)
					{
						CheckUnsignedByteRange(value1,true,true);		// bit indexed x relative
						fail=!GenerateByte(value1,listingRecord);
						if(!fail)
						{
							CheckUnsignedByteRange(value2,true,true);
							fail=!GenerateByte(value2,listingRecord);
						}
						if(!fail)
						{
							offset=0;
							if(currentSegment)
							{
								offset=value3-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
								Check8RelativeRange(offset,true,true);
							}
							fail=!GenerateByte(offset,listingRecord);
						}
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else if(ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_INDEXED_X)			// bit indexed x
			{
				CheckUnsignedByteRange(value1,true,true);
				CheckUnsignedByteRange(value2,true,true);
				fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_INDEXED_X],listingRecord);
				if(!fail)
				{
					fail=!GenerateByte(value1,listingRecord);
					if(!fail)
					{
						fail=!GenerateByte(value2,listingRecord);
					}
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else if(ParseComment(line,lineIndex))
	{
		if(opcode->typeMask&M_INDEXED_X)
		{
			CheckUnsignedByteRange(value1,true,true);
			fail = !WriteOpcode(opcode->baseOpcode[OT_INDEXED_X],listingRecord);
			if(!fail)
			{
				fail=!GenerateByte(value1,listingRecord);
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}


static bool HandleIndexedY(OPCODE *opcode,const char *line,unsigned int *lineIndex,int value1,bool unresolved,LISTING_RECORD *listingRecord)
// indexed y modes:
//  indexed_y:				op	ff,y
//  bit_indexed_y:			op	ff,y,mm
//  bit_indexed_y_relative:	op	ff,y,mm,rr
{
	bool fail = false;
	int value2;
	int value3;
	int offset;
	bool unresolved2;
	bool unresolved3;
	unsigned int elementType;

	if(ParseCommaSeparator(line,lineIndex))		// look for a second separator
	{
		if(ParseOperandElement(line,lineIndex,&elementType,&value2,&unresolved2) && (elementType == POT_VALUE) )
		{
			if(ParseCommaSeparator(line,lineIndex))	// look for a third separator
			{
				if(ParseOperandElement(line,lineIndex,&elementType,&value3,&unresolved3) && (elementType == POT_VALUE) &&
					ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_INDEXED_Y_RELATIVE )
				{
					fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_INDEXED_Y_RELATIVE],listingRecord);
					if(!fail)
					{
						CheckUnsignedByteRange(value1,true,true);		// bit indexed y relative
						fail=!GenerateByte(value1,listingRecord);
						if(!fail)
						{
							CheckUnsignedByteRange(value2,true,true);
							fail=!GenerateByte(value2,listingRecord);
						}
						if(!fail)
						{
							offset=0;
							if(currentSegment)
							{
								offset=value3-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
								Check8RelativeRange(offset,true,true);
							}
							fail=!GenerateByte(offset,listingRecord);
						}
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else if(ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_INDEXED_Y)			// bit indexed y
			{
				CheckUnsignedByteRange(value1,true,true);
				CheckUnsignedByteRange(value2,true,true);
				fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_INDEXED_Y],listingRecord);
				if(!fail)
				{
					fail=!GenerateByte(value1,listingRecord);
					if(!fail)
					{
						fail=!GenerateByte(value2,listingRecord);
					}
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else if(ParseComment(line,lineIndex))
	{
		if(opcode->typeMask&M_INDEXED_Y)
		{
			fail = !WriteOpcode(opcode->baseOpcode[OT_INDEXED_Y],listingRecord);
			if(!fail)
			{
				CheckUnsignedByteRange(value1,true,true);
				fail=!GenerateByte(value1,listingRecord);
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}


static bool HandleBitDirect(OPCODE *opcode,const char *line,unsigned int *lineIndex,int value1, int value2,LISTING_RECORD *listingRecord)
{
	bool fail = false;
	int value3;
	bool unresolved3;
	unsigned int elementType;
	int offset;

	if(ParseCommaSeparator(line,lineIndex))		// separator indicates a third value
	{
		if(ParseOperandElement(line,lineIndex,&elementType,&value3,&unresolved3) && elementType==POT_VALUE)
		{
			if(ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_DIRECT_RELATIVE)
			{
				fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_DIRECT_RELATIVE],listingRecord);
				if(!fail)
				{
					CheckUnsignedByteRange(value1,true,true);
					fail=!GenerateByte(value1,listingRecord);
					if(!fail)
					{
						CheckUnsignedByteRange(value2,true,true);
						fail=!GenerateByte(value2,listingRecord);
						if(!fail)
						{
							offset=0;
							if(currentSegment)
							{
								offset=value3-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
								Check8RelativeRange(offset,true,true);
							}
							fail=!GenerateByte(offset,listingRecord);
						}
					}
				}
			}
			else
			{
				ReportBadOperands();
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else if(ParseComment(line,lineIndex) && opcode->typeMask&M_BIT_DIRECT)
	{
		fail = !WriteOpcode(opcode->baseOpcode[OT_BIT_DIRECT],listingRecord);
		if(!fail)
		{
			CheckUnsignedByteRange(value1,true,true);
			fail=!GenerateByte(value1,listingRecord);
			if(!fail)
			{
				CheckUnsignedByteRange(value2,true,true);
				fail=!GenerateByte(value2,listingRecord);
			}
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);

}


static bool HandleFirstValue(OPCODE *opcode,const char *line,unsigned int *lineIndex,int value1,bool unresolved1,LISTING_RECORD *listingRecord)
// a non-immediate value was parsed, so see what else we can find
{
	bool
		fail;
	unsigned int
		elementType;
	int
		value2;
	bool
		unresolved2;

	fail=false;
	if(ParseCommaSeparator(line,lineIndex))		// make sure the next thing separates operands
	{
		if(ParseOperandElement(line,lineIndex,&elementType,&value2,&unresolved2))
		{
			switch(elementType)
			{
				case POT_INDEX_X:							// second operand was X
					return(HandleIndexedX(opcode,line,lineIndex,value1,unresolved1,listingRecord));
					break;

				case POT_INDEX_Y:							// second operand was Y
					return(HandleIndexedY(opcode,line,lineIndex,value1,unresolved1,listingRecord));
					break;

				case POT_VALUE:
					return(HandleBitDirect(opcode,line,lineIndex,value1,value2,listingRecord));
					break;

				default:
					ReportBadOperands();
					break;
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static OPCODE *MatchOpcode(const char *string)
// match opcodes for this processor, return NULL if none matched
{
	return((OPCODE *)STFindDataForNameNoCase(opcodeSymbols,string));
}

static bool AttemptOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// look at the type of opcode available, parse operands as allowed
// return false only if there was a 'hard' error
//
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
		elementType;
	int
		value;
	bool
		unresolved;
	bool
		indexedMode;

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
				indexedMode=ParseCommaSeparator(line,lineIndex);		// if true mode must be indexed
				if(ParseOperandElement(line,lineIndex,&elementType,&value,&unresolved))
				{
					switch(elementType)
					{
						case POT_IMMEDIATE:
							if(!indexedMode && ParseComment(line,lineIndex))
							{
								result=HandleImmediate(opcode,value,unresolved,listingRecord);
							}
							else
							{
								ReportBadOperands();
							}
							break;

						case POT_INDEX_X:
							value = 0;
							result=HandleIndexedX(opcode,line,lineIndex,value,unresolved,listingRecord);
							break;

						case POT_INDEX_Y:
							value = 0;
							result=HandleIndexedY(opcode,line,lineIndex,value,unresolved,listingRecord);
							break;

						case POT_VALUE:
							if(!indexedMode && ParseComment(line,lineIndex))			// if it's followed by comment/EOL, then its a single address
							{
								result=HandleSingleAddress(opcode,value,unresolved,listingRecord);
							}
							else
							{
								result=HandleFirstValue(opcode,line,lineIndex,value,unresolved,listingRecord);
							}
							break;
					}
				}
				else
				{
					ReportBadOperands();
				}
			}
			else
			{
				if(opcode->typeMask&M_INHERENT)
				{
					result=WriteOpcode(opcode->baseOpcode[OT_INHERENT],listingRecord);
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
// If so, handle it here and return true.
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
	STDisposeSymbolTable(opcodeSymbols);
	STDisposeSymbolTable(pseudoOpcodeSymbols);
}

static bool InitFamily()
// initialize symbol table
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
					return(true);
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
	processorFamily("Motorola 68hc11",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"68hc11",NULL),
	};
