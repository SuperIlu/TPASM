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


// Generate code for Atmel AVR devices

// ajp - 8 June 2000:
//			separated out processor-specific instructions
//			added incbin
//
// ajp - 10 June 2000:
//			reversed byte order for incbin and dc.b/db
//			corrected lpm instruction so extra modes are only supported on the mega161
//
// ajp - 11 June 2000:
//			improved method of handling extra modes for ld and st on different parts
//

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols,					// symbols for the opcodes common to all
	*opcodeFullSymbols,				// symbols for all remaining opcodes
	*opcode23xxSymbols,				// symbols for the extra opcodes in the 23xx/44xx/85xx parts
	*opcodeMega103Symbols,			// symbols for the extra opcodes in the mega103 part
	*opcodeMega8Mega161Symbols,		// symbols for the extra opcodes in the mega8 and mega161 parts
	*opcodeTinySymbols,				// symbols for the extra opcodes in the tiny parts except the ATtiny22
	*opcodeTiny22Symbols;			// symbols for the extra opcodes in the ATtiny22


static PROCESSOR
	*currentProcessor;


struct OPERAND
{
	unsigned int
		type;		// type of operand located
	int
		value,		// first immediate value, non immediate value, or register offset
		bit;		// bit number if present
	bool
		unresolved,		// if value was an expression, this tells if it was unresolved
		bitUnresolved;	// if bit number was an expression, this tells if it was unresolved
};

// enumerated operand types (used when parsing)

enum
{
	OT_VALUE,				// xxxx
	OT_REGISTER,			// Rn
	OT_REG_X,				// X
	OT_REG_XINC,			// X+
	OT_REG_DECX,			// -X
	OT_REG_X_OFF,			// X+xx	<<parsed but never used by AVR
	OT_REG_Y,				// Y
	OT_REG_YINC,			// Y+
	OT_REG_DECY,			// -Y
	OT_REG_Y_OFF,			// Y+xx
	OT_REG_Z,				// Z
	OT_REG_ZINC,			// Z+
	OT_REG_DECZ,			// -Z
	OT_REG_Z_OFF,			// Z+xx
};


// enumerated addressing modes

enum
{
	AM_IMPLIED,						// no operands

	AM_REG3_REG3,					// Rd,Rr (d=16-23, r=16-23)

	AM_REG4,						// Rd (d=16-31)
	AM_REG4_REG4,					// Rd,Rr (d=16-31  r=16-31)
	AM_REG4_IMMEDIATE8,				// Rd,#xx (d=16-31)
	AM_REG4_NOTIMMEDIATE8,			// Rd,#xx (d=16-31) (xx is complemented)

	AM_REG5,						// Rd
	AM_REG5_REG5,					// Rd,Rr
	AM_REG5_DUP,					// Rd (register is duplicated in src and dst)
	AM_REG5_BIT,					// Rd,b
	AM_REG5_ADDR6,					// Rd,xx
	AM_REG5_ADDR16,					// Rd,xx 5 bits of register, 16 bits of address

	AM_REGPAIR2_IMMEDIATE6,			// Rd,#xx (d=24,26,28,30)
	AM_REGPAIR4_REGPAIR4,			// Rd,Rr (d=0,2,...,30  r=0,2,...,30) register pair, register pair

	AM_SREG,						// s bit position in s (0-7)
	AM_SREG_RELATIVE7,				// s,xx s bit and 7 bit relative address

	AM_RELATIVE7,					// xx 7 bit relative address
	AM_RELATIVE12,					// xx 12 bit relative address

	AM_ADDR5_BIT,					// xx,b 5 bits of address, and a bit index
	AM_ADDR6_REG5,					// xx,Rd
	AM_ADDR16_REG5,					// xx,Rd 16 bits of address, 5 bits of register
	AM_ADDR22,						// xx 22 bits of address

	AM_REG5_OFFSETX,				// Rd,X
	AM_REG5_OFFSETXINC,				// Rd,X+
	AM_REG5_OFFSETDECX,				// Rd,-X
	AM_REG5_OFFSETY,				// Rd,Y
	AM_REG5_OFFSETYINC,				// Rd,Y+
	AM_REG5_OFFSETDECY,				// Rd,-Y
	AM_REG5_OFFSETY6,				// Rd,Y+xx
	AM_REG5_OFFSETZ,				// Rd,Z
	AM_REG5_OFFSETZINC,				// Rd,Z+
	AM_REG5_OFFSETDECZ,				// Rd,-Z
	AM_REG5_OFFSETZ6,				// Rd,Z+xx

	AM_OFFSETX_REG5,				// X,Rd
	AM_OFFSETXINC_REG5,				// X+,Rd
	AM_OFFSETDECX_REG5,				// -X,Rd
	AM_OFFSETY_REG5,				// Y,Rd
	AM_OFFSETYINC_REG5,				// Y+,Rd
	AM_OFFSETDECY_REG5,				// -Y,Rd
	AM_OFFSETY6_REG5,				// Y+xx,Rd
	AM_OFFSETZ_REG5,				// Z,Rd
	AM_OFFSETZINC_REG5,				// Z+,Rd
	AM_OFFSETDECZ_REG5,				// -Z,Rd
	AM_OFFSETZ6_REG5,				// Z+xx,Rd
};

struct ADDRESSING_MODE
{
	unsigned int
		mode;
	unsigned short
		baseOpcode;
};

struct OPCODE
{
	const char
		*name;
	unsigned int
		numModes;
	ADDRESSING_MODE
		*addressingModes;
};

#define	MODES(modeArray)	sizeof(modeArray)/sizeof(ADDRESSING_MODE),&modeArray[0]

