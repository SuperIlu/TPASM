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

// Generate code for Motorola 6805, or the newer hc08's

#include	"include.h"

#define	SP_PREAMBLE	0x9E	// all opcodes which reference SP require this preamble

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols6805,
	*opcodeSymbols68hc08;

static PROCESSOR
	*currentProcessor;

// enumerated addressing modes

enum
{
	OT_INHERENT,			// no operands
	OT_IMM8,				// 8-bit immediate operand
	OT_IMM16,				// 16-bit immediate operand
	OT_DIRECT,				// one byte direct
	OT_EXTENDED,			// two byte absolute
	OT_INDEXED,				// indexed with no offset
	OT_OFF8_INDEXED,		// indexed with 8-bit offset
	OT_OFF16_INDEXED,		// indexed with 16-bit offset
	OT_OFF8_SP,				// stack pointer with 8-bit offset
	OT_OFF16_SP,			// stack pointer with 16-bit offset
	OT_BIT_DIRECT,			// one bit index, one byte of direct
	OT_IMM8_DIRECT,			// 8-bit immediate operand, one byte direct address
	OT_DIRECT_DIRECT,		// one byte direct address, one byte direct address
	OT_INDEXED_PI_DIRECT,	// indexed to direct with post-increment
	OT_DIRECT_INDEXED_PI,	// direct to indexed with post-increment
	OT_INDEXED_PI,			// indexed post-increment
	OT_OFF8_INDEXED_PI,		// indexed with 8 bit offset and post-increment
	OT_NUM					// number of addressing modes
};

// masks for the various addressing modes (all opcodes which use relative offsets are
// handled with a separate flag that tells us to expect a relative offset as the last
// parameter)

#define	M_INHERENT				(1<<OT_INHERENT)
#define	M_IMM8					(1<<OT_IMM8)
#define	M_IMM16					(1<<OT_IMM16)
#define	M_DIRECT				(1<<OT_DIRECT)
#define	M_EXTENDED				(1<<OT_EXTENDED)
#define	M_INDEXED				(1<<OT_INDEXED)
#define	M_OFF8_INDEXED			(1<<OT_OFF8_INDEXED)
#define	M_OFF16_INDEXED			(1<<OT_OFF16_INDEXED)
#define	M_OFF8_SP				(1<<OT_OFF8_SP)
#define	M_OFF16_SP				(1<<OT_OFF16_SP)
#define	M_BIT_DIRECT			(1<<OT_BIT_DIRECT)
#define	M_IMM8_DIRECT			(1<<OT_IMM8_DIRECT)
#define	M_DIRECT_DIRECT			(1<<OT_DIRECT_DIRECT)
#define	M_INDEXED_PI_DIRECT		(1<<OT_INDEXED_PI_DIRECT)
#define	M_DIRECT_INDEXED_PI		(1<<OT_DIRECT_INDEXED_PI)
#define	M_INDEXED_PI			(1<<OT_INDEXED_PI)
#define	M_OFF8_INDEXED_PI		(1<<OT_OFF8_INDEXED_PI)


// types of operands which we can discover while parsing
enum
{
	POT_INDEX,
	POT_INDEX_PI,						// index with post-increment (X+)
	POT_SP,
	POT_IMMEDIATE,
	POT_VALUE,
};

struct OPCODE
{
	const char
		*name;							// opcode name
	bool
		relativeMode;					// tells if this opcode requires a relative address as its last operand (in addition to any others that may be specified)
	unsigned int
		typeMask;						// mask of bits that tells which type of operands are allowed for this opcode
	unsigned char
		baseOpcode[OT_NUM];				// list of opcode values -- one for each possible operand type
};

