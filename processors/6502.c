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


// Generate code for Rockwell 6502 (with or without 65C02 enhancements)

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcode6502Symbols,
	*opcode65C02Symbols;

static PROCESSOR
	*currentProcessor;

// enumerated addressing modes

#define	OT_IMPLIED				0								// no operands
#define	OT_IMMEDIATE			1								// #xx
#define	OT_ZP					2								// xx
#define	OT_ZP_OFF_X				3								// xx,X
#define	OT_ZP_OFF_Y				4								// xx,Y
#define	OT_ZP_INDIRECT_OFF_X	5								// (xx,X)
#define	OT_ZP_INDIRECT_OFF_Y	6								// (xx),Y
#define	OT_ZP_INDIRECT			7								// (xx)
#define	OT_EXTENDED				8								// xxxx
#define	OT_EXTENDED_OFF_X		9								// xxxx,X
#define	OT_EXTENDED_OFF_Y		10								// xxxx,Y
#define	OT_EXTENDED_INDIRECT	11								// (xxxx)
#define	OT_RELATIVE				12								// one byte relative offset
#define	OT_IMPLIED_2			13								// two-byte implied opcode (second byte is ignored)

#define	OT_NUM					OT_IMPLIED_2+1					// number of addressing modes

// masks for the various addressing modes

#define	M_IMPLIED				(1<<OT_IMPLIED)
#define	M_IMMEDIATE				(1<<OT_IMMEDIATE)
#define	M_ZP					(1<<OT_ZP)
#define	M_ZP_OFF_X				(1<<OT_ZP_OFF_X)
#define	M_ZP_OFF_Y				(1<<OT_ZP_OFF_Y)
#define	M_ZP_INDIRECT_OFF_X		(1<<OT_ZP_INDIRECT_OFF_X)
#define	M_ZP_INDIRECT_OFF_Y		(1<<OT_ZP_INDIRECT_OFF_Y)
#define	M_ZP_INDIRECT			(1<<OT_ZP_INDIRECT)
#define	M_EXTENDED				(1<<OT_EXTENDED)
#define	M_EXTENDED_OFF_X		(1<<OT_EXTENDED_OFF_X)
#define	M_EXTENDED_OFF_Y		(1<<OT_EXTENDED_OFF_Y)
#define	M_EXTENDED_INDIRECT		(1<<OT_EXTENDED_INDIRECT)
#define	M_RELATIVE				(1<<OT_RELATIVE)
#define	M_IMPLIED_2				(1<<OT_IMPLIED_2)

struct OPCODE
{
	const char
		*name;
	unsigned int
		typeMask;
	unsigned char
		baseOpcode[OT_NUM];
};

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"db",		HandleDB},
		{"dc.b",	HandleDB},
		{"dw",		HandleLEDW},		// words are little endian
		{"dc.w",	HandleLEDW},
		{"ds",		HandleDS},
		{"ds.b",	HandleDS},
		{"ds.w",	HandleDSW},
		{"incbin",	HandleIncbin},
	};

// These macros are used to make the opcode table creation simpler and less error prone
// Entries which are passed as whitespace are not encoded into typeMask
// Entries which are non-whitespace have a bit set in typeMask

#define	OP_FLAG(a,mask) ((sizeof(#a)>1)?mask:0)
#define	OP_VAL(a) (a+0)

// This macro creates the typeFlags and baseOpcode list. For each non-white entry in the baseOpcode
// list, a bit is set in typeFlags.
#define	OP_ENTRY(a,b,c,d,e,f,g,h,i,j,k,l,m,n) OP_FLAG(a,M_IMPLIED)|OP_FLAG(b,M_IMMEDIATE)|OP_FLAG(c,M_ZP)|OP_FLAG(d,M_ZP_OFF_X)|OP_FLAG(e,M_ZP_OFF_Y)|OP_FLAG(f,M_ZP_INDIRECT_OFF_X)|OP_FLAG(g,M_ZP_INDIRECT_OFF_Y)|OP_FLAG(h,M_ZP_INDIRECT)|OP_FLAG(i,M_EXTENDED)|OP_FLAG(j,M_EXTENDED_OFF_X)|OP_FLAG(k,M_EXTENDED_OFF_Y)|OP_FLAG(l,M_EXTENDED_INDIRECT)|OP_FLAG(m,M_RELATIVE)|OP_FLAG(n,M_IMPLIED_2),{OP_VAL(a),OP_VAL(b),OP_VAL(c),OP_VAL(d),OP_VAL(e),OP_VAL(f),OP_VAL(g),OP_VAL(h),OP_VAL(i),OP_VAL(j),OP_VAL(k),OP_VAL(l),OP_VAL(m),OP_VAL(n)}