static ADDRESSING_MODE
	M_ADC[]=
	{
		{AM_REG5_REG5,					0x1C00},
	},
	M_ADD[]=
	{
		{AM_REG5_REG5,					0x0C00},
	},
	M_ADIW[]=
	{
		{AM_REGPAIR2_IMMEDIATE6,		0x9600},
	},
	M_AND[]=
	{
		{AM_REG5_REG5,					0x2000},
	},
	M_ANDI[]=
	{
		{AM_REG4_IMMEDIATE8,			0x7000},
	},
	M_ASR[]=
	{
		{AM_REG5,						0x9405},
	},
	M_BCLR[]=
	{
		{AM_SREG,						0x9488},
	},
	M_BLD[]=
	{
		{AM_REG5_BIT,					0xF800},
	},
	M_BRBC[]=
	{
		{AM_SREG_RELATIVE7,				0xF400},
	},
	M_BRBS[]=
	{
		{AM_SREG_RELATIVE7,				0xF000},
	},
	M_BRCC[]=
	{
		{AM_RELATIVE7,					0xF400},
	},
	M_BRCS[]=
	{
		{AM_RELATIVE7,					0xF000},
	},
	M_BREQ[]=
	{
		{AM_RELATIVE7,					0xF001},
	},
	M_BRGE[]=
	{
		{AM_RELATIVE7,					0xF404},
	},
	M_BRHC[]=
	{
		{AM_RELATIVE7,					0xF405},
	},
	M_BRHS[]=
	{
		{AM_RELATIVE7,					0xF005},
	},
	M_BRID[]=
	{
		{AM_RELATIVE7,					0xF407},
	},
	M_BRIE[]=
	{
		{AM_RELATIVE7,					0xF007},
	},
	M_BRLO[]=
	{
		{AM_RELATIVE7,					0xF000},
	},
	M_BRLT[]=
	{
		{AM_RELATIVE7,					0xF004},
	},
	M_BRMI[]=
	{
		{AM_RELATIVE7,					0xF002},
	},
	M_BRNE[]=
	{
		{AM_RELATIVE7,					0xF401},
	},
	M_BRPL[]=
	{
		{AM_RELATIVE7,					0xF402},
	},
	M_BRSH[]=
	{
		{AM_RELATIVE7,					0xF400},
	},
	M_BRTC[]=
	{
		{AM_RELATIVE7,					0xF406},
	},
	M_BRTS[]=
	{
		{AM_RELATIVE7,					0xF006},
	},
	M_BRVC[]=
	{
		{AM_RELATIVE7,					0xF403},
	},
	M_BRVS[]=
	{
		{AM_RELATIVE7,					0xF003},
	},
	M_BSET[]=
	{
		{AM_SREG,						0x9408},
	},
	M_BST[]=
	{
		{AM_REG5_BIT,					0xFA00},
	},
	M_CALL[]=
	{
		{AM_ADDR22,						0x940E},
	},
	M_CBI[]=
	{
		{AM_ADDR5_BIT,					0x9800},
	},
	M_CBR[]=
	{
		{AM_REG4_NOTIMMEDIATE8,			0x7000},
	},
	M_CLC[]=
	{
		{AM_IMPLIED,					0x9488},
	},
	M_CLH[]=
	{
		{AM_IMPLIED,					0x94D8},
	},
	M_CLI[]=
	{
		{AM_IMPLIED,					0x94F8},
	},
	M_CLN[]=
	{
		{AM_IMPLIED,					0x94A8},
	},
	M_CLR[]=
	{
		{AM_REG5_DUP,					0x2400},
	},
	M_CLS[]=
	{
		{AM_IMPLIED,					0x94C8},
	},
	M_CLT[]=
	{
		{AM_IMPLIED,					0x94E8},
	},
	M_CLV[]=
	{
		{AM_IMPLIED,					0x94B8},
	},
	M_CLZ[]=
	{
		{AM_IMPLIED,					0x9498},
	},
	M_COM[]=
	{
		{AM_REG5,						0x9400},
	},
	M_CP[]=
	{
		{AM_REG5_REG5,					0x1400},
	},
	M_CPC[]=
	{
		{AM_REG5_REG5,					0x0400},
	},
	M_CPI[]=
	{
		{AM_REG4_IMMEDIATE8,			0x3000},
	},
	M_CPSE[]=
	{
		{AM_REG5_REG5,					0x1000},
	},
	M_DEC[]=
	{
		{AM_REG5,						0x940A},
	},
	M_EICALL[]=
	{
		{AM_IMPLIED,					0x9519},
	},
	M_EIJMP[]=
	{
		{AM_IMPLIED,					0x9419},
	},
	M_ELPM[]=
	{
		{AM_IMPLIED,					0x95D8},
		{AM_REG5_OFFSETZ,				0x9006},
		{AM_REG5_OFFSETZINC,			0x9007},
	},
	M_EOR[]=
	{
		{AM_REG5_REG5,					0x2400},
	},
	M_ESPM[]=
	{
		{AM_IMPLIED,					0x95F8},
	},
	M_FMUL[]=
	{
		{AM_REG3_REG3,					0x0308},
	},
	M_FMULS[]=
	{
		{AM_REG3_REG3,					0x0380},
	},
	M_FMULSU[]=
	{
		{AM_REG3_REG3,					0x0388},
	},
	M_ICALL[]=
	{
		{AM_IMPLIED,					0x9509},
	},
	M_IJMP[]=
	{
		{AM_IMPLIED,					0x9409},
	},
	M_IN[]=
	{
		{AM_REG5_ADDR6,					0xB000},
	},
	M_INC[]=
	{
		{AM_REG5,						0x9403},
	},
	M_JMP[]=
	{
		{AM_ADDR22,						0x940C},
	},
	M_LD[]=
	{
		{AM_REG5_OFFSETZ,				0x8000},
	},
	M_LD_EX[]=
	{
		{AM_REG5_OFFSETX,				0x900C},
		{AM_REG5_OFFSETXINC,			0x900D},
		{AM_REG5_OFFSETDECX,			0x900E},
		{AM_REG5_OFFSETY,				0x8008},
		{AM_REG5_OFFSETYINC,			0x9009},
		{AM_REG5_OFFSETDECY,			0x900A},
		{AM_REG5_OFFSETZ,				0x8000},
		{AM_REG5_OFFSETZINC,			0x9001},
		{AM_REG5_OFFSETDECZ,			0x9002},
		// the 2 below just duplicate LDD
		{AM_REG5_OFFSETY6,				0x8008},
		{AM_REG5_OFFSETZ6,				0x8000},
	},
	M_LDD[]=
	{
		{AM_REG5_OFFSETY6,				0x8008},
		{AM_REG5_OFFSETZ6,				0x8000},
	},
	M_LDI[]=
	{
		{AM_REG4_IMMEDIATE8,			0xE000},
	},
	M_LDS[]=
	{
		{AM_REG5_ADDR16,				0x9000},
	},
	M_LPM[]=
	{
		{AM_IMPLIED,					0x95C8},
	},
	M_LPM_EX[]=
	{
		{AM_IMPLIED,					0x95C8},	// extra modes for lpm that few processors support
		{AM_REG5_OFFSETZ,				0x9004},
		{AM_REG5_OFFSETZINC,			0x9005},
	},
	M_LSL[]=
	{
		{AM_REG5_DUP,					0x0C00},
	},
	M_LSR[]=
	{
		{AM_REG5,						0x9406},
	},
	M_MOV[]=
	{
		{AM_REG5_REG5,					0x2C00},
	},
	M_MOVW[]=
	{
		{AM_REGPAIR4_REGPAIR4,			0x0100},
	},
	M_MUL[]=
	{
		{AM_REG5_REG5,					0x9C00},
	},
	M_MULS[]=
	{
		{AM_REG4_REG4,					0x0200},
	},
	M_MULSU[]=
	{
		{AM_REG4_REG4,					0x0300},
	},
	M_NEG[]=
	{
		{AM_REG5,						0x9401},
	},
	M_NOP[]=
	{
		{AM_IMPLIED,					0x0000},
	},
	M_OR[]=
	{
		{AM_REG5_REG5,					0x2800},
	},
	M_ORI[]=
	{
		{AM_REG4_IMMEDIATE8,			0x6000},
	},
	M_OUT[]=
	{
		{AM_ADDR6_REG5,					0xB800},
	},
	M_POP[]=
	{
		{AM_REG5,						0x900F},
	},
	M_PUSH[]=
	{
		{AM_REG5,						0x920F},
	},
	M_RCALL[]=
	{
		{AM_RELATIVE12,					0xD000},
	},
	M_RET[]=
	{
		{AM_IMPLIED,					0x9508},
	},
	M_RETI[]=
	{
		{AM_IMPLIED,					0x9518},
	},
	M_RJMP[]=
	{
		{AM_RELATIVE12,					0xC000},
	},
	M_ROL[]=
	{
		{AM_REG5_DUP,					0x1C00},
	},
	M_ROR[]=
	{
		{AM_REG5,						0x9407},
	},
	M_SBC[]=
	{
		{AM_REG5_REG5,					0x0800},
	},
	M_SBCI[]=
	{
		{AM_REG4_IMMEDIATE8,			0x4000},
	},
	M_SBI[]=
	{
		{AM_ADDR5_BIT,					0x9A00},
	},
	M_SBIC[]=
	{
		{AM_ADDR5_BIT,					0x9900},
	},
	M_SBIS[]=
	{
		{AM_ADDR5_BIT,					0x9B00},
	},
	M_SBIW[]=
	{
		{AM_REGPAIR2_IMMEDIATE6,		0x9700},
	},
	M_SBR[]=
	{
		{AM_REG4_IMMEDIATE8,			0x6000},
	},
	M_SBRC[]=
	{
		{AM_REG5_BIT,					0xFC00},
	},
	M_SBRS[]=
	{
		{AM_REG5_BIT,					0xFE00},
	},
	M_SEC[]=
	{
		{AM_IMPLIED,					0x9408},
	},
	M_SEH[]=
	{
		{AM_IMPLIED,					0x9458},
	},
	M_SEI[]=
	{
		{AM_IMPLIED,					0x9478},
	},
	M_SEN[]=
	{
		{AM_IMPLIED,					0x9428},
	},
	M_SER[]=
	{
		{AM_REG4,						0xEF0F},
	},
	M_SES[]=
	{
		{AM_IMPLIED,					0x9448},
	},
	M_SET[]=
	{
		{AM_IMPLIED,					0x9468},
	},
	M_SEV[]=
	{
		{AM_IMPLIED,					0x9438},
	},
	M_SEZ[]=
	{
		{AM_IMPLIED,					0x9418},
	},
	M_SLEEP[]=
	{
		{AM_IMPLIED,					0x9588},
	},
	M_SPM[]=
	{
		{AM_IMPLIED,					0x95E8},
	},
	M_ST[]=
	{
		{AM_OFFSETZ_REG5,				0x8200},
	},
	M_ST_EX[]=
	{
		{AM_OFFSETX_REG5,				0x920C},
		{AM_OFFSETXINC_REG5,			0x920D},
		{AM_OFFSETDECX_REG5,			0x920E},
		{AM_OFFSETY_REG5,				0x8208},
		{AM_OFFSETYINC_REG5,			0x9209},
		{AM_OFFSETDECY_REG5,			0x920A},
		{AM_OFFSETZ_REG5,				0x8200},
		{AM_OFFSETZINC_REG5,			0x9201},
		{AM_OFFSETDECZ_REG5,			0x9202},
		// the 2 below just duplicate STD
		{AM_OFFSETY6_REG5,				0x8208},
		{AM_OFFSETZ6_REG5,				0x8200},
	},
	M_STD[]=
	{
		{AM_OFFSETY6_REG5,				0x8208},
		{AM_OFFSETZ6_REG5,				0x8200},
	},
	M_STS[]=
	{
		{AM_ADDR16_REG5,				0x9200},
	},
	M_SUB[]=
	{
		{AM_REG5_REG5,					0x1800},
	},
	M_SUBI[]=
	{
		{AM_REG4_IMMEDIATE8,			0x5000},
	},
	M_SWAP[]=
	{
		{AM_REG5,						0x9402},
	},
	M_TST[]=
	{
		{AM_REG5_DUP,					0x2000},
	},
	M_WDR[]=
	{
		{AM_IMPLIED,					0x95A8},
	};