struct OPERAND
{
	unsigned int
		type;			// type of operand located
	int
		value;			// first immediate value, or non immediate value
	bool
		unresolved;		// if value was an expression, this tells if it was unresolved
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

// These macros are used to make the opcode table creation simpler and less error prone
// Entries which are passed as whitespace are not encoded into typeMask
// Entries which are non-whitespace have a bit set in typeMask

#define	OP_FLAG(a,mask) ((sizeof(#a)>1)?mask:0)
#define	OP_VAL(a) (a+0)

// This macro creates the typeFlags and baseOpcode list. For each non-white entry in the baseOpcode
// list, a bit is set in typeFlags.
#define OP_ENTRY(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) OP_FLAG(a,M_INHERENT)|OP_FLAG(b,M_IMM8)|OP_FLAG(c,M_IMM16)|OP_FLAG(d,M_DIRECT)|OP_FLAG(e,M_EXTENDED)|OP_FLAG(f,M_INDEXED)|OP_FLAG(g,M_OFF8_INDEXED)|OP_FLAG(h,M_OFF16_INDEXED)|OP_FLAG(i,M_OFF8_SP)|OP_FLAG(j,M_OFF16_SP)|OP_FLAG(k,M_BIT_DIRECT)|OP_FLAG(l,M_IMM8_DIRECT)|OP_FLAG(m,M_DIRECT_DIRECT)|OP_FLAG(n,M_INDEXED_PI_DIRECT)|OP_FLAG(o,M_DIRECT_INDEXED_PI)|OP_FLAG(p,M_INDEXED_PI)|OP_FLAG(q,M_OFF8_INDEXED_PI),{OP_VAL(a),OP_VAL(b),OP_VAL(c),OP_VAL(d),OP_VAL(e),OP_VAL(f),OP_VAL(g),OP_VAL(h),OP_VAL(i),OP_VAL(j),OP_VAL(k),OP_VAL(l),OP_VAL(m),OP_VAL(n),OP_VAL(o),OP_VAL(p),OP_VAL(q)}

static OPCODE
	Opcodes6805[]=
	{
//								   inh  im8  im16 dir  ext  hx   8hx  16hx 8sp  16sp bd   im8d dd   hx+d dhx+ hx+  8hx+
		{"adc",		false,OP_ENTRY(   0,0xA9,   0,0xB9,0xC9,0xF9,0xE9,0xD9,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"add",		false,OP_ENTRY(   0,0xAB,   0,0xBB,0xCB,0xFB,0xEB,0xDB,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"and",		false,OP_ENTRY(   0,0xA4,   0,0xB4,0xC4,0xF4,0xE4,0xD4,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asl",		false,OP_ENTRY(   0,   0,   0,0x38,   0,0x78,0x68,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asla",	false,OP_ENTRY(0x48,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"aslx",	false,OP_ENTRY(0x58,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asr",		false,OP_ENTRY(   0,   0,   0,0x37,   0,0x77,0x67,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asra",	false,OP_ENTRY(0x47,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asrx",	false,OP_ENTRY(0x57,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bcc",		true ,OP_ENTRY(0x24,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr",	false,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x11,   0,   0,   0,   0,   0,   0)},
		{"bclr0",	false,OP_ENTRY(   0,   0,   0,0x11,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr1",	false,OP_ENTRY(   0,   0,   0,0x13,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr2",	false,OP_ENTRY(   0,   0,   0,0x15,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr3",	false,OP_ENTRY(   0,   0,   0,0x17,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr4",	false,OP_ENTRY(   0,   0,   0,0x19,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr5",	false,OP_ENTRY(   0,   0,   0,0x1B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr6",	false,OP_ENTRY(   0,   0,   0,0x1D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr7",	false,OP_ENTRY(   0,   0,   0,0x1F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bcs",		true ,OP_ENTRY(0x25,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"beq",		true ,OP_ENTRY(0x27,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhcc",	true ,OP_ENTRY(0x28,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhcs",	true ,OP_ENTRY(0x29,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhi",		true ,OP_ENTRY(0x22,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhs",		true ,OP_ENTRY(0x24,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bih",		true ,OP_ENTRY(0x2F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bil",		true ,OP_ENTRY(0x2E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bit",		false,OP_ENTRY(   0,0xA5,   0,0xB5,0xC5,0xF5,0xE5,0xD5,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"blo",		true ,OP_ENTRY(0x25,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bls",		true ,OP_ENTRY(0x23,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bmc",		true ,OP_ENTRY(0x2C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bmi",		true ,OP_ENTRY(0x2B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bms",		true ,OP_ENTRY(0x2D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bne",		true ,OP_ENTRY(0x26,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bpl",		true ,OP_ENTRY(0x2A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bra",		true ,OP_ENTRY(0x20,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr",	true ,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x01,   0,   0,   0,   0,   0,   0)},
		{"brclr0",	true ,OP_ENTRY(   0,   0,   0,0x01,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr1",	true ,OP_ENTRY(   0,   0,   0,0x03,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr2",	true ,OP_ENTRY(   0,   0,   0,0x05,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr3",	true ,OP_ENTRY(   0,   0,   0,0x07,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr4",	true ,OP_ENTRY(   0,   0,   0,0x09,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr5",	true ,OP_ENTRY(   0,   0,   0,0x0B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr6",	true ,OP_ENTRY(   0,   0,   0,0x0D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr7",	true ,OP_ENTRY(   0,   0,   0,0x0F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brn",		true ,OP_ENTRY(0x21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset",	true ,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x00,   0,   0,   0,   0,   0,   0)},
		{"brset0",	true ,OP_ENTRY(   0,   0,   0,0x00,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset1",	true ,OP_ENTRY(   0,   0,   0,0x02,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset2",	true ,OP_ENTRY(   0,   0,   0,0x04,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset3",	true ,OP_ENTRY(   0,   0,   0,0x06,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset4",	true ,OP_ENTRY(   0,   0,   0,0x08,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset5",	true ,OP_ENTRY(   0,   0,   0,0x0A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset6",	true ,OP_ENTRY(   0,   0,   0,0x0C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset7",	true ,OP_ENTRY(   0,   0,   0,0x0E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset",	false,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x10,   0,   0,   0,   0,   0,   0)},
		{"bset0",	false,OP_ENTRY(   0,   0,   0,0x10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset1",	false,OP_ENTRY(   0,   0,   0,0x12,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset2",	false,OP_ENTRY(   0,   0,   0,0x14,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset3",	false,OP_ENTRY(   0,   0,   0,0x16,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset4",	false,OP_ENTRY(   0,   0,   0,0x18,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset5",	false,OP_ENTRY(   0,   0,   0,0x1A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset6",	false,OP_ENTRY(   0,   0,   0,0x1C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset7",	false,OP_ENTRY(   0,   0,   0,0x1E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bsr",		true ,OP_ENTRY(0xAD,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clc",		false,OP_ENTRY(0x98,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cli",		false,OP_ENTRY(0x9A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clr",		false,OP_ENTRY(   0,   0,   0,0x3F,   0,0x7F,0x6F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clra",	false,OP_ENTRY(0x4F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clrx",	false,OP_ENTRY(0x5F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cmp",		false,OP_ENTRY(   0,0xA1,   0,0xB1,0xC1,0xF1,0xE1,0xD1,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"com",		false,OP_ENTRY(   0,   0,   0,0x33,   0,0x73,0x63,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"coma",	false,OP_ENTRY(0x43,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"comx",	false,OP_ENTRY(0x53,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cpx",		false,OP_ENTRY(   0,0xA3,   0,0xB3,0xC3,0xF3,0xE3,0xD3,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dec",		false,OP_ENTRY(   0,   0,   0,0x3A,   0,0x7A,0x6A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"deca",	false,OP_ENTRY(0x4A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"decx",	false,OP_ENTRY(0x5A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dex",		false,OP_ENTRY(0x5A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"eor",		false,OP_ENTRY(   0,0xA8,   0,0xB8,0xC8,0xF8,0xE8,0xD8,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"inc",		false,OP_ENTRY(   0,   0,   0,0x3C,   0,0x7C,0x6C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"inca",	false,OP_ENTRY(0x4C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"incx",	false,OP_ENTRY(0x5C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"inx",		false,OP_ENTRY(0x5C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"jmp",		false,OP_ENTRY(   0,   0,   0,0xBC,0xCC,0xFC,0xEC,0xDC,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"jsr",		false,OP_ENTRY(   0,   0,   0,0xBD,0xCD,0xFD,0xED,0xDD,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lda",		false,OP_ENTRY(   0,0xA6,   0,0xB6,0xC6,0xF6,0xE6,0xD6,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ldx",		false,OP_ENTRY(   0,0xAE,   0,0xBE,0xCE,0xFE,0xEE,0xDE,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsl",		false,OP_ENTRY(   0,   0,   0,0x38,   0,0x78,0x68,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsla",	false,OP_ENTRY(0x48,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lslx",	false,OP_ENTRY(0x58,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsr",		false,OP_ENTRY(   0,   0,   0,0x34,   0,0x74,0x64,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsra",	false,OP_ENTRY(0x44,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsrx",	false,OP_ENTRY(0x54,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"mul",		false,OP_ENTRY(0x42,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"neg",		false,OP_ENTRY(   0,   0,   0,0x30,   0,0x70,0x60,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"nega",	false,OP_ENTRY(0x40,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"negx",	false,OP_ENTRY(0x50,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"nop",		false,OP_ENTRY(0x9D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ora",		false,OP_ENTRY(   0,0xAA,   0,0xBA,0xCA,0xFA,0xEA,0xDA,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rol",		false,OP_ENTRY(   0,   0,   0,0x39,   0,0x79,0x69,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rola",	false,OP_ENTRY(0x49,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rolx",	false,OP_ENTRY(0x59,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ror",		false,OP_ENTRY(   0,   0,   0,0x36,   0,0x76,0x66,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rora",	false,OP_ENTRY(0x46,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rorx",	false,OP_ENTRY(0x56,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rsp",		false,OP_ENTRY(0x9C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rti",		false,OP_ENTRY(0x80,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rts",		false,OP_ENTRY(0x81,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sbc",		false,OP_ENTRY(   0,0xA2,   0,0xB2,0xC2,0xF2,0xE2,0xD2,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sec",		false,OP_ENTRY(0x99,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sei",		false,OP_ENTRY(0x9B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sta",		false,OP_ENTRY(   0,   0,   0,0xB7,0xC7,0xF7,0xE7,0xD7,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"stop",	false,OP_ENTRY(0x8E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"stx",		false,OP_ENTRY(   0,   0,   0,0xBF,0xCF,0xFF,0xEF,0xDF,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sub",		false,OP_ENTRY(   0,0xA0,   0,0xB0,0xC0,0xF0,0xE0,0xD0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"swi",		false,OP_ENTRY(0x83,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tax",		false,OP_ENTRY(0x97,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tst",		false,OP_ENTRY(   0,   0,   0,0x3D,   0,0x7D,0x6D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tsta",	false,OP_ENTRY(0x4D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tstx",	false,OP_ENTRY(0x5D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"txa",		false,OP_ENTRY(0x9F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"wait",	false,OP_ENTRY(0x8F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
	};

static OPCODE
	Opcodes68hc08[]=
	{
//								   inh  im8  im16 dir  ext  hx   8hx  16hx 8sp  16sp bd   im8d dd   hx+d dhx+ hx+  8hx+
		{"adc",		false,OP_ENTRY(   0,0xA9,   0,0xB9,0xC9,0xF9,0xE9,0xD9,0xE9,0xD9,   0,   0,   0,   0,   0,   0,   0)},
		{"add",		false,OP_ENTRY(   0,0xAB,   0,0xBB,0xCB,0xFB,0xEB,0xDB,0xEB,0xDB,   0,   0,   0,   0,   0,   0,   0)},
		{"ais",		false,OP_ENTRY(   0,0xA7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"aix",		false,OP_ENTRY(   0,0xAF,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"and",		false,OP_ENTRY(   0,0xA4,   0,0xB4,0xC4,0xF4,0xE4,0xD4,0xE4,0xD4,   0,   0,   0,   0,   0,   0,   0)},
		{"asl",		false,OP_ENTRY(   0,   0,   0,0x38,   0,0x78,0x68,   0,0x68,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asla",	false,OP_ENTRY(0x48,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"aslx",	false,OP_ENTRY(0x58,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asr",		false,OP_ENTRY(   0,   0,   0,0x37,   0,0x77,0x67,   0,0x67,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asra",	false,OP_ENTRY(0x47,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"asrx",	false,OP_ENTRY(0x57,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bcc",		true ,OP_ENTRY(0x24,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr",	false,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x11,   0,   0,   0,   0,   0,   0)},
		{"bclr0",	false,OP_ENTRY(   0,   0,   0,0x11,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr1",	false,OP_ENTRY(   0,   0,   0,0x13,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr2",	false,OP_ENTRY(   0,   0,   0,0x15,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr3",	false,OP_ENTRY(   0,   0,   0,0x17,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr4",	false,OP_ENTRY(   0,   0,   0,0x19,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr5",	false,OP_ENTRY(   0,   0,   0,0x1B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr6",	false,OP_ENTRY(   0,   0,   0,0x1D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bclr7",	false,OP_ENTRY(   0,   0,   0,0x1F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bcs",		true ,OP_ENTRY(0x25,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"beq",		true ,OP_ENTRY(0x27,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bge",		true ,OP_ENTRY(0x90,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bgt",		true ,OP_ENTRY(0x92,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhcc",	true ,OP_ENTRY(0x28,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhcs",	true ,OP_ENTRY(0x29,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhi",		true ,OP_ENTRY(0x22,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bhs",		true ,OP_ENTRY(0x24,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bih",		true ,OP_ENTRY(0x2F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bil",		true ,OP_ENTRY(0x2E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bit",		false,OP_ENTRY(   0,0xA5,   0,0xB5,0xC5,0xF5,0xE5,0xD5,0xE5,0xD5,   0,   0,   0,   0,   0,   0,   0)},
		{"ble",		true ,OP_ENTRY(0x93,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"blo",		true ,OP_ENTRY(0x25,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bls",		true ,OP_ENTRY(0x23,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"blt",		true ,OP_ENTRY(0x91,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bmc",		true ,OP_ENTRY(0x2C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bmi",		true ,OP_ENTRY(0x2B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bms",		true ,OP_ENTRY(0x2D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bne",		true ,OP_ENTRY(0x26,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bpl",		true ,OP_ENTRY(0x2A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bra",		true ,OP_ENTRY(0x20,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr",	true ,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x01,   0,   0,   0,   0,   0,   0)},
		{"brclr0",	true ,OP_ENTRY(   0,   0,   0,0x01,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr1",	true ,OP_ENTRY(   0,   0,   0,0x03,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr2",	true ,OP_ENTRY(   0,   0,   0,0x05,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr3",	true ,OP_ENTRY(   0,   0,   0,0x07,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr4",	true ,OP_ENTRY(   0,   0,   0,0x09,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr5",	true ,OP_ENTRY(   0,   0,   0,0x0B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr6",	true ,OP_ENTRY(   0,   0,   0,0x0D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brclr7",	true ,OP_ENTRY(   0,   0,   0,0x0F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brn",		true ,OP_ENTRY(0x21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset",	true ,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x00,   0,   0,   0,   0,   0,   0)},
		{"brset0",	true ,OP_ENTRY(   0,   0,   0,0x00,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset1",	true ,OP_ENTRY(   0,   0,   0,0x02,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset2",	true ,OP_ENTRY(   0,   0,   0,0x04,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset3",	true ,OP_ENTRY(   0,   0,   0,0x06,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset4",	true ,OP_ENTRY(   0,   0,   0,0x08,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset5",	true ,OP_ENTRY(   0,   0,   0,0x0A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset6",	true ,OP_ENTRY(   0,   0,   0,0x0C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"brset7",	true ,OP_ENTRY(   0,   0,   0,0x0E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset",	false,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x10,   0,   0,   0,   0,   0,   0)},
		{"bset0",	false,OP_ENTRY(   0,   0,   0,0x10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset1",	false,OP_ENTRY(   0,   0,   0,0x12,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset2",	false,OP_ENTRY(   0,   0,   0,0x14,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset3",	false,OP_ENTRY(   0,   0,   0,0x16,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset4",	false,OP_ENTRY(   0,   0,   0,0x18,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset5",	false,OP_ENTRY(   0,   0,   0,0x1A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset6",	false,OP_ENTRY(   0,   0,   0,0x1C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bset7",	false,OP_ENTRY(   0,   0,   0,0x1E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"bsr",		true ,OP_ENTRY(0xAD,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cbeq",	true ,OP_ENTRY(   0,   0,   0,0x31,   0,   0,   0,   0,0x61,   0,   0,   0,   0,   0,   0,0x71,0x61)},
		{"cbeqa",	true ,OP_ENTRY(   0,0x41,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cbeqx",	true ,OP_ENTRY(   0,0x51,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clc",		false,OP_ENTRY(0x98,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cli",		false,OP_ENTRY(0x9A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clr",		false,OP_ENTRY(   0,   0,   0,0x3F,   0,0x7F,0x6F,   0,0x6F,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clra",	false,OP_ENTRY(0x4F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clrx",	false,OP_ENTRY(0x5F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"clrh",	false,OP_ENTRY(0x8C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cmp",		false,OP_ENTRY(   0,0xA1,   0,0xB1,0xC1,0xF1,0xE1,0xD1,0xE1,0xD1,   0,   0,   0,   0,   0,   0,   0)},
		{"com",		false,OP_ENTRY(   0,   0,   0,0x33,   0,0x73,0x63,   0,0x63,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"coma",	false,OP_ENTRY(0x43,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"comx",	false,OP_ENTRY(0x53,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cphx",	false,OP_ENTRY(   0,   0,0x65,0x75,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"cpx",		false,OP_ENTRY(   0,0xA3,   0,0xB3,0xC3,0xF3,0xE3,0xD3,0xE3,0xD3,   0,   0,   0,   0,   0,   0,   0)},
		{"daa",		false,OP_ENTRY(0x72,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dbnz",	true ,OP_ENTRY(   0,   0,   0,0x3B,   0,0x7B,0x6B,   0,0x6B,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dbnza",	true ,OP_ENTRY(0x4B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dbnzx",	true ,OP_ENTRY(0x5B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dec",		false,OP_ENTRY(   0,   0,   0,0x3A,   0,0x7A,0x6A,   0,0x6A,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"deca",	false,OP_ENTRY(0x4A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"decx",	false,OP_ENTRY(0x5A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"dex",		false,OP_ENTRY(0x5A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"div",		false,OP_ENTRY(0x52,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"eor",		false,OP_ENTRY(   0,0xA8,   0,0xB8,0xC8,0xF8,0xE8,0xD8,0xE8,0xD8,   0,   0,   0,   0,   0,   0,   0)},
		{"inc",		false,OP_ENTRY(   0,   0,   0,0x3C,   0,0x7C,0x6C,   0,0x6C,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"inca",	false,OP_ENTRY(0x4C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"incx",	false,OP_ENTRY(0x5C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"inx",		false,OP_ENTRY(0x5C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"jmp",		false,OP_ENTRY(   0,   0,   0,0xBC,0xCC,0xFC,0xEC,0xDC,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"jsr",		false,OP_ENTRY(   0,   0,   0,0xBD,0xCD,0xFD,0xED,0xDD,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lda",		false,OP_ENTRY(   0,0xA6,   0,0xB6,0xC6,0xF6,0xE6,0xD6,0xE6,0xD6,   0,   0,   0,   0,   0,   0,   0)},
		{"ldhx",	false,OP_ENTRY(   0,   0,0x45,0x55,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ldx",		false,OP_ENTRY(   0,0xAE,   0,0xBE,0xCE,0xFE,0xEE,0xDE,0xEE,0xDE,   0,   0,   0,   0,   0,   0,   0)},
		{"lsl",		false,OP_ENTRY(   0,   0,   0,0x38,   0,0x78,0x68,   0,0x68,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsla",	false,OP_ENTRY(0x48,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lslx",	false,OP_ENTRY(0x58,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsr",		false,OP_ENTRY(   0,   0,   0,0x34,   0,0x74,0x64,   0,0x64,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsra",	false,OP_ENTRY(0x44,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"lsrx",	false,OP_ENTRY(0x54,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"mov",		false,OP_ENTRY(   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0x6E,0x4E,0x7E,0x5E,   0,   0)},
		{"mul",		false,OP_ENTRY(0x42,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"neg",		false,OP_ENTRY(   0,   0,   0,0x30,   0,0x70,0x60,   0,0x60,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"nega",	false,OP_ENTRY(0x40,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"negx",	false,OP_ENTRY(0x50,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"nop",		false,OP_ENTRY(0x9D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"nsa",		false,OP_ENTRY(0x62,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ora",		false,OP_ENTRY(   0,0xAA,   0,0xBA,0xCA,0xFA,0xEA,0xDA,0xEA,0xDA,   0,   0,   0,   0,   0,   0,   0)},
		{"psha",	false,OP_ENTRY(0x87,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"pshh",	false,OP_ENTRY(0x8B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"pshx",	false,OP_ENTRY(0x89,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"pula",	false,OP_ENTRY(0x86,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"pulh",	false,OP_ENTRY(0x8A,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"pulx",	false,OP_ENTRY(0x88,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rol",		false,OP_ENTRY(   0,   0,   0,0x39,   0,0x79,0x69,   0,0x69,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rola",	false,OP_ENTRY(0x49,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rolx",	false,OP_ENTRY(0x59,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"ror",		false,OP_ENTRY(   0,   0,   0,0x36,   0,0x76,0x66,   0,0x66,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rora",	false,OP_ENTRY(0x46,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rorx",	false,OP_ENTRY(0x56,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rsp",		false,OP_ENTRY(0x9C,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rti",		false,OP_ENTRY(0x80,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"rts",		false,OP_ENTRY(0x81,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sbc",		false,OP_ENTRY(   0,0xA2,   0,0xB2,0xC2,0xF2,0xE2,0xD2,0xE2,0xD2,   0,   0,   0,   0,   0,   0,   0)},
		{"sec",		false,OP_ENTRY(0x99,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sei",		false,OP_ENTRY(0x9B,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"sta",		false,OP_ENTRY(   0,   0,   0,0xB7,0xC7,0xF7,0xE7,0xD7,0xE7,0xD7,   0,   0,   0,   0,   0,   0,   0)},
		{"sthx",	false,OP_ENTRY(   0,   0,   0,0x35,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"stop",	false,OP_ENTRY(0x8E,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"stx",		false,OP_ENTRY(   0,   0,   0,0xBF,0xCF,0xFF,0xEF,0xDF,0xEF,0xDF,   0,   0,   0,   0,   0,   0,   0)},
		{"sub",		false,OP_ENTRY(   0,0xA0,   0,0xB0,0xC0,0xF0,0xE0,0xD0,0xE0,0xD0,   0,   0,   0,   0,   0,   0,   0)},
		{"swi",		false,OP_ENTRY(0x83,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tap",		false,OP_ENTRY(0x84,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tax",		false,OP_ENTRY(0x97,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tpa",		false,OP_ENTRY(0x85,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tst",		false,OP_ENTRY(   0,   0,   0,0x3D,   0,0x7D,0x6D,   0,0x6D,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tsta",	false,OP_ENTRY(0x4D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tstx",	false,OP_ENTRY(0x5D,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"tsx",		false,OP_ENTRY(0x95,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"txa",		false,OP_ENTRY(0x9F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"txs",		false,OP_ENTRY(0x94,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
		{"wait",	false,OP_ENTRY(0x8F,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0)},
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

static bool ParseIndex(const char *line,unsigned int *lineIndex)
// see if next thing on the line is the index register
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

static bool ParseSp(const char *line,unsigned int *lineIndex)
// see if next thing on the line is the stack register
// return true if so, false otherwise
{
	if(line[*lineIndex]=='S'||line[*lineIndex]=='s')
	{
		if(line[(*lineIndex)+1]=='P'||line[(*lineIndex)+1]=='p')
		{
			if(!IsLabelChar(line[(*lineIndex)+2]))
			{
				(*lineIndex)+=2;				// step over the register
				return(true);
			}
		}
	}
	return(false);
}

static bool ParseOperand(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Try to parse an operand and determine its type
// return true if the parsing succeeds
{
	if(ParseImmediatePreamble(line,lineIndex))		// an immediate operand?
	{
		if(ParseExpression(line,lineIndex,&operand->value,&operand->unresolved))
		{
			operand->type=POT_IMMEDIATE;
			return(true);
		}
	}
	else if(ParseIndex(line,lineIndex))
	{
		if(line[*lineIndex]=='+')	// post increment being specified?
		{
			(*lineIndex)++;
			operand->type=POT_INDEX_PI;
		}
		else
		{
			operand->type=POT_INDEX;
		}
		return(true);
	}
	else if(ParseSp(line,lineIndex))
	{
		operand->type=POT_SP;
		return(true);
	}
	else if(ParseExpression(line,lineIndex,&operand->value,&operand->unresolved))
	{
		operand->type=POT_VALUE;
		return(true);
	}
	return(false);
}

static bool GenerateImmediate8(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate an 8 bit immediate on the output byte stream
// if there is a hard failure, return false
{
	CheckByteRange(value,true,true);
	return(GenerateByte(value,listingRecord));
}

static bool GenerateImmediate16(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate a 16 bit immediate on the output byte stream
// if there is a hard failure, return false
{
	CheckWordRange(value,true,true);
	if(GenerateByte(value>>8,listingRecord))
	{
		return(GenerateByte(value&0xFF,listingRecord));
	}
	return(false);
}

static bool GenerateDirectAddress(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate a direct address on the output byte stream
// if there is a hard failure, return false
{
	CheckUnsignedByteRange(value,true,true);
	return(GenerateByte(value,listingRecord));
}

static bool GenerateExtendedAddress(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate an extended address on the output byte stream
// if there is a hard failure, return false
{
	CheckUnsignedWordRange(value,true,true);
	if(GenerateByte(value>>8,listingRecord))
	{
		return(GenerateByte(value&0xFF,listingRecord));
	}
	return(false);
}

static bool GenerateRelativeOffset(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate an relative offset on the output byte stream
// if there is a hard failure, return false
{
	unsigned int
		offset;

	offset=0;
	if(!unresolved&&currentSegment)
	{
		offset=value-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
		Check8RelativeRange(offset,true,true);
	}
	return(GenerateByte(offset,listingRecord));
}

static bool GenerateOffset8(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate a 8-bit offset on the output byte stream
// if there is a hard failure, return false
{
	CheckUnsignedByteRange(value,true,true);
	return(GenerateByte(value,listingRecord));
}

static bool GenerateOffset16(int value,bool unresolved,LISTING_RECORD *listingRecord)
// Generate a 16-bit offset on the output byte stream
// if there is a hard failure, return false
{
	CheckWordRange(value,true,true);		// can be signed too, since address space is 16 bits
	if(GenerateByte(value>>8,listingRecord))
	{
		return(GenerateByte(value&0xFF,listingRecord));
	}
	return(false);
}

// ----------------------------------------------------------------------------------------

static bool HandleInherent(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle an inherent opcode. Return false on hard error
{
	bool
		fail;

	fail=!GenerateByte(opcode->baseOpcode[OT_INHERENT],listingRecord);
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[0].value,operands[0].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleIndexed(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle X. Return false on hard error
{
	bool
		fail;

	fail=!GenerateByte(opcode->baseOpcode[OT_INDEXED],listingRecord);
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[1].value,operands[1].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleIndexedPi(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle X+. Return false on hard error
{
	bool
		fail;

	fail=!GenerateByte(opcode->baseOpcode[OT_INDEXED_PI],listingRecord);
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[1].value,operands[1].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleImmediate(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle immediate (8 or 16 bit) values. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_IMM8)
	{
		if(GenerateByte(opcode->baseOpcode[OT_IMM8],listingRecord))
		{
			fail=!GenerateImmediate8(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else	// must be 16 or we would not be here
	{
		if(GenerateByte(opcode->baseOpcode[OT_IMM16],listingRecord))
		{
			fail=!GenerateImmediate16(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[1].value,operands[1].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleDirectExtended(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle direct/extended address. Return false on hard error
{
	bool
		fail;

	fail=false;
	if((opcode->typeMask&M_DIRECT)&&(((operands[0].value>=0)&&(operands[0].value<256))||!(opcode->typeMask&M_EXTENDED)))
	{
		if(GenerateByte(opcode->baseOpcode[OT_DIRECT],listingRecord))
		{
			fail=!GenerateDirectAddress(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		if(GenerateByte(opcode->baseOpcode[OT_EXTENDED],listingRecord))
		{
			fail=!GenerateExtendedAddress(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[1].value,operands[1].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleIndexedPiDirect(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle X+,direct. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_INDEXED_PI_DIRECT],listingRecord))
	{
		fail=!GenerateDirectAddress(operands[1].value,operands[1].unresolved,listingRecord);
	}
	else
	{
		fail=true;
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleImmediate8Direct(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle #immediate,direct. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_IMM8_DIRECT],listingRecord))
	{
		if(GenerateImmediate8(operands[0].value,operands[0].unresolved,listingRecord))
		{
			fail=!GenerateDirectAddress(operands[1].value,operands[1].unresolved,listingRecord);
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
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleOffsetIndexed(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle offset,X. Return false on hard error
{
	bool
		fail;

	fail=false;
	if((opcode->typeMask&M_OFF8_INDEXED)&&(((operands[0].value>=0)&&(operands[0].value<256))||!(opcode->typeMask&M_OFF16_INDEXED)))
	{
		if(GenerateByte(opcode->baseOpcode[OT_OFF8_INDEXED],listingRecord))
		{
			fail=!GenerateOffset8(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		if(GenerateByte(opcode->baseOpcode[OT_OFF16_INDEXED],listingRecord))
		{
			fail=!GenerateOffset16(operands[0].value,operands[0].unresolved,listingRecord);
		}
		else
		{
			fail=true;
		}
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleDirectIndexedPi(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle direct,X+. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_DIRECT_INDEXED_PI],listingRecord))
	{
		fail=!GenerateDirectAddress(operands[0].value,operands[0].unresolved,listingRecord);
	}
	else
	{
		fail=true;
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleOffsetIndexedPi(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle offset,X+. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_OFF8_INDEXED_PI],listingRecord))
	{
		fail=!GenerateOffset8(operands[0].value,operands[0].unresolved,listingRecord);
	}
	else
	{
		fail=true;
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleOffsetSp(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle offset,SP. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(SP_PREAMBLE,listingRecord))	// output preamble that tells the '08 this is a SP referencing instruction
	{
		if((opcode->typeMask&M_OFF8_SP)&&(((operands[0].value>=0)&&(operands[0].value<256))||!(opcode->typeMask&M_OFF16_SP)))
		{
			if(GenerateByte(opcode->baseOpcode[OT_OFF8_SP],listingRecord))
			{
				fail=!GenerateOffset8(operands[0].value,operands[0].unresolved,listingRecord);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			if(GenerateByte(opcode->baseOpcode[OT_OFF16_SP],listingRecord))
			{
				fail=!GenerateOffset16(operands[0].value,operands[0].unresolved,listingRecord);
			}
			else
			{
				fail=true;
			}
		}
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleBitDirect(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle bit,direct. Return false on hard error
{
	bool
		fail;

	fail=false;
	Check8BitIndexRange(operands[0].value,true,true);
	operands[0].value&=0x07;
	if(GenerateByte(opcode->baseOpcode[OT_BIT_DIRECT]|(operands[0].value<<1),listingRecord))
	{
		fail=!GenerateDirectAddress(operands[1].value,operands[1].unresolved,listingRecord);
	}
	else
	{
		fail=true;
	}
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool HandleDirectDirect(OPCODE *opcode,OPERAND *operands,LISTING_RECORD *listingRecord)
// Handle direct,direct. Return false on hard error
{
	bool
		fail;

	fail=false;
	if(GenerateByte(opcode->baseOpcode[OT_DIRECT_DIRECT],listingRecord))
	{
		if(GenerateDirectAddress(operands[0].value,operands[0].unresolved,listingRecord))
		{
			fail=!GenerateDirectAddress(operands[1].value,operands[1].unresolved,listingRecord);
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
	if(!fail&&opcode->relativeMode)
	{
		fail=!GenerateRelativeOffset(operands[2].value,operands[2].unresolved,listingRecord);
	}
	return(!fail);
}

static bool ParseOperands(const char *line,unsigned int *lineIndex,unsigned int *numOperands,OPERAND *operands)
// parse from 0 to 3 operands from the line
// return the operands parsed
// If something does not look right, return false
{
	bool
		fail;
	bool
		initialComma;						// only allowed if first parameter is index reg

	fail=false;
	*numOperands=0;
	if(!ParseComment(line,lineIndex))	// make sure not at end of line
	{
		initialComma=ParseCommaSeparator(line,lineIndex);
		if(!ParseComment(line,lineIndex))	// make sure not at end of line
		{
			if(ParseOperand(line,lineIndex,&operands[0]))
			{
				if(!initialComma||(operands[0].type==POT_INDEX)||(operands[0].type==POT_INDEX_PI))
				{
					*numOperands=1;
					if(!ParseComment(line,lineIndex))	// make sure not at end of line
					{
						if(ParseCommaSeparator(line,lineIndex))
						{
							if(!ParseComment(line,lineIndex))	// make sure not at end of line
							{
								if(ParseOperand(line,lineIndex,&operands[1]))
								{
									*numOperands=2;
									if(!ParseComment(line,lineIndex))	// make sure not at end of line
									{
										if(ParseCommaSeparator(line,lineIndex))
										{
											if(!ParseComment(line,lineIndex))	// make sure not at end of line
											{
												if(ParseOperand(line,lineIndex,&operands[2]))
												{
													*numOperands=3;
													if(!ParseComment(line,lineIndex))	// make sure were at end of line
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
		}
		else
		{
			fail=true;
		}
	}
	return(!fail);
}

static OPCODE *MatchOpcode(const char *string)
// match opcodes for this processor, return NULL if none matched
{
	return((OPCODE *)STFindDataForNameNoCase(*((SYM_TABLE **)(currentProcessor->processorData)),string));
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
		numOperands;
	OPERAND
		operands[3];

	result=true;					// no hard failure yet
	*success=false;					// no match yet
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=MatchOpcode(string)))						// found opcode?
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;
			if(ParseOperands(line,lineIndex,&numOperands,&operands[0]))	// fetch operands for opcode
			{
				if(!opcode->relativeMode||(numOperands&&(operands[numOperands-1].type==POT_VALUE)))	// if relative, make sure last operand is a value
				{
					switch(opcode->relativeMode?(numOperands-1):numOperands)
					{
						case 0:
							if(opcode->typeMask&M_INHERENT)
							{
								result=HandleInherent(opcode,operands,listingRecord);
							}
							else
							{
								ReportBadOperands();
							}
							break;
						case 1:
							switch(operands[0].type)
							{
								case POT_INDEX:
									if(opcode->typeMask&M_INDEXED)
									{
										result=HandleIndexed(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								case POT_INDEX_PI:
									if(opcode->typeMask&M_INDEXED_PI)
									{
										result=HandleIndexedPi(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								case POT_IMMEDIATE:
									if(opcode->typeMask&(M_IMM8|M_IMM16))
									{
										result=HandleImmediate(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								case POT_VALUE:
									if(opcode->typeMask&(M_DIRECT|M_EXTENDED))
									{
										result=HandleDirectExtended(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								default:
									ReportBadOperands();
									break;
							}
							break;
						case 2:
							switch(operands[0].type)
							{
								case POT_INDEX_PI:
									if((opcode->typeMask&M_INDEXED_PI_DIRECT)&&(operands[1].type==POT_VALUE))
									{
										result=HandleIndexedPiDirect(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								case POT_IMMEDIATE:
									if((opcode->typeMask&M_IMM8_DIRECT)&&(operands[1].type==POT_VALUE))
									{
										result=HandleImmediate8Direct(opcode,operands,listingRecord);
									}
									else
									{
										ReportBadOperands();
									}
									break;
								case POT_VALUE:
									switch(operands[1].type)
									{
										case POT_INDEX:
											if(opcode->typeMask&(M_OFF8_INDEXED|M_OFF16_INDEXED))
											{
												result=HandleOffsetIndexed(opcode,operands,listingRecord);
											}
											else
											{
												ReportBadOperands();
											}
											break;
										case POT_INDEX_PI:
											if(opcode->typeMask&M_DIRECT_INDEXED_PI)
											{
												result=HandleDirectIndexedPi(opcode,operands,listingRecord);
											}
											else if(opcode->typeMask&M_OFF8_INDEXED_PI)
											{
												result=HandleOffsetIndexedPi(opcode,operands,listingRecord);
											}
											else
											{
												ReportBadOperands();
											}
											break;
										case POT_SP:
											if(opcode->typeMask&(M_OFF8_SP|M_OFF16_SP))
											{
												result=HandleOffsetSp(opcode,operands,listingRecord);
											}
											else
											{
												ReportBadOperands();
											}
											break;
										case POT_VALUE:
											if(opcode->typeMask&M_BIT_DIRECT)
											{
												result=HandleBitDirect(opcode,operands,listingRecord);
											}
											else if(opcode->typeMask&M_DIRECT_DIRECT)
											{
												result=HandleDirectDirect(opcode,operands,listingRecord);
											}
											else
											{
												ReportBadOperands();
											}
											break;
										default:
											ReportBadOperands();
											break;
									}
									break;
								default:
									ReportBadOperands();
									break;
							}
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
	STDisposeSymbolTable(opcodeSymbols68hc08);
	STDisposeSymbolTable(opcodeSymbols6805);
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
			if((opcodeSymbols6805=STNewSymbolTable(elementsof(Opcodes6805))))
			{
				for(i=0;!fail&&(i<elementsof(Opcodes6805));i++)
				{
					if(!STAddEntryAtEnd(opcodeSymbols6805,Opcodes6805[i].name,&Opcodes6805[i]))
					{
						fail=true;
					}
				}
				if(!fail)
				{
					if((opcodeSymbols68hc08=STNewSymbolTable(elementsof(Opcodes68hc08))))
					{
						for(i=0;!fail&&(i<elementsof(Opcodes68hc08));i++)
						{
							if(!STAddEntryAtEnd(opcodeSymbols68hc08,Opcodes68hc08[i].name,&Opcodes68hc08[i]))
							{
								fail=true;
							}
						}
						if(!fail)
						{
							return(true);
						}
						STDisposeSymbolTable(opcodeSymbols68hc08);
					}
				}
				STDisposeSymbolTable(opcodeSymbols6805);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Motorola 6805",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"6805",&opcodeSymbols6805),
		PROCESSOR(&processorFamily,"68hc05",&opcodeSymbols6805),
		PROCESSOR(&processorFamily,"68705",&opcodeSymbols6805),
		PROCESSOR(&processorFamily,"68hc08",&opcodeSymbols68hc08),
	};