static OPCODE
	opcodes6502[]=
	{
//							 imp  imm  zp	zpx	 zpy  indx indy (zp) ext  extx exty (ext) rel impl2
		{"adc",		OP_ENTRY(	0,0x69,0x65,0x75,	0,0x61,0x71,   0,0x6D,0x7D,0x79,   0,	0,	 0)},
		{"and",		OP_ENTRY(	0,0x29,0x25,0x35,	0,0x21,0x31,   0,0x2D,0x3D,0x39,   0,	0,	 0)},
		{"asl",		OP_ENTRY(0x0A,	 0,0x06,0x16,	0,	 0,	  0,   0,0x0E,0x1E,	  0,   0,	0,	 0)},
		{"bcc",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x90,	 0)},
		{"bcs",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xB0,	 0)},
		{"beq",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xF0,	 0)},
		{"bit",		OP_ENTRY(	0,	 0,0x24,   0,	0,	 0,	  0,   0,0x2C,	 0,	  0,   0,	0,	 0)},
		{"bmi",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x30,	 0)},
		{"bne",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xD0,	 0)},
		{"bpl",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x10,	 0)},
		{"brk",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,0x00)},
		{"bvc",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x50,	 0)},
		{"bvs",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x70,	 0)},
		{"clc",		OP_ENTRY(0x18,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cld",		OP_ENTRY(0xD8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cli",		OP_ENTRY(0x58,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"clv",		OP_ENTRY(0xB8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cmp",		OP_ENTRY(	0,0xC9,0xC5,0xD5,	0,0xC1,0xD1,   0,0xCD,0xDD,0xD9,   0,	0,	 0)},
		{"cpx",		OP_ENTRY(	0,0xE0,0xE4,   0,	0,	 0,	  0,   0,0xEC,	 0,	  0,   0,	0,	 0)},
		{"cpy",		OP_ENTRY(	0,0xC0,0xC4,   0,	0,	 0,	  0,   0,0xCC,	 0,	  0,   0,	0,	 0)},
		{"dec",		OP_ENTRY(	0,	 0,0xC6,0xD6,	0,	 0,	  0,   0,0xCE,0xDE,	  0,   0,	0,	 0)},
		{"dex",		OP_ENTRY(0xCA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"dey",		OP_ENTRY(0x88,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"eor",		OP_ENTRY(	0,0x49,0x45,0x55,	0,0x41,0x51,   0,0x4D,0x5D,0x59,   0,	0,	 0)},
		{"inc",		OP_ENTRY(	0,	 0,0xE6,0xF6,	0,	 0,	  0,   0,0xEE,0xFE,	  0,   0,	0,	 0)},
		{"inx",		OP_ENTRY(0xE8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"iny",		OP_ENTRY(0xC8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"jmp",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x4C,	 0,	  0,0x6C,	0,	 0)},
		{"jsr",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x20,	 0,	  0,   0,	0,	 0)},
		{"lda",		OP_ENTRY(	0,0xA9,0xA5,0xB5,	0,0xA1,0xB1,   0,0xAD,0xBD,0xB9,   0,	0,	 0)},
		{"ldx",		OP_ENTRY(	0,0xA2,0xA6,   0,0xB6,	 0,	  0,   0,0xAE,	 0,0xBE,   0,	0,	 0)},
		{"ldy",		OP_ENTRY(	0,0xA0,0xA4,0xB4,	0,	 0,	  0,   0,0xAC,0xBC,	  0,   0,	0,	 0)},
		{"lsr",		OP_ENTRY(0x4A,	 0,0x46,0x56,	0,	 0,	  0,   0,0x4E,0x5E,	  0,   0,	0,	 0)},
		{"nop",		OP_ENTRY(0xEA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"ora",		OP_ENTRY(	0,0x09,0x05,0x15,	0,0x01,0x11,   0,0x0D,0x1D,0x19,   0,	0,	 0)},
		{"pha",		OP_ENTRY(0x48,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"php",		OP_ENTRY(0x08,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"pla",		OP_ENTRY(0x68,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"plp",		OP_ENTRY(0x28,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"rol",		OP_ENTRY(0x2A,	 0,0x26,0x36,	0,	 0,	  0,   0,0x2E,0x3E,	  0,   0,	0,	 0)},
		{"ror",		OP_ENTRY(0x6A,	 0,0x66,0x76,	0,	 0,	  0,   0,0x6E,0x7E,	  0,   0,	0,	 0)},
		{"rti",		OP_ENTRY(0x40,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"rts",		OP_ENTRY(0x60,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sbc",		OP_ENTRY(	0,0xE9,0xE5,0xF5,	0,0xE1,0xF1,   0,0xED,0xFD,0xF9,   0,	0,	 0)},
		{"sec",		OP_ENTRY(0x38,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sed",		OP_ENTRY(0xF8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sei",		OP_ENTRY(0x78,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sta",		OP_ENTRY(	0,	 0,0x85,0x95,	0,0x81,0x91,   0,0x8D,0x9D,0x99,   0,	0,	 0)},
		{"stx",		OP_ENTRY(	0,	 0,0x86,   0,0x96,	 0,	  0,   0,0x8E,	 0,	  0,   0,	0,	 0)},
		{"sty",		OP_ENTRY(	0,	 0,0x84,0x94,	0,	 0,	  0,   0,0x8C,	 0,	  0,   0,	0,	 0)},
		{"tax",		OP_ENTRY(0xAA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"tay",		OP_ENTRY(0xA8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"tsx",		OP_ENTRY(0xBA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"txa",		OP_ENTRY(0x8A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"txs",		OP_ENTRY(0x9A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"tya",		OP_ENTRY(0x98,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
	};

// opcodes for the 65C02
static OPCODE
	opcodes65C02[]=
	{
//							 imp  imm  zp	zpx	 zpy  indx indy (zp) ext  extx exty indw rel  impl2
		{"adc",		OP_ENTRY(	0,0x69,0x65,0x75,	0,0x61,0x71,0x72,0x6D,0x7D,0x79,   0,	0,	 0)},
		{"and",		OP_ENTRY(	0,0x29,0x25,0x35,	0,0x21,0x31,0x32,0x2D,0x3D,0x39,   0,	0,	 0)},
		{"asl",		OP_ENTRY(0x0A,	 0,0x06,0x16,	0,	 0,	  0,   0,0x0E,0x1E,	  0,   0,	0,	 0)},
		{"bcc",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x90,	 0)},
		{"bcs",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xB0,	 0)},
		{"beq",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xF0,	 0)},
		{"bit",		OP_ENTRY(	0,0x89,0x24,0x34,	0,	 0,	  0,   0,0x2C,0x3C,	  0,   0,	0,	 0)},
		{"bmi",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x30,	 0)},
		{"bne",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0xD0,	 0)},
		{"bpl",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x10,	 0)},
		{"bra",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x80,	 0)},
		{"brk",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,0x00)},
		{"bvc",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x50,	 0)},
		{"bvs",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x70,	 0)},
		{"clc",		OP_ENTRY(0x18,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cld",		OP_ENTRY(0xD8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cli",		OP_ENTRY(0x58,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"clv",		OP_ENTRY(0xB8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"cmp",		OP_ENTRY(	0,0xC9,0xC5,0xD5,	0,0xC1,0xD1,0xD2,0xCD,0xDD,0xD9,   0,	0,	 0)},
		{"cpx",		OP_ENTRY(	0,0xE0,0xE4,   0,	0,	 0,	  0,   0,0xEC,	 0,	  0,   0,	0,	 0)},
		{"cpy",		OP_ENTRY(	0,0xC0,0xC4,   0,	0,	 0,	  0,   0,0xCC,	 0,	  0,   0,	0,	 0)},
		{"dea",		OP_ENTRY(0x3A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"dec",		OP_ENTRY(	0,	 0,0xC6,0xD6,	0,	 0,	  0,   0,0xCE,0xDE,	  0,   0,	0,	 0)},
		{"dex",		OP_ENTRY(0xCA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"dey",		OP_ENTRY(0x88,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"eor",		OP_ENTRY(	0,0x49,0x45,0x55,	0,0x41,0x51,0x52,0x4D,0x5D,0x59,   0,	0,	 0)},
		{"ina",		OP_ENTRY(0x1A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"inc",		OP_ENTRY(	0,	 0,0xE6,0xF6,	0,	 0,	  0,   0,0xEE,0xFE,	  0,   0,	0,	 0)},
		{"inx",		OP_ENTRY(0xE8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"iny",		OP_ENTRY(0xC8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"jmp",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x4C,0x7C,	  0,0x6C,	0,	 0)},
		{"jsr",		OP_ENTRY(	0,	 0,	  0,   0,	0,	 0,	  0,   0,0x20,	 0,	  0,   0,	0,	 0)},
		{"lda",		OP_ENTRY(	0,0xA9,0xA5,0xB5,	0,0xA1,0xB1,0xB2,0xAD,0xBD,0xB9,   0,	0,	 0)},
		{"ldx",		OP_ENTRY(	0,0xA2,0xA6,   0,0xB6,	 0,	  0,   0,0xAE,	 0,0xBE,   0,	0,	 0)},
		{"ldy",		OP_ENTRY(	0,0xA0,0xA4,0xB4,	0,	 0,	  0,   0,0xAC,0xBC,	  0,   0,	0,	 0)},
		{"lsr",		OP_ENTRY(0x4A,	 0,0x46,0x56,	0,	 0,	  0,   0,0x4E,0x5E,	  0,   0,	0,	 0)},
		{"nop",		OP_ENTRY(0xEA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"ora",		OP_ENTRY(	0,0x09,0x05,0x15,	0,0x01,0x11,0x12,0x0D,0x1D,0x19,   0,	0,	 0)},
		{"pha",		OP_ENTRY(0x48,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"php",		OP_ENTRY(0x08,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"phx",		OP_ENTRY(0xDA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"phy",		OP_ENTRY(0x5A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"pla",		OP_ENTRY(0x68,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"plp",		OP_ENTRY(0x28,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"plx",		OP_ENTRY(0xFA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"ply",		OP_ENTRY(0x7A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"rol",		OP_ENTRY(0x2A,	 0,0x26,0x36,	0,	 0,	  0,   0,0x2E,0x3E,	  0,   0,	0,	 0)},
		{"ror",		OP_ENTRY(0x6A,	 0,0x66,0x76,	0,	 0,	  0,   0,0x6E,0x7E,	  0,   0,	0,	 0)},
		{"rti",		OP_ENTRY(0x40,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"rts",		OP_ENTRY(0x60,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sbc",		OP_ENTRY(	0,0xE9,0xE5,0xF5,	0,0xE1,0xF1,0xF2,0xED,0xFD,0xF9,   0,	0,	 0)},
		{"sec",		OP_ENTRY(0x38,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sed",		OP_ENTRY(0xF8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sei",		OP_ENTRY(0x78,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"sta",		OP_ENTRY(	0,	 0,0x85,0x95,	0,0x81,0x91,0x92,0x8D,0x9D,0x99,   0,	0,	 0)},
		{"stx",		OP_ENTRY(	0,	 0,0x86,   0,0x96,	 0,	  0,   0,0x8E,	 0,	  0,   0,	0,	 0)},
		{"sty",		OP_ENTRY(	0,	 0,0x84,0x94,	0,	 0,	  0,   0,0x8C,	 0,	  0,   0,	0,	 0)},
		{"stz",		OP_ENTRY(	0,	 0,0x64,0x74,	0,	 0,	  0,   0,0x9C,0x9E,	  0,   0,	0,	 0)},
		{"tax",		OP_ENTRY(0xAA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"tay",		OP_ENTRY(0xA8,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"trb",		OP_ENTRY(	0,	 0,0x14,   0,	0,	 0,	  0,   0,0x1C,	 0,	  0,   0,	0,	 0)},
		{"tsb",		OP_ENTRY(	0,	 0,0x04,   0,	0,	 0,	  0,   0,0x0C,	 0,	  0,   0,	0,	 0)},
		{"tsx",		OP_ENTRY(0xBA,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"txa",		OP_ENTRY(0x8A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"txs",		OP_ENTRY(0x9A,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},
		{"tya",		OP_ENTRY(0x98,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0)},

// bbr, bbs, rmb, smb should be added some day
//	BBR		#n,zp,addr	branch to addr if bit n of location zp is clear
//	BBS		#n,zp,addr	branch to addr if bit n of location zp is set
//	RMB		#n,zp		clear bit n of location zp
//	SMB		#n,zp		set bit n of location zp

//These might be written differently in some assemblers (n=0..7):

//	BBRn	zp,addr
//	BBSn	zp,addr
//	RMBn	zp
//	SMBn	zp

	};

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
// see if next thing on the line is the X register
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
// see if next thing on the line is the Y register
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

// opcode handling for 6502

enum
{
	POT_IMMEDIATE,
	POT_VALUE,					// extended, zero page, or relative xxxx or xx
	POT_VALUE_OFF_X,			// extended, or zero page			xxxx,X or xx,X
	POT_VALUE_OFF_Y,			// extended							xxxx,Y
	POT_INDIRECT,				// extended or zero page			(xxxx) or (xx)
	POT_ZP_INDIRECT_OFF_X,		// zero page						(xx,X)
	POT_ZP_INDIRECT_OFF_Y,		// zero page						(xx),Y
};

static bool ParseValueOperand(const char *line,unsigned int *lineIndex,unsigned int *type,int *value,bool *unresolved)
// parse the operand as a value with a possible offset
{
	if(ParseExpression(line,lineIndex,value,unresolved))
	{
		if(ParseComment(line,lineIndex))
		{
			*type=POT_VALUE;
			return(true);
		}
		else if(ParseCommaSeparator(line,lineIndex))
		{
			if(ParseIndexX(line,lineIndex))
			{
				if(ParseComment(line,lineIndex))
				{
					*type=POT_VALUE_OFF_X;
					return(true);
				}
			}
			else if(ParseIndexY(line,lineIndex))
			{
				if(ParseComment(line,lineIndex))
				{
					*type=POT_VALUE_OFF_Y;
					return(true);
				}
			}
		}
	}
	return(false);
}

static bool ParseIndirectValueOperand(const char *string,unsigned int *type,int *value,bool *unresolved)
// An indirect operand has been parsed
// the type tells what it looked like at the outer level
// The expression within the parenthesis still needs to be evaluated
{
	unsigned int
		expressionIndex;

	expressionIndex=0;
	if(ParseExpression(string,&expressionIndex,value,unresolved))
	{
		if(ParseComment(string,&expressionIndex))
		{
			return(true);		// done, expression makes sense, and we are at the end
		}
		else
		{
			if(*type==POT_INDIRECT)		// make sure there was no index register on the outside of the indirect address
			{
				if(ParseCommaSeparator(string,&expressionIndex))
				{
					if(ParseIndexX(string,&expressionIndex))
					{
						if(ParseComment(string,&expressionIndex))
						{
							*type=POT_ZP_INDIRECT_OFF_X;
							return(true);
						}
					}
				}
			}
		}
	}
	return(false);
}

static bool ParseOperand(const char *line,unsigned int *lineIndex,unsigned int *type,int *value,bool *unresolved)
// Try to parse an operand and determine its type
// return true if the parsing succeeds
{
	char
		string[MAX_STRING];
	unsigned int
		startIndex;

	if(ParseImmediatePreamble(line,lineIndex))		// an immediate operand?
	{
		if(ParseExpression(line,lineIndex,value,unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				*type=POT_IMMEDIATE;
				return(true);
			}
		}
		return(false);
	}
	else
	{
		startIndex=*lineIndex;							// remember where we are, because we may need to come back here
		if(ParseParentheticString(line,lineIndex,string))	// does this look like a possible indirect value?
		{
			if(ParseComment(line,lineIndex))
			{
				*type=POT_INDIRECT;
				*lineIndex=startIndex;
				return(ParseIndirectValueOperand(string,type,value,unresolved));
			}
			else if(ParseCommaSeparator(line,lineIndex))
			{
				if(ParseIndexY(line,lineIndex))
				{
					if(ParseComment(line,lineIndex))
					{
						*type=POT_ZP_INDIRECT_OFF_Y;
						return(ParseIndirectValueOperand(string,type,value,unresolved));
					}
				}
			}
			else
			{
				*lineIndex=startIndex;					// value does not look indirect, so try direct approach
				return(ParseValueOperand(line,lineIndex,type,value,unresolved));
			}
		}
		else
		{
			return(ParseValueOperand(line,lineIndex,type,value,unresolved));
		}
	}
	return(false);
}

static bool HandleImmediate(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// an immediate addressing mode was located
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_IMMEDIATE)
	{
		CheckByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_IMMEDIATE],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleZeroPage(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with zero page mode output only
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
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
		if(GenerateByte(opcode->baseOpcode[OT_EXTENDED],listingRecord))
		{
			if(GenerateByte(value&0xFF,listingRecord))
			{
				fail=!GenerateByte(value>>8,listingRecord);
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
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleZeroPageOrExtended(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a zero page or extended address has been parsed
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP)
	{
		if(((value>=0)&&(value<256))||!(opcode->typeMask&M_EXTENDED))
		{
			fail=!HandleZeroPage(opcode,value,unresolved,listingRecord);
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

static bool HandleRelative(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a relative operand has been parsed
{
	bool
		fail;
	unsigned int
		offset;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_RELATIVE],listingRecord))
	{
		offset=0;
		if(!unresolved&&currentSegment)
		{
			offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
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

static bool HandleValue(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a value has been parsed, it might be relative, zero page, or extended
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_RELATIVE)
	{
		fail=!HandleRelative(opcode,value,unresolved,listingRecord);
	}
	else
	{
		fail=!HandleZeroPageOrExtended(opcode,value,unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleZeroPageOffX(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with zero page offset by X
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_OFF_X)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP_OFF_X],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleExtendedOffX(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with extended mode offset by X
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_EXTENDED_OFF_X)
	{
		CheckUnsignedWordRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_EXTENDED_OFF_X],listingRecord))
		{
			if(GenerateByte(value&0xFF,listingRecord))
			{
				fail=!GenerateByte(value>>8,listingRecord);
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
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleValueOffX(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// possible extended or zeropage value offset by X
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_OFF_X)
	{
		if(((value>=0)&&(value<256))||!(opcode->typeMask&M_EXTENDED_OFF_X))
		{
			fail=!HandleZeroPageOffX(opcode,value,unresolved,listingRecord);
		}
		else
		{
			fail=!HandleExtendedOffX(opcode,value,unresolved,listingRecord);
		}
	}
	else
	{
		fail=!HandleExtendedOffX(opcode,value,unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleZeroPageOffY(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with zero page offset by Y
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_OFF_Y)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP_OFF_Y],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleExtendedOffY(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with extended mode offset by Y
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_EXTENDED_OFF_Y)
	{
		CheckUnsignedWordRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_EXTENDED_OFF_Y],listingRecord))
		{
			if(GenerateByte(value&0xFF,listingRecord))
			{
				fail=!GenerateByte(value>>8,listingRecord);
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
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleValueOffY(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// possible extended or zero page value offset by Y
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_OFF_Y)
	{
		if(((value>=0)&&(value<256))||!(opcode->typeMask&M_EXTENDED_OFF_Y))
		{
			fail=!HandleZeroPageOffY(opcode,value,unresolved,listingRecord);
		}
		else
		{
			fail=!HandleExtendedOffY(opcode,value,unresolved,listingRecord);
		}
	}
	else
	{
		fail=!HandleExtendedOffY(opcode,value,unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleZeroPageIndirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with zero page indirect output only
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_INDIRECT)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP_INDIRECT],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleExtendedIndirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// deal with extended indirect output only
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_EXTENDED_INDIRECT)
	{
		CheckUnsignedWordRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_EXTENDED_INDIRECT],listingRecord))
		{
			if(GenerateByte(value&0xFF,listingRecord))
			{
				fail=!GenerateByte(value>>8,listingRecord);
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
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleZeroPageOrExtendedIndirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a zero page or extended indirect address has been parsed
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_INDIRECT)
	{
		if(((value>=0)&&(value<256))||!(opcode->typeMask&M_EXTENDED_INDIRECT))
		{
			fail=!HandleZeroPageIndirect(opcode,value,unresolved,listingRecord);
		}
		else
		{
			fail=!HandleExtendedIndirect(opcode,value,unresolved,listingRecord);
		}
	}
	else
	{
		fail=!HandleExtendedIndirect(opcode,value,unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleIndirectOffX(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a indirect value offset by X has been parsed, it must be zero page
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_INDIRECT_OFF_X)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP_INDIRECT_OFF_X],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleIndirectOffY(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a indirect value offset by Y has been parsed, it must be zero page
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_ZP_INDIRECT_OFF_Y)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZP_INDIRECT_OFF_Y],listingRecord))
		{
			fail=!GenerateByte(value,listingRecord);
		}
		else
		{
			fail=true;
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
	OPCODE
		*result;

	result=(OPCODE *)STFindDataForNameNoCase(*((SYM_TABLE **)(currentProcessor->processorData)),string);
	return(result);
}

static bool AttemptOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// See if the next thing looks like an opcode for this processor.
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
		elementType;
	int
		value;
	bool
		unresolved;

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
				if(ParseOperand(line,lineIndex,&elementType,&value,&unresolved))
				{
					switch(elementType)
					{
						case POT_IMMEDIATE:
							result=HandleImmediate(opcode,value,unresolved,listingRecord);
							break;
						case POT_VALUE:
							result=HandleValue(opcode,value,unresolved,listingRecord);
							break;
						case POT_VALUE_OFF_X:
							result=HandleValueOffX(opcode,value,unresolved,listingRecord);
							break;
						case POT_VALUE_OFF_Y:
							result=HandleValueOffY(opcode,value,unresolved,listingRecord);
							break;
						case POT_INDIRECT:
							result=HandleZeroPageOrExtendedIndirect(opcode,value,unresolved,listingRecord);
							break;
						case POT_ZP_INDIRECT_OFF_X:
							result=HandleIndirectOffX(opcode,value,unresolved,listingRecord);
							break;
						case POT_ZP_INDIRECT_OFF_Y:
							result=HandleIndirectOffY(opcode,value,unresolved,listingRecord);
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
				if(opcode->typeMask&M_IMPLIED)
				{
					result=GenerateByte(opcode->baseOpcode[OT_IMPLIED],listingRecord);
				}
				else if(opcode->typeMask&M_IMPLIED_2)
				{
					if((result=GenerateByte(opcode->baseOpcode[OT_IMPLIED_2],listingRecord)))
					{
						result=GenerateByte(0x00,listingRecord);			// two-byte opcode, second byte is ignored
					}
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
	currentProcessor=processor;
	return(true);
}

static void DeselectProcessor(PROCESSOR *processor)
// A processor in this family is being deselected
{
}

static void UnInitFamily()
// undo what InitFamily did
{
	STDisposeSymbolTable(opcode65C02Symbols);
	STDisposeSymbolTable(opcode6502Symbols);
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
	currentProcessor=NULL;

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
			if((opcode6502Symbols=STNewSymbolTable(elementsof(opcodes6502))))
			{
				for(i=0;!fail&&(i<elementsof(opcodes6502));i++)
				{
					if(!STAddEntryAtEnd(opcode6502Symbols,opcodes6502[i].name,&opcodes6502[i]))
					{
						fail=true;
					}
				}
				if(!fail)
				{
					if((opcode65C02Symbols=STNewSymbolTable(elementsof(opcodes65C02))))
					{
						for(i=0;!fail&&(i<elementsof(opcodes65C02));i++)
						{
							if(!STAddEntryAtEnd(opcode65C02Symbols,opcodes65C02[i].name,&opcodes65C02[i]))
							{
								fail=true;
							}
						}
						if(!fail)
						{
							return(true);
						}
						STDisposeSymbolTable(opcode65C02Symbols);
					}
				}
				STDisposeSymbolTable(opcode6502Symbols);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Rockwell 6502",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"6502",&opcode6502Symbols),
		PROCESSOR(&processorFamily,"65c02",&opcode65C02Symbols),
	};