static bool HandleAVRDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleAVRDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"db",		HandleAVRDB},
		{"dc.b",	HandleAVRDB},
		{"dw",		HandleAVRDW},
		{"dc.w",	HandleAVRDW},
		{"ds",		HandleDS},
	};

static OPCODE
	Opcodes[]=
	{
		{"adc",		MODES(M_ADC)	},
		{"add",		MODES(M_ADD)	},
		{"and",		MODES(M_AND)	},
		{"andi",	MODES(M_ANDI)	},
		{"asr",		MODES(M_ASR)	},
		{"bclr",	MODES(M_BCLR)	},
		{"bld",		MODES(M_BLD)	},
		{"brbc",	MODES(M_BRBC)	},
		{"brbs",	MODES(M_BRBS)	},
		{"brcc",	MODES(M_BRCC)	},
		{"brcs",	MODES(M_BRCS)	},
		{"breq",	MODES(M_BREQ)	},
		{"brge",	MODES(M_BRGE)	},
		{"brhc",	MODES(M_BRHC)	},
		{"brhs",	MODES(M_BRHS)	},
		{"brid",	MODES(M_BRID)	},
		{"brie",	MODES(M_BRIE)	},
		{"brlo",	MODES(M_BRLO)	},
		{"brlt",	MODES(M_BRLT)	},
		{"brmi",	MODES(M_BRMI)	},
		{"brne",	MODES(M_BRNE)	},
		{"brpl",	MODES(M_BRPL)	},
		{"brsh",	MODES(M_BRSH)	},
		{"brtc",	MODES(M_BRTC)	},
		{"brts",	MODES(M_BRTS)	},
		{"brvc",	MODES(M_BRVC)	},
		{"brvs",	MODES(M_BRVS)	},
		{"bset",	MODES(M_BSET)	},
		{"bst",		MODES(M_BST)	},
		{"cbi",		MODES(M_CBI)	},
		{"cbr",		MODES(M_CBR)	},
		{"clc",		MODES(M_CLC)	},
		{"clh",		MODES(M_CLH)	},
		{"cli",		MODES(M_CLI)	},
		{"cln",		MODES(M_CLN)	},
		{"clr",		MODES(M_CLR)	},
		{"cls",		MODES(M_CLS)	},
		{"clt",		MODES(M_CLT)	},
		{"clv",		MODES(M_CLV)	},
		{"clz",		MODES(M_CLZ)	},
		{"com",		MODES(M_COM)	},
		{"cp",		MODES(M_CP)		},
		{"cpc",		MODES(M_CPC)	},
		{"cpi",		MODES(M_CPI)	},
		{"cpse",	MODES(M_CPSE)	},
		{"dec",		MODES(M_DEC)	},
		{"eor",		MODES(M_EOR)	},
		{"in",		MODES(M_IN)		},
		{"inc",		MODES(M_INC)	},
		{"ldi",		MODES(M_LDI)	},
		{"lsl",		MODES(M_LSL)	},
		{"lsr",		MODES(M_LSR)	},
		{"mov",		MODES(M_MOV)	},
		{"neg",		MODES(M_NEG)	},
		{"nop",		MODES(M_NOP)	},
		{"or",		MODES(M_OR)		},
		{"ori",		MODES(M_ORI)	},
		{"out",		MODES(M_OUT)	},
		{"rcall",	MODES(M_RCALL)	},
		{"ret",		MODES(M_RET)	},
		{"reti",	MODES(M_RETI)	},
		{"rjmp",	MODES(M_RJMP)	},
		{"rol",		MODES(M_ROL)	},
		{"ror",		MODES(M_ROR)	},
		{"sbc",		MODES(M_SBC)	},
		{"sbci",	MODES(M_SBCI)	},
		{"sbi",		MODES(M_SBI)	},
		{"sbic",	MODES(M_SBIC)	},
		{"sbis",	MODES(M_SBIS)	},
		{"sbr",		MODES(M_SBR)	},
		{"sbrc",	MODES(M_SBRC)	},
		{"sbrs",	MODES(M_SBRS)	},
		{"sec",		MODES(M_SEC)	},
		{"seh",		MODES(M_SEH)	},
		{"sei",		MODES(M_SEI)	},
		{"sen",		MODES(M_SEN)	},
		{"ser",		MODES(M_SER)	},
		{"ses",		MODES(M_SES)	},
		{"set",		MODES(M_SET)	},
		{"sev",		MODES(M_SEV)	},
		{"sez",		MODES(M_SEZ)	},
		{"sleep",	MODES(M_SLEEP)	},
		{"sub",		MODES(M_SUB)	},
		{"subi",	MODES(M_SUBI)	},
		{"swap",	MODES(M_SWAP)	},
		{"tst",		MODES(M_TST)	},
		{"wdr",		MODES(M_WDR)	},
	};


// full set of opcodes not common to all

static OPCODE
	AdditionalFullOpcodes[]=
	{
		{"adiw",	MODES(M_ADIW)	},
		{"call",	MODES(M_CALL)	},
		{"eicall",	MODES(M_EICALL)	},
		{"eijmp",	MODES(M_EIJMP)	},
		{"elpm",	MODES(M_ELPM)	},
		{"espm",	MODES(M_ESPM)	},
		{"fmul",	MODES(M_FMUL)	},
		{"fmuls",	MODES(M_FMULS)	},
		{"fmulsu",	MODES(M_FMULSU)	},
		{"icall",	MODES(M_ICALL)	},
		{"ijmp",	MODES(M_IJMP)	},
		{"jmp",		MODES(M_JMP)	},
		{"ld",		MODES(M_LD_EX)	},
		{"ldd",		MODES(M_LDD)	},
		{"lds",		MODES(M_LDS)	},
		{"lpm",		MODES(M_LPM_EX)	},
		{"movw",	MODES(M_MOVW)	},
		{"mul",		MODES(M_MUL)	},
		{"muls",	MODES(M_MULS)	},
		{"mulsu",	MODES(M_MULSU)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"sbiw",	MODES(M_SBIW)	},
		{"spm",		MODES(M_SPM)	},
		{"st",		MODES(M_ST_EX)	},
		{"std",		MODES(M_STD)	},
		{"sts",		MODES(M_STS)	},
	};


// additional opcodes on AT90S23xx/AT90S44xx/AT90S85xx devices

static OPCODE
	Additional23xxOpcodes[]=
	{
		{"adiw",	MODES(M_ADIW)	},
		{"icall",	MODES(M_ICALL)	},
		{"ijmp",	MODES(M_IJMP)	},
		{"jmp",		MODES(M_JMP)	},
		{"ld",		MODES(M_LD_EX)	},
		{"ldd",		MODES(M_LDD)	},
		{"lds",		MODES(M_LDS)	},
		{"lpm",		MODES(M_LPM)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"sbiw",	MODES(M_SBIW)	},
		{"st",		MODES(M_ST_EX)	},
		{"std",		MODES(M_STD)	},
		{"sts",		MODES(M_STS)	},
	};


// additional opcodes on ATmega103 devices

static OPCODE
	AdditionalMega103Opcodes[]=
	{
		{"adiw",	MODES(M_ADIW)	},
		{"call",	MODES(M_CALL)	},
		{"elpm",	MODES(M_ELPM)	},
		{"icall",	MODES(M_ICALL)	},
		{"ijmp",	MODES(M_IJMP)	},
		{"jmp",		MODES(M_JMP)	},
		{"ld",		MODES(M_LD_EX)	},
		{"ldd",		MODES(M_LDD)	},
		{"lds",		MODES(M_LDS)	},
		{"lpm",		MODES(M_LPM)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"sbiw",	MODES(M_SBIW)	},
		{"st",		MODES(M_ST_EX)	},
		{"std",		MODES(M_STD)	},
		{"sts",		MODES(M_STS)	},
	};


// additional opcodes on ATmega8 and ATmega161 devices

static OPCODE
	AdditionalMega8Mega161Opcodes[]=
	{
		{"adiw",	MODES(M_ADIW)	},
		{"call",	MODES(M_CALL)	},
		{"fmul",	MODES(M_FMUL)	},
		{"fmuls",	MODES(M_FMULS)	},
		{"fmulsu",	MODES(M_FMULSU)	},
		{"icall",	MODES(M_ICALL)	},
		{"ijmp",	MODES(M_IJMP)	},
		{"jmp",		MODES(M_JMP)	},
		{"ld",		MODES(M_LD_EX)	},
		{"ldd",		MODES(M_LDD)	},
		{"lds",		MODES(M_LDS)	},
		{"lpm",		MODES(M_LPM_EX)	},
		{"movw",	MODES(M_MOVW)	},
		{"mul",		MODES(M_MUL)	},
		{"muls",	MODES(M_MULS)	},
		{"mulsu",	MODES(M_MULSU)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"sbiw",	MODES(M_SBIW)	},
		{"spm",		MODES(M_SPM)	},
		{"st",		MODES(M_ST_EX)	},
		{"std",		MODES(M_STD)	},
		{"sts",		MODES(M_STS)	},
	};


// additional opcodes on ATtinyxx devices except ATtiny22

static OPCODE
	AdditionalTinyOpcodes[]=
	{
		{"ld",		MODES(M_LD)		},
		{"st",		MODES(M_ST)		},
		{"lpm",		MODES(M_LPM)	},
	};


// additional opcodes on ATtiny22 devices

static OPCODE
	AdditionalTiny22Opcodes[]=
	{
		{"ld",		MODES(M_LD_EX)	},
		{"st",		MODES(M_ST_EX)	},
		{"lpm",		MODES(M_LPM)	},
	};


static bool GenerateAVRWord(int wordValue,LISTING_RECORD *listingRecord)
// output the wordValue to the current segment
// This will return false only if a "hard" error occurs
{
	unsigned char
		outputBytes[2];
	unsigned int
		length;
	bool
		fail;

	fail=false;
	if(currentSegment)
	{
		if(currentSegment->currentPC+currentSegment->codeGenOffset<(1<<22))
		{
			if(!intermediatePass)				// only do the real work if necessary
			{
				length=strlen(listingRecord->listObjectString);
				if(length+6<MAX_STRING)
				{
					sprintf(&listingRecord->listObjectString[length],"%04X ",wordValue);		// create list file output
				}
				outputBytes[0]=wordValue&0xFF;	// data is written as little endian words
				outputBytes[1]=wordValue>>8;

				fail=!AddBytesToSegment(currentSegment,currentSegment->currentPC*2,outputBytes,2);	// each memory location is 16 bits wide
			}
			currentSegment->currentPC++;
		}
		else
		{
			AssemblyComplaint(NULL,true,"Attempt to write code outside address space\n");
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Code cannot occur outside of a segment\n");
	}
	return(!fail);
}

static unsigned char
	heldByte;
static unsigned int
	byteGenCounter;

static void StartByteGeneration()
// some opcodes generate bytes at a time which need to be packed into words
// this makes that easier
{
	byteGenCounter=0;
}

static bool GenerateAVRByte(unsigned char byteValue,LISTING_RECORD *listingRecord)
// output the byteValue to the current segment
{
	bool
		fail;

	fail=false;
	if(byteGenCounter&1)
	{
		fail=!GenerateAVRWord((byteValue<<8)|heldByte,listingRecord);
	}
	else
	{
		heldByte=byteValue;					// just hold the byte until it is later flushed
	}
	byteGenCounter++;
	return(!fail);
}

static bool FlushByteGeneration(LISTING_RECORD *listingRecord)
// flush out any odd byte
{
	if(byteGenCounter&1)
	{
		return(GenerateAVRByte(0,listingRecord));
	}
	return(true);
}

static bool ParseIndexRegister(const char *line,unsigned int *lineIndex,unsigned int *index)
// Parse out the name of one of the index registers (X,Y, or Z)
// if one is located, return 0,1, or 2 to indicate which one
// If none is located, return false
// NOTE: this is brute force and inelegant.
{
	SkipWhiteSpace(line,lineIndex);					// move past any white space
	if(line[*lineIndex]=='X'||line[*lineIndex]=='x')
	{
		(*lineIndex)++;
		*index=0;
		return(true);
	}
	if(line[*lineIndex]=='Y'||line[*lineIndex]=='y')
	{
		(*lineIndex)++;
		*index=1;
		return(true);
	}
	if(line[*lineIndex]=='Z'||line[*lineIndex]=='z')
	{
		(*lineIndex)++;
		*index=2;
		return(true);
	}
	return(false);
}

static bool ParseRegisterOperand(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Try to parse out what looks like a register
// This will match:
// Rn  (where n is 0-31)
// X
// X+
// -X
// Y
// Y+
// -Y
// Y+xx
// Z
// Z+
// -Z
// Z+xx
// NOTE: if what was located does not look like a register return false
{
	unsigned int
		inputIndex;
	unsigned int
		tempIndex;
	unsigned int
		regIndex;
	bool
		fail;

	fail=false;
	SkipWhiteSpace(line,lineIndex);					// move past any white space
	inputIndex=*lineIndex;
	if(line[inputIndex]=='r'||line[inputIndex]=='R')	// register ?
	{
		operand->type=OT_REGISTER;
		inputIndex++;
		if(line[inputIndex]>='0'&&line[inputIndex]<='9')
		{
			operand->value=line[inputIndex]-'0';
			inputIndex++;
			if(line[inputIndex]>='0'&&line[inputIndex]<='9')
			{
				operand->value=operand->value*10+(line[inputIndex]-'0');
				inputIndex++;
				if(operand->value>=32)
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
	else	// not a Rn register, try indexed
	{
		if(line[inputIndex]=='-')	// predecrement?
		{
			inputIndex++;
			SkipWhiteSpace(line,&inputIndex);		// move past any white space
			if(ParseIndexRegister(line,&inputIndex,&regIndex))
			{
				switch(regIndex)
				{
					case 0:
						operand->type=OT_REG_DECX;
						break;
					case 1:
						operand->type=OT_REG_DECY;
						break;
					case 2:
						operand->type=OT_REG_DECZ;
						break;
					default:
						fail=true;		// should not happen
						break;
				}
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			if(ParseIndexRegister(line,&inputIndex,&regIndex))	// need to see an index register at this point
			{
				tempIndex=inputIndex;						// might need to back up to here if '+' not located
				SkipWhiteSpace(line,&tempIndex);			// move past any white space
				if(line[tempIndex]=='+')					// post increment or offset?
				{
					tempIndex++;
					inputIndex=tempIndex;					// ok so far, now see if there's something other than a separator or a comment
					if(!ParseComment(line,&tempIndex)&&!ParseCommaSeparator(line,&tempIndex))	// if not a comment, and not a separator, then assume expression used as an offset
					{
						if(ParseExpression(line,&tempIndex,&operand->value,&operand->unresolved))	// get the expression used for the offset
						{
							inputIndex=tempIndex;
							switch(regIndex)
							{
								case 0:
									operand->type=OT_REG_X_OFF;	// we parse this, but the processor cannot handle it
									break;
								case 1:
									operand->type=OT_REG_Y_OFF;
									break;
								case 2:
									operand->type=OT_REG_Z_OFF;
									break;
								default:
									fail=true;		// should not happen
									break;
							}
						}
						else
						{
							fail=true;						// failed to parse an expression, but SOMETHING followed the +, so complain that nothing was found
						}
					}
					else	// post increment
					{
						switch(regIndex)
						{
							case 0:
								operand->type=OT_REG_XINC;
								break;
							case 1:
								operand->type=OT_REG_YINC;
								break;
							case 2:
								operand->type=OT_REG_ZINC;
								break;
							default:
								fail=true;		// should not happen
								break;
						}
					}
				}
				else	// just an index register, no post increment or offset
				{
					switch(regIndex)
					{
						case 0:
							operand->type=OT_REG_X;
							break;
						case 1:
							operand->type=OT_REG_Y;
							break;
						case 2:
							operand->type=OT_REG_Z;
							break;
						default:
							fail=true;		// should not happen
							break;
					}
				}
			}
			else
			{
				fail=true;
			}
		}
	}

	if(!fail)			// something was located that looked like a register. Now make sure it does not run up against a label
	{
		if(!IsLabelChar(line[inputIndex]))	// make sure the character following this does not look like part of a label
		{
			*lineIndex=inputIndex;
			return(true);
		}
	}
	return(false);
}

static bool ParseOperand(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Parse an operand as the next thing on line
// return it in operand
// NOTE: if what was located does not look like an operand return false
{
	unsigned int
		inputIndex;
	int
		value;
	bool
		unresolved;

	if(ParseRegisterOperand(line,lineIndex,operand))
	{
		return(true);
	}
	else	// not register form, so try immediate number, or value
	{
		inputIndex=*lineIndex;
		if(ParseExpression(line,&inputIndex,&value,&unresolved))	// whatever it is, must be an expression
		{
			operand->type=OT_VALUE;
			operand->value=value;
			operand->unresolved=unresolved;
			*lineIndex=inputIndex;
			return(true);
		}
	}
	return(false);
}

// opcode handling

static bool CheckRegister3Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 3 bit register address
// complain if not
{
	if(value>=16&&value<24)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Register number (%d) out of range\n",value);
	}
	return(false);
}

static bool CheckRegister4Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 4 bit register address
// complain if not
{
	if(value>=16&&value<32)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Register number (%d) out of range\n",value);
	}
	return(false);
}

static bool CheckRegisterPair2Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 2 bit register pair address
// complain if not
{
	if(value==24||value==26||value==28||value==30)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Register number (%d) out of range\n",value);
	}
	return(false);
}

static bool CheckRegisterPair4Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 4 bit register pair address
// complain if not
{
	if((value&1)==0)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Register number (%d) out of range\n",value);
	}
	return(false);
}

static bool CheckImmediate6Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 6 bit immediate value
// complain if not
{
	if(value>=0&&value<64)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Immediate (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckRelative7Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 7 bit relative value
// complain if not
{
	if(value>=-64&&value<64)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Relative offset (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckRelative12Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 12 bit relative value
// complain if not
{
	if(value>=-2048&&value<2048)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Relative offset (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress5Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 5 bit address
// complain if not
{
	if(value>=0&&value<32)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Address (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress6Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 6 bit address
// complain if not
{
	if(value>=0&&value<64)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Address (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress16Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 16 bit address
// complain if not
{
	if(value>=0&&value<65536)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Address (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress22Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 16 bit address
// complain if not
{
	if(value>=0&&value<(1<<22))
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Address (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckOffset6Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 6 bit offset
// complain if not
{
	if(value>=0&&value<64)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Offset (0x%X) out of range\n",value);
	}
	return(false);
}


// pseudo op handling

static bool HandleAVRDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring bytes of data, or packed strings
{
	int
		value;
	bool
		unresolved;
	char
		outputString[MAX_STRING];
	unsigned int
		stringLength;
	bool
		done,
		fail;
	unsigned int
		i;

	done=false;
	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	StartByteGeneration();
	while(!done&&!fail)
	{
		if(ParseQuotedString(line,lineIndex,'"','"',outputString,&stringLength))
		{
			i=0;
			while(!fail&&i<stringLength)
			{
				fail=!GenerateAVRByte(outputString[i],listingRecord);
				i++;
			}
		}	
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckByteRange(value,true,true);
			fail=!GenerateAVRByte(value&0xFF,listingRecord);
		}
		else
		{
			ReportBadOperands();
			done=true;
		}
		if(!done&&!fail)				// look for separator, or comment
		{
			if(ParseCommaSeparator(line,lineIndex))
			{
			}
			else if(ParseComment(line,lineIndex))
			{
				done=true;
			}
			else
			{
				ReportBadOperands();
				done=true;
			}
		}
	}
	if(!fail)
	{
		fail=!FlushByteGeneration(listingRecord);
	}
	return(!fail);
}

static bool HandleAVRDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring words of data, or packed strings
{
	int
		value;
	bool
		unresolved;
	char
		outputString[MAX_STRING];
	unsigned int
		stringLength;
	bool
		done,
		fail;
	unsigned int
		i;

	done=false;
	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	while(!done&&!fail)
	{
		if(ParseQuotedString(line,lineIndex,'"','"',outputString,&stringLength))
		{
			i=0;
			while(!fail&&i<stringLength)
			{
				fail=!GenerateAVRByte(outputString[i],listingRecord);
				i++;
			}
			if(!fail)
			{
				fail=!FlushByteGeneration(listingRecord);
			}
		}	
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckWordRange(value,true,true);
			fail=!GenerateAVRWord(value,listingRecord);
		}
		else
		{
			ReportBadOperands();
			done=true;
		}
		if(!done&&!fail)				// look for separator, or comment
		{
			if(ParseCommaSeparator(line,lineIndex))
			{
			}
			else if(ParseComment(line,lineIndex))
			{
				done=true;
			}
			else
			{
				ReportBadOperands();
				done=true;
			}
		}
	}
	return(!fail);
}

static int ConvertRegister3(int value)
// Make sure value is in range for a 3 bit register if not, complain and
// force it into range
// return the bitfield value needed to represent it
{
	if(!CheckRegister3Range(value,true,true))
	{
		value=16;
	}
	return(value-16);
}

static int ConvertRegister4(int value)
// Make sure value is in range for a 4 bit register if not, complain and
// force it into range
// return the bitfield value needed to represent it
{
	if(!CheckRegister4Range(value,true,true))
	{
		value=16;
	}
	return(value-16);
}

static int ConvertRegister5(int value)
// Make sure value is in range for a 5 bit register if not, complain and
// force it into range
// return the bitfield value needed to represent it
{
	return(value);	// this is guaranteed to be in range
}

static int ConvertRegisterPair2(int value)
// Make sure value is in range for a 2 bit register pair if not, complain and
// force it into range
// return the bitfield value needed to represent it
{
	if(!CheckRegisterPair2Range(value,true,true))
	{
		value=24;
	}
	return((value-24)/2);
}

static int ConvertRegisterPair4(int value)
// Make sure value is in range for a 4 bit register pair if not, complain and
// force it into range
// return the bitfield value needed to represent it
{
	CheckRegisterPair4Range(value,true,true);
	value&=0xFE;
	return(value/2);
}

static int ConvertBit(int value)
// Make sure value is in range for a bit index
{
	Check8BitIndexRange(value,true,true);
	value&=0x07;
	return(value);
}

static int ConvertImmediate6(int value)
// Make sure value is in range for a 6 bit immediate, complain and
// force it into range
{
	CheckImmediate6Range(value,true,true);
	value&=0x3F;
	return(value);
}

static int ConvertImmediate8(int value)
// Make sure value is in range for a 8 bit immediate, complain and
// force it into range
{
	CheckByteRange(value,true,true);
	value&=0xFF;
	return(value);
}

static int ConvertRelative7(int value)
// Make sure value is in range for a 7 bit relative address, complain and
// force it into range
{
	if(currentSegment)
	{
		value=value-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
		CheckRelative7Range(value,true,true);
		value&=0x7F;
	}
	else
	{
		value=0;
	}
	return(value);
}

static int ConvertRelative12(int value)
// Make sure value is in range for a 12 bit relative address, complain and
// force it into range
{
	if(currentSegment)
	{
		value=value-(currentSegment->currentPC+currentSegment->codeGenOffset)-1;
		CheckRelative12Range(value,true,true);
		value&=0xFFF;
	}
	else
	{
		value=0;
	}
	return(value);
}

static int ConvertAddress5(int value)
// Make sure value is in range for a 5 bit address, complain and
// force it into range
{
	CheckAddress5Range(value,true,true);
	value&=0x1F;
	return(value);
}

static int ConvertAddress6(int value)
// Make sure value is in range for a 6 bit address, complain and
// force it into range
{
	CheckAddress6Range(value,true,true);
	value&=0x3F;
	return(value);
}

static int ConvertAddress16(int value)
// Make sure value is in range for a 16 bit address, complain and
// force it into range
{
	CheckAddress16Range(value,true,true);
	value&=0xFFFF;
	return(value);
}

static int ConvertAddress22(int value)
// Make sure value is in range for a 22 bit address, complain and
// force it into range
{
	CheckAddress22Range(value,true,true);
	value&=0x3FFFFF;
	return(value);
}

static int ConvertOffset6(int value)
// Make sure value is in range for a 6 bit offset, complain and
// force it into range
{
	CheckOffset6Range(value,true,true);
	value&=0x3F;
	return(value);
}

static bool HandleAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *operand1,OPERAND *operand2,LISTING_RECORD *listingRecord)
// Given an addressing mode record, and a set of operands, generate code (or an error message if something is
// out of range)
// This will only return false on a 'hard' error
{
	bool
		fail;
	int
		value1,
		value2;

	fail=false;
	switch(addressingMode->mode)
	{
		case AM_IMPLIED:
			fail=!GenerateAVRWord(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_REG3_REG3:
			value1=ConvertRegister3(operand1->value);
			value2=ConvertRegister3(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|value2,listingRecord);
			break;
		case AM_REG4:
			value1=ConvertRegister4(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG4_REG4:
			value1=ConvertRegister4(operand1->value);
			value2=ConvertRegister4(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|value2,listingRecord);
			break;
		case AM_REG4_IMMEDIATE8:
			value1=ConvertRegister4(operand1->value);
			value2=ConvertImmediate8(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0xF0)<<4)|(value2&0x0F),listingRecord);
			break;
		case AM_REG4_NOTIMMEDIATE8:
			value1=ConvertRegister4(operand1->value);
			value2=~ConvertImmediate8(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0xF0)<<4)|(value2&0x0F),listingRecord);
			break;
		case AM_REG5:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_REG5:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0x10)<<5)|(value2&0x0F),listingRecord);
			break;
		case AM_REG5_DUP:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value1&0x10)<<5)|(value1&0x0F),listingRecord);
			break;
		case AM_REG5_BIT:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertBit(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|value2,listingRecord);
			break;
		case AM_REG5_ADDR6:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertAddress6(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0x30)<<5)|(value2&0x0F),listingRecord);
			break;
		case AM_REG5_ADDR16:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertAddress16(operand2->value);
			if(GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord))
			{
				fail=!GenerateAVRWord(value2,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_REGPAIR2_IMMEDIATE6:
			value1=ConvertRegisterPair2(operand1->value);
			value2=ConvertImmediate6(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0x30)<<2)|(value2&0x0F),listingRecord);
			break;
		case AM_REGPAIR4_REGPAIR4:
			value1=ConvertRegisterPair4(operand1->value);
			value2=ConvertRegisterPair4(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|value2,listingRecord);
			break;
		case AM_SREG:
			value1=ConvertBit(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_SREG_RELATIVE7:
			value1=ConvertBit(operand1->value);
			value2=ConvertRelative7(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|value1|(value2<<3),listingRecord);
			break;
		case AM_RELATIVE7:
			value1=ConvertRelative7(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<3),listingRecord);
			break;
		case AM_RELATIVE12:
			value1=ConvertRelative12(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|value1,listingRecord);
			break;
		case AM_ADDR5_BIT:
			value1=ConvertAddress5(operand1->value);
			value2=ConvertBit(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<3)|value2,listingRecord);
			break;
		case AM_ADDR6_REG5:
			value1=ConvertAddress6(operand1->value);
			value2=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value2<<4)|((value1&0x30)<<5)|(value1&0x0F),listingRecord);
			break;
		case AM_ADDR16_REG5:
			value1=ConvertAddress16(operand1->value);
			value2=ConvertRegister5(operand2->value);
			if(GenerateAVRWord(addressingMode->baseOpcode|(value2<<4),listingRecord))
			{
				fail=!GenerateAVRWord(value1,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_ADDR22:
			value1=ConvertAddress22(operand1->value);
			if(GenerateAVRWord(addressingMode->baseOpcode|((value1>>16)&1)|(((value1>>17)&0x1F)<<4),listingRecord))
			{
				fail=!GenerateAVRWord(value1&0xFFFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_REG5_OFFSETX:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETXINC:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETDECX:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETY:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETYINC:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETDECY:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETY6:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertOffset6(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0x20)<<8)|((value2&0x18)<<7)|(value2&0x07),listingRecord);
			break;
		case AM_REG5_OFFSETZ:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETZINC:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETDECZ:
			value1=ConvertRegister5(operand1->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_REG5_OFFSETZ6:
			value1=ConvertRegister5(operand1->value);
			value2=ConvertOffset6(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4)|((value2&0x20)<<8)|((value2&0x18)<<7)|(value2&0x07),listingRecord);
			break;
		case AM_OFFSETX_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETXINC_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETDECX_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETY_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETYINC_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETDECY_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETY6_REG5:
			value1=ConvertOffset6(operand1->value);
			value2=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value2<<4)|((value1&0x20)<<8)|((value1&0x18)<<7)|(value1&0x07),listingRecord);
			break;
		case AM_OFFSETZ_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETZINC_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETDECZ_REG5:
			value1=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value1<<4),listingRecord);
			break;
		case AM_OFFSETZ6_REG5:
			value1=ConvertOffset6(operand1->value);
			value2=ConvertRegister5(operand2->value);
			fail=!GenerateAVRWord(addressingMode->baseOpcode|(value2<<4)|((value1&0x20)<<8)|((value1&0x18)<<7)|(value1&0x07),listingRecord);
			break;
	}
	return(!fail);
}

static bool OperandsMatchAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *operand1,OPERAND *operand2)
// See if the given addressing mode matches the passed operands
// return true for a match, false if no match
{
	switch(addressingMode->mode)
	{
		case AM_IMPLIED:
			return(numOperands==0);
			break;
		case AM_REG3_REG3:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REGISTER));
			break;
		case AM_REG4:
			return((numOperands==1)&&(operand1->type==OT_REGISTER));
			break;
		case AM_REG4_REG4:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REGISTER));
			break;
		case AM_REG4_IMMEDIATE8:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REG4_NOTIMMEDIATE8:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REG5:
			return((numOperands==1)&&(operand1->type==OT_REGISTER));
			break;
		case AM_REG5_REG5:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REGISTER));
			break;
		case AM_REG5_DUP:
			return((numOperands==1)&&(operand1->type==OT_REGISTER));
			break;
		case AM_REG5_BIT:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REG5_ADDR6:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REG5_ADDR16:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REGPAIR2_IMMEDIATE6:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REGPAIR4_REGPAIR4:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REGISTER));
			break;
		case AM_SREG:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_SREG_RELATIVE7:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_VALUE));
			break;
		case AM_RELATIVE7:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_RELATIVE12:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_ADDR5_BIT:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_VALUE));
			break;
		case AM_ADDR6_REG5:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_REGISTER));
			break;
		case AM_ADDR16_REG5:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_REGISTER));
			break;
		case AM_ADDR22:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_REG5_OFFSETX:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_X));
			break;
		case AM_REG5_OFFSETXINC:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_XINC));
			break;
		case AM_REG5_OFFSETDECX:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_DECX));
			break;
		case AM_REG5_OFFSETY:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_Y));
			break;
		case AM_REG5_OFFSETYINC:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_YINC));
			break;
		case AM_REG5_OFFSETDECY:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_DECY));
			break;
		case AM_REG5_OFFSETY6:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_Y_OFF));
			break;
		case AM_REG5_OFFSETZ:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_Z));
			break;
		case AM_REG5_OFFSETZINC:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_ZINC));
			break;
		case AM_REG5_OFFSETDECZ:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_DECZ));
			break;
		case AM_REG5_OFFSETZ6:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_REG_Z_OFF));
			break;
		case AM_OFFSETX_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_X)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETXINC_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_XINC)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETDECX_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_DECX)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETY_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_Y)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETYINC_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_YINC)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETDECY_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_DECY)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETY6_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_Y_OFF)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETZ_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_Z)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETZINC_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_ZINC)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETDECZ_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_DECZ)&&(operand2->type==OT_REGISTER));
			break;
		case AM_OFFSETZ6_REG5:
			return((numOperands==2)&&(operand1->type==OT_REG_Z_OFF)&&(operand2->type==OT_REGISTER));
			break;
	}
	return(false);
}

static bool ParseOperands(const char *line,unsigned int *lineIndex,unsigned int *numOperands,OPERAND *operand1,OPERAND *operand2)
// parse from 0 to 2 operands from the line
// return the operands parsed
// If something does not look right, return false
{
	bool
		fail;

	fail=false;
	*numOperands=0;
	if(!ParseComment(line,lineIndex))	// make sure not at end of line
	{
		if(ParseOperand(line,lineIndex,operand1))
		{
			*numOperands=1;
			if(!ParseComment(line,lineIndex))	// make sure not at end of line
			{
				if(ParseCommaSeparator(line,lineIndex))
				{
					if(!ParseComment(line,lineIndex))	// make sure not at end of line
					{
						if(ParseOperand(line,lineIndex,operand2))
						{
							*numOperands=2;
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
	return(!fail);
}

static OPCODE *MatchOpcode(const char *string)
// match opcodes for this processor, return NULL if none matched
{
	OPCODE
		*result;

	result=NULL;
	if(currentProcessor->processorData)
	{
		result=(OPCODE *)STFindDataForNameNoCase(*((SYM_TABLE **)(currentProcessor->processorData)),string);	// search enhanced opcodes first
	}
	if(!result)
	{
		result=(OPCODE *)STFindDataForNameNoCase(opcodeSymbols,string);		// fallback to here if nothing located
	}
	return(result);
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
		operand1,
		operand2;
	unsigned int
		i;
	bool
		done;

	result=true;					// no hard failure yet
	*success=false;					// no match yet
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=MatchOpcode(string)))						// found opcode?
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;
			if(ParseOperands(line,lineIndex,&numOperands,&operand1,&operand2))	// fetch operands for opcode
			{
				done=false;
				for(i=0;!done&&(i<opcode->numModes);i++)
				{
					if(OperandsMatchAddressingMode(&(opcode->addressingModes[i]),numOperands,&operand1,&operand2))
					{
						result=HandleAddressingMode(&(opcode->addressingModes[i]),numOperands,&operand1,&operand2,listingRecord);
						done=true;
					}
				}
				if(!done)
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
	if(currentSegment)
	{
		currentSegment->currentPC=(currentSegment->currentPC+1)/2;
		currentSegment->codeGenOffset=(currentSegment->codeGenOffset+1)/2;
	}
	return(true);
}

static void DeselectProcessor(PROCESSOR *processor)
// A processor in this family is being deselected
{
	if(currentSegment)
	{
		currentSegment->currentPC*=2;
		currentSegment->codeGenOffset*=2;
	}
}

static void UnInitFamily()
// undo what InitFamily did
{
	STDisposeSymbolTable(opcodeTiny22Symbols);
	STDisposeSymbolTable(opcodeTinySymbols);
	STDisposeSymbolTable(opcodeMega8Mega161Symbols);
	STDisposeSymbolTable(opcodeMega103Symbols);
	STDisposeSymbolTable(opcode23xxSymbols);
	STDisposeSymbolTable(opcodeFullSymbols);
	STDisposeSymbolTable(opcodeSymbols);
	STDisposeSymbolTable(pseudoOpcodeSymbols);
}

static bool AddSymbols(SYM_TABLE **symbols,OPCODE *opcodes,unsigned int numOpCodes)
// Create a new symbol table, and add numOpCodes of opcodes to the table
// if there is a problem, return false
{
	unsigned int
		i;
	bool
		fail;

	fail=!(*symbols=STNewSymbolTable(numOpCodes));
	if(!fail)
	{
		for(i=0;!fail&&(i<numOpCodes);i++)
		{
			fail=!STAddEntryAtEnd(*symbols,opcodes[i].name,&opcodes[i]);
		}
		if(fail)
		{
			STDisposeSymbolTable(*symbols);
		}
	}
	return(!fail);
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
			if(AddSymbols(&opcodeSymbols,Opcodes,elementsof(Opcodes)))
			{
				if(AddSymbols(&opcodeFullSymbols,AdditionalFullOpcodes,elementsof(AdditionalFullOpcodes)))
				{
					if(AddSymbols(&opcode23xxSymbols,Additional23xxOpcodes,elementsof(Additional23xxOpcodes)))
					{
						if(AddSymbols(&opcodeMega103Symbols,AdditionalMega103Opcodes,elementsof(AdditionalMega103Opcodes)))
						{
							if(AddSymbols(&opcodeMega8Mega161Symbols,AdditionalMega8Mega161Opcodes,elementsof(AdditionalMega8Mega161Opcodes)))
							{
								if(AddSymbols(&opcodeTinySymbols,AdditionalTinyOpcodes,elementsof(AdditionalTinyOpcodes)))
								{
									if(AddSymbols(&opcodeTiny22Symbols,AdditionalTiny22Opcodes,elementsof(AdditionalTiny22Opcodes)))
									{
										return(true);
									}
									STDisposeSymbolTable(opcodeTinySymbols);
								}
								STDisposeSymbolTable(opcodeMega8Mega161Symbols);
							}
							STDisposeSymbolTable(opcodeMega103Symbols);
						}
						STDisposeSymbolTable(opcode23xxSymbols);
					}
					STDisposeSymbolTable(opcodeFullSymbols);
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
	processorFamily("Atmel AVR",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"avr",&opcodeFullSymbols),			// allow any instruction
		PROCESSOR(&processorFamily,"attiny10",&opcodeTinySymbols),		
		PROCESSOR(&processorFamily,"attiny11",&opcodeTinySymbols),		
		PROCESSOR(&processorFamily,"attiny12",&opcodeTinySymbols),		
		PROCESSOR(&processorFamily,"attiny15",&opcodeTinySymbols),		
		PROCESSOR(&processorFamily,"attiny22",&opcodeTiny22Symbols),	
		PROCESSOR(&processorFamily,"attiny28",&opcodeTinySymbols),		
		PROCESSOR(&processorFamily,"at90s1200",NULL),				
		PROCESSOR(&processorFamily,"at90s2313",&opcode23xxSymbols),		
		PROCESSOR(&processorFamily,"at90s2323",&opcode23xxSymbols),		
		PROCESSOR(&processorFamily,"at90s2333",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s2343",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s4414",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s4433",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s4434",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s8515",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90c8534",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"at90s8535",&opcode23xxSymbols),	
		PROCESSOR(&processorFamily,"atmega8",&opcodeMega8Mega161Symbols),
		PROCESSOR(&processorFamily,"atmega103",&opcodeMega103Symbols),	
		PROCESSOR(&processorFamily,"atmega161",&opcodeMega8Mega161Symbols),
	};
