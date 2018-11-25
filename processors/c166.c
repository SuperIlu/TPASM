//
//
// Generate code for the Siemens/ST/Infineon C166 style processors
//
// Built from the Z80 code generator. See copyright below.
//
// Copyright(C) 2010 Gustaf Kröling
// g.kroling@swipnet.se
//
// 100314	1.0
//

//
//-------------------------------------------------------------------
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
//-------------------------------------------------------------------
//
//  Generate code for the Zilog Z80 and Z180/Hitachi 64180
//   copyright (c) 2000 Cosmodog, Ltd.
//




#include	"include.h"
#include	<limits.h>

static bool HandleDefr(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleLIT(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleEven(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleSegmented(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleNonSegmented(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleAssume(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandleExtern(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool bigEndian);
bool HandlePLEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols;

static PROCESSOR
	*currentProcessor;

static	bool	segmented;
static	unsigned int	dpp0;
static	unsigned int	dpp1;
static	unsigned int	dpp2;
static	unsigned int	dpp3;

// register codes

#define RCODE_R0	0
#define RCODE_R1	1
#define RCODE_R2	2
#define RCODE_R3	3
#define RCODE_R4	4
#define RCODE_R5	5
#define RCODE_R6	6
#define RCODE_R7	7
#define RCODE_R8	8
#define RCODE_R9	9
#define RCODE_R10	10
#define RCODE_R11	11
#define RCODE_R12	12
#define RCODE_R13	13
#define RCODE_R14	14
#define RCODE_R15	15

#define RCODE_RL0	0
#define RCODE_RH0	1
#define RCODE_RL1	2
#define RCODE_RH1	3
#define RCODE_RL2	4
#define RCODE_RH2	5
#define RCODE_RL3	6
#define RCODE_RH3	7
#define RCODE_RL4	8
#define RCODE_RH4	9
#define RCODE_RL5	10
#define RCODE_RH5	11
#define RCODE_RL6	12
#define RCODE_RH6	13
#define RCODE_RL7	14
#define RCODE_RH7	15


// codes for condition codes

#define	F_UC	0	
#define	F_Z	2
#define	F_NZ	3
#define	F_V	4	
#define	F_NV	5
#define	F_N	6	
#define	F_NN	7	
#define	F_C	8	
#define	F_NC	9	
#define	F_EQ	2	
#define	F_NE	3	
#define	F_ULT	8	
#define	F_ULE	15	
#define	F_UGE	9	
#define	F_UGT	14	
#define	F_USLT	12	
#define	F_USLE	11	
#define	F_USGE	13	
#define	F_USGT	10	
#define	F_UNET	1
	

struct REGISTER
{
	unsigned int
		type;		// type of operand located
	int
		value;		// register offset
};

// enumerated register types (used when parsing)
// This table must match the rcode[] table
enum
{
	R_RH7,
	R_RL7,
	R_RH6,
	R_RL6,
	R_RH5,
	R_RL5,
	R_RH4,
	R_RL4,
	R_RH3,
	R_RL3,
	R_RH2,
	R_RL2,
	R_RH1,
	R_RL1,
	R_RH0,
	R_RL0,

	R_R15,
	R_R14,
	R_R13,
	R_R12,
	R_R11,
	R_R10,
	R_R9,
	R_R8,
	R_R7,
	R_R6,
	R_R5,
	R_R4,
	R_R3,
	R_R2,
	R_R1,
	R_R0,

	R_DPP0,
	R_DPP1,
	R_DPP2,
	R_DPP3,
	R_SEG,
	R_SOF,
	R_POF,
	R_PAG,


};

// enumerated operand types (used when parsing)

enum
{
	OT_R15,
	OT_R14,
	OT_R13,
	OT_R12,
	OT_R11,
	OT_R10,
	OT_R9,
	OT_R8,
	OT_R7,
	OT_R6,
	OT_R5,
	OT_R4,
	OT_R3,
	OT_R2,
	OT_R1,
	OT_R0,


	OT_INDIRECT,
	OT_VALUE,			// This stands for almost any value except immediate
	OT_NOTVALUE,
	OT_FLAG,		
	OT_RW_DIRECT,		
	OT_RB_DIRECT,		
	OT_REG_DIRECT,		
	OT_BITADDR_DIRECT,	
	OT_BITADDR_RW_DIRECT,	
	OT_BITOFF_DIRECT,
	OT_IRANGE2,
	OT_DATA2,		
	OT_DATA3,		
	OT_DATA4,		
	OT_DATA8,		
	OT_DATA16,		
	OT_DATA32,
	OT_MEM_DPPX,		
	OT_RW_INDIRECT,		
	OT_RW_INC_INDIRECT,	
	OT_DEC_RW_INDIRECT,	
	OT_RW_DATA16_INDIRECT,	

};


static const unsigned short rcode[] =
{
	RCODE_RH7,
	RCODE_RL7,
	RCODE_RH6,
	RCODE_RL6,
	RCODE_RH5,
	RCODE_RL5,
	RCODE_RH4,
	RCODE_RL4,
	RCODE_RH3,
	RCODE_RL3,
	RCODE_RH2,
	RCODE_RL2,
	RCODE_RH1,
	RCODE_RL1,
	RCODE_RH0,
	RCODE_RL0,

	RCODE_R15,
	RCODE_R14,
	RCODE_R13,
	RCODE_R12,
	RCODE_R11,
	RCODE_R10,
	RCODE_R9,
	RCODE_R8,
	RCODE_R7,
	RCODE_R6,
	RCODE_R5,
	RCODE_R4,
	RCODE_R3,
	RCODE_R2,
	RCODE_R1,
	RCODE_R0,
};


struct REGISTER_NAME
{
	unsigned int
		type;
	const char
		*name;
};

static const REGISTER_NAME registerList[] =
{
	{R_SEG, "SEG" },
	{R_SOF, "SOF" },
	{R_POF, "POF" },
	{R_PAG, "PAG" },

	{R_RH7, "RH7" },
	{R_RL7, "RL7" },
	{R_RH6, "RH6" },
	{R_RL6, "RL6" },
	{R_RH5, "RH5" },
	{R_RL5, "RL5" },
	{R_RH4, "RH4" },
	{R_RL4, "RL4" },
	{R_RH3, "RH3" },
	{R_RL3, "RL3" },
	{R_RH2, "RH2" },
	{R_RL2, "RL2" },
	{R_RH1, "RH1" },
	{R_RL1, "RL1" },
	{R_RH0, "RH0" },
	{R_RL0, "RL0" },
	{R_R15, "R15" },
	{R_R14, "R14" },
	{R_R13, "R13" },
	{R_R12, "R12" },
	{R_R11, "R11" },
	{R_R10, "R10" },
	{R_R9, "R9" },
	{R_R8, "R8" },
	{R_R7, "R7" },
	{R_R6, "R6" },
	{R_R5, "R5" },
	{R_R4, "R4" },
	{R_R3, "R3" },
	{R_R2, "R2" },
	{R_R1, "R1" },
	{R_R0, "R0" },
};



static const REGISTER_NAME flagList[] =
{
	{F_USLT,"cc_SLT"},
	{F_USLE,"cc_SLE"},
	{F_USGE,"cc_SGE"},
	{F_USGT,"cc_SGT"},
	{F_UNET,"cc_NET"},
	{F_ULT,	"cc_ULT"},
	{F_ULE,	"cc_ULE"},
	{F_UGE,	"cc_UGE"},
	{F_UGT,	"cc_UGT"},
	{F_UC,	"cc_UC"	},
	{F_NZ,	"cc_NZ"	},
	{F_NV,	"cc_NV"	},
	{F_NN,	"cc_NN"	},
	{F_NC,	"cc_NC"	},
	{F_EQ,	"cc_EQ"	},
	{F_NE,	"cc_NE"	},
	{F_Z,	"cc_Z"	},
	{F_V,	"cc_V"	},
	{F_N,	"cc_N"	},
	{F_C,	"cc_C"	},
};


static const TOKEN
	dppList[]=
	{
		{"DPP0",0},
		{"DPP1",1},
		{"DPP2",2},
		{"DPP3",3},
		{"dpp0",0},
		{"dpp1",1},
		{"dpp2",2},
		{"dpp3",3},
		{"",0}
	};

static const TOKEN
	nothing[]=
	{
		{"NOTHING",1},
		{"",0}
	};



struct OPERAND
{
	unsigned int
		type;			// type of operand located
	int
		value,			// first immediate value, non-immediate value
		bit,			// bit number if present
		dpp,			// data page pointer if used
		offset;			// offset used by register indirect mode
	bool
		unresolved,		// if value was an expression, this tells if it was unresolved
		bitUnresolved;		// if bit number was an expression, this tells if it was unresolved
};


// enumerated addressing modes (yikes!)

enum
{
	AM_IMP,				// implied (no operands)
	AM_RELATIVE,			// relative (for branching)
	AM_FLAG_RELATIVE,		// 
	AM_NOFLAG_RELATIVE,		// pseudo AM for implied cc_UC flag
	AM_NOFLAG_RWI,			// pseudo AM for implied cc_UC flag
	AM_RB_RB,
	AM_RB_RWI,
	AM_RWI_RB,
	AM_RB_RWI_INC,
	AM_RB_MEM,
	AM_MEM_RB,
	AM_RB_DATA4,
	AM_RB_DATA8,
	AM_RW_RW,			// op n0
	AM_RW_RB,
	AM_RW_RWI,			// op nm	Moves
	AM_RW_RWI2,			// op n:10ii	Arithmetics
	AM_RB_RWI2,			// op n:10ii	Arithmetics
	AM_RW_RWI2_INC,			// op n:11ii	Arithmetics
	AM_RB_RWI2_INC,			// op n:11ii	Arithmetics
	AM_RWI_RW,
	AM_RW_RWI16,
	AM_RWI16_RW,
	AM_RB_RWI16,
	AM_RWI16_RB,
	AM_RWI_RWI,
	AM_RW_RWI_INC,
	AM_RWI_INC_RWI,
	AM_RWI_RWI_INC,
	AM_RW_DATA3,
	AM_RB_DATA3,
	AM_REG_DATA16,
	AM_REG_DATA8,
	AM_REG_MEM,
	AM_MEM_REG,
	AM_RW_DATA4,
	AM_RW_DATA8,
	AM_RW_DATA16,			// op Fn ## ##
	AM_REG_CADDR,			// op RR MM MM
	AM_RW_CADDR,			// op Fn MM MM
	AM_SEG_CADDR,
	AM_BITADDR_DIRECT,
	AM_BITADDR_BITADDR,
	AM_BITADDR_REL,
	AM_REG,
	AM_RW,
	AM_RB,
	AM_FLAG_ABSOLUTE,
	AM_NOFLAG_ABSOLUTE,
	AM_FLAG_RWI,
	AM_IRANG2,			// op :aa##-0		EXTR ATOMIC
	AM_RW_IRANG2,			// op :aa##-m		EXTS...
	AM_SEG_IRANG2,			// op :aa##-0 ss 00 	EXTS...
	AM_PAG_IRANG2,			// op :aa##-0 pp pp 	EXTP...
	AM_RWI_MEM,
	AM_MEM_RWI,
	AM_RW_MEM,
	AM_MEM_RW,
	AM_DEC_RWI_RW,
	AM_DEC_RWI_RB,
	AM_TRAP7,
	AM_REG_DATA8_DATA8_L,		// op reg mask data8
	AM_RW_DATA8_DATA8_L,		// op RW mask data8
	AM_REG_DATA8_DATA8_H,		// op reg data8 mask
	AM_RW_DATA8_DATA8_H,		// op RW data8 mask
};

struct ADDRESSING_MODE
{
	unsigned int
		mode;				// address mode
	unsigned char
		opcodeLen;			// total number of bytes in the opcode
	unsigned int
		baseOpcode;			// base opcode for this combination of modes
	unsigned char
		trailLen;			// number of trailing bytes (1 or 0)
	unsigned char
		trailOpcode;			// trailing opcode byte ( bits aa in address modes above )
	unsigned char
		regOffset;			// number of positions left to shift register bit code (usually 0)
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

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"lit",		HandleLIT},
		{"defb",	HandleLIT},		// just do a LIT of this
		{"defr",	HandleDefr},		// just do an EQU of this
		{"defa",	HandleDefr},		// just do an EQU of this
		{"even",	HandleEven},
		{"assume",	HandleAssume},
		{"segmented",	HandleSegmented},
		{"nonsegmented",HandleNonSegmented},
		{"db",		HandleDB},
		{"dw",		HandlePLEDW},		// words are little endian
		{"ds",		HandleDS},
		{"dsw",		HandleDSW},
		{"incbin",	HandleIncbin},
		{"extrn",	HandleExtern},		// ignored
		{"extern",	HandleExtern},		// ignored
		{"retv",	HandleExtern},		// ignored
		{"proc",	HandleExtern},		// ignored
		{"name",	HandleExtern},		// ignored
		{"regdef",	HandleExtern},		// ignored
		{"sskdef",	HandleExtern},		// ignored
		{"public",	HandleExtern},		// ignored
		{"global",	HandleExtern},		// ignored
		{"cgroup",	HandleExtern},		// ignored
		{"dgroup",	HandleExtern},		// ignored
		{"endp",	HandleExtern},		// ignored
	};

#define	MODES(modeArray)	sizeof(modeArray)/sizeof(ADDRESSING_MODE),&modeArray[0]


//
// Always put GPR modes before the corresponding REG modes
//
static ADDRESSING_MODE
	M_ADD[]=
	{
		{AM_RW_RW,		1,	0x00,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x08,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x08,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x08,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x06,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x06,	0,	0,		0},
		{AM_RW_MEM,		1,	0x02,	0,	0,		4},
		{AM_REG_MEM,		1,	0x02,	0,	0,		4},
		{AM_MEM_RW,		1,	0x04,	0,	0,		4},
		{AM_MEM_REG,		1,	0x04,	0,	0,		4},
	},
	M_ADDB[]=
	{
		{AM_RB_RB,		1,	0x01,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x09,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x09,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x09,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x07,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x07,	0,	0,		0},
		{AM_RW_MEM,		1,	0x03,	0,	0,		4},
		{AM_REG_MEM,		1,	0x03,	0,	0,		4},
		{AM_MEM_RB,		1,	0x05,	0,	0,		4},
		{AM_MEM_REG,		1,	0x05,	0,	0,		4},
	},
	M_ADDC[]=
	{
		{AM_RW_RW,		1,	0x10,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x18,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x18,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x18,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x16,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x16,	0,	0,		0},
		{AM_RW_MEM,		1,	0x12,	0,	0,		4},
		{AM_REG_MEM,		1,	0x12,	0,	0,		4},
		{AM_MEM_RW,		1,	0x14,	0,	0,		4},
		{AM_MEM_REG,		1,	0x14,	0,	0,		4},
	},
	M_ADDCB[]=
	{
		{AM_RB_RB,		1,	0x11,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x19,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x19,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x19,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x17,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x17,	0,	0,		0},
		{AM_RB_MEM,		1,	0x13,	0,	0,		4},
		{AM_REG_MEM,		1,	0x13,	0,	0,		4},
		{AM_MEM_RB,		1,	0x15,	0,	0,		4},
		{AM_MEM_REG,		1,	0x15,	0,	0,		4},
	},
	M_AND[]=
	{
		{AM_RW_RW,		1,	0x60,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x68,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x68,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x68,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x66,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x66,	0,	0,		0},
		{AM_RW_MEM,		1,	0x62,	0,	0,		4},
		{AM_REG_MEM,		1,	0x62,	0,	0,		4},
		{AM_MEM_RW,		1,	0x64,	0,	0,		4},
		{AM_MEM_REG,		1,	0x64,	0,	0,		4},
	},
	M_ANDB[]=
	{
		{AM_RB_RB,		1,	0x61,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x69,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x69,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x69,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x67,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x67,	0,	0,		0},
		{AM_RB_MEM,		1,	0x63,	0,	0,		4},
		{AM_REG_MEM,		1,	0x63,	0,	0,		4},
		{AM_MEM_RB,		1,	0x65,	0,	0,		4},
		{AM_MEM_REG,		1,	0x65,	0,	0,		4},
	},
	M_ASHR[]=
	{
		{AM_RW_DATA4,		1,	0xBC,	0,	0,		0},
		{AM_RW_RW,		1,	0xAC,	0,	0,		0},
	},
	M_ATOMIC[]=
	{
		{AM_IRANG2,		1,	0xD1,	0,	0x00,		0},
	},
	M_BAND[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x6A,	0,	0,		0},
	},
	M_BCLR[]=
	{
		{AM_BITADDR_DIRECT,	1,	0x0E,	0,	0,		0},
	},
	M_BCMP[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x2A,	0,	0,		0},
	},
	M_BFLDH[]=
	{
		{AM_RW_DATA8_DATA8_H,	1,	0x1A,	0,	0,		0},
		{AM_REG_DATA8_DATA8_H,	1,	0x1A,	0,	0,		0},
	},
	M_BFLDL[]=
	{
		{AM_RW_DATA8_DATA8_L,	1,	0x0A,	0,	0,		0},
		{AM_REG_DATA8_DATA8_L,	1,	0x0A,	0,	0,		0},
	},
	M_BMOV[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x4A,	0,	0,		0},
	},
	M_BMOVN[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x3A,	0,	0,		0},
	},
	M_BOR[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x5A,	0,	0,		0},
	},
	M_BSET[]=
	{
		{AM_BITADDR_DIRECT,	1,	0x0F,	0,	0,		0},
	},
	M_BXOR[]=
	{
		{AM_BITADDR_BITADDR,	1,	0x7A,	0,	0,		0},
	},
	M_CALLA[]=
	{
		{AM_NOFLAG_ABSOLUTE,	1,	0xCA,	0,	0,		0},
		{AM_FLAG_ABSOLUTE,	1,	0xCA,	0,	0,		0},
	},
	M_CALLI[]=
	{
		{AM_FLAG_RWI,		1,	0xAB,	0,	0,		0},
		{AM_NOFLAG_RWI,		1,	0xAB,	0,	0,		0},
	},
	M_CALLR[]=
	{
		{AM_RELATIVE,		1,	0xBB,	0,	0,		0},
	},
	M_CALLS[]=
	{
		{AM_SEG_CADDR,		1,	0xDA,	0,	0,		0},
	},

	M_CMP[]=
	{
		{AM_RW_RW,		1,	0x40,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x48,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x48,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x48,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x46,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x46,	0,	0,		0},
		{AM_RW_MEM,		1,	0x42,	0,	0,		4},
		{AM_REG_MEM,		1,	0x42,	0,	0,		4},
	},
	M_CMPB[]=
	{
		{AM_RB_RB,		1,	0x41,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x49,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x49,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x49,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x47,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x47,	0,	0,		0},
		{AM_RB_MEM,		1,	0x43,	0,	0,		4},
		{AM_REG_MEM,		1,	0x43,	0,	0,		4},
	},
	M_CMPD1[]=
	{
		{AM_RW_DATA4,		1,	0xA0,	0,	0,		0},
		{AM_RW_DATA16,		1,	0xA6,	0,	0,		0},
		{AM_RW_MEM,		1,	0xA2,	0,	0,		0},
	},
	M_CMPD2[]=
	{
		{AM_RW_DATA4,		1,	0xB0,	0,	0,		0},
		{AM_RW_DATA16,		1,	0xB6,	0,	0,		0},
		{AM_RW_MEM,		1,	0xB2,	0,	0,		0},
	},
	M_CMPI1[]=
	{
		{AM_RW_DATA4,		1,	0x80,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x86,	0,	0,		0},
		{AM_RW_MEM,		1,	0x82,	0,	0,		0},

	},
	M_CMPI2[]=
	{
		{AM_RW_DATA4,		1,	0x90,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x96,	0,	0,		0},
		{AM_RW_MEM,		1,	0x92,	0,	0,		0},
	},
	M_CPL[]=
	{
		{AM_RW,			1,	0x91,	0,	0,		0},
	},
	M_CPLB[]=
	{
		{AM_RB,			1,	0xB1,	0,	0,		0},
	},
	M_DISWDT[]=
	{
		{AM_IMP,		4,	0xA55AA5A5,	0,	0,		0},
	},
	M_DIV[]=
	{
		{AM_RW,			2,	0x4B,	0,	0,		0},
	},

	M_DIVL[]=
	{
		{AM_RW,			2,	0x6B,	0,	0,		0},
	},

	M_DIVLU[]=
	{
		{AM_RW,			2,	0x7B,	0,	0,		0},
	},

	M_DIVU[]=
	{
		{AM_RW,			2,	0x5B,	0,	0,		0},
	},
	M_EINIT[]=
	{
		{AM_IMP,		4,	0xB54AB5B5,	0,	0,		0},
	},
	M_EXTP[]=
	{
		{AM_RW_IRANG2,		1,	0xDC,	1,	0x40,		0},
		{AM_PAG_IRANG2,		1,	0xD7,	1,	0x40,		0},
	},
	M_EXTPR[]=
	{
		{AM_RW_IRANG2,		1,	0xDC,	1,	0xC0,		0},
		{AM_PAG_IRANG2,		1,	0xD7,	1,	0xC0,		0},
	},
	M_EXTR[]=
	{
		{AM_IRANG2,		1,	0xD1,	1,	0x80,		0},
	},
	M_EXTS[]=
	{
		{AM_RW_IRANG2,		1,	0xDC,	1,	0x00,		0},
		{AM_SEG_IRANG2,		1,	0xD7,	1,	0x00,		0},
	},
	M_EXTSR[]=
	{
		{AM_RW_IRANG2,		1,	0xDC,	1,	0x80,		0},
		{AM_SEG_IRANG2,		1,	0xD7,	1,	0x80,		0},
	},

	M_IDLE[]=
	{
		{AM_IMP,		4,	0x87788787,	0,	0,		0},
	},
	M_JB[]=
	{
		{AM_BITADDR_REL,	4,	0x8A,	0,	0,		0},
	},
	M_JBC[]=
	{
		{AM_BITADDR_REL,	4,	0xAA,	0,	0,		0},
	},
	M_JMPA[]=
	{
		{AM_NOFLAG_ABSOLUTE,	1,	0xEA,	0,	0,		0},
		{AM_FLAG_ABSOLUTE,	1,	0xEA,	0,	0,		0},
	},
	M_JMPI[]=
	{
		{AM_FLAG_RWI,		1,	0x9C,	0,	0,		0},
		{AM_NOFLAG_RWI,		1,	0x9C,	0,	0,		0},
	},
	M_JMPS[]=
	{
		{AM_SEG_CADDR,		1,	0xFA,	0,	0,		0},
	},
	M_JMPR[]=
	{
		{AM_NOFLAG_RELATIVE,	2,	0x0D,	0,	0,		0},
		{AM_FLAG_RELATIVE,	2,	0x0D,	0,	0,		0},
	},
	M_JNB[]=
	{
		{AM_BITADDR_REL,	4,	0x9A,	0,	0,		0},
	},
	M_JNBS[]=
	{
		{AM_BITADDR_REL,	4,	0xBA,	0,	0,		0},
	},
	M_MOV[]=
	{
		{AM_RW_RW,			1,	0xF0,	0,	0,		0},
		{AM_RW_RWI,			1,	0xA8,	0,	0,		0},
		{AM_RWI_RW,			1,	0xB8,	0,	0,		0},
		{AM_RWI_RWI,			1,	0xC8,	0,	0,		0},
		{AM_RWI16_RW,			1,	0xC4,	0,	0,		0},
		{AM_RW_RWI16,			1,	0xD4,	0,	0,		0},
		{AM_RW_RWI_INC,			1,	0x98,	0,	0,		0},
		{AM_RWI_INC_RWI,		1,	0xD8,	0,	0,		0},
		{AM_RWI_RWI_INC,		1,	0xE8,	0,	0,		0},
		{AM_RW_MEM,			1,	0xF2,	0,	0,		0},
		{AM_REG_MEM,			1,	0xF2,	0,	0,		0},
		{AM_MEM_RW,			1,	0xF6,	0,	0,		0},
		{AM_MEM_REG,			1,	0xF6,	0,	0,		0},
		{AM_RW_DATA4,			1,	0xE0,	0,	0,		0},
		{AM_RW_DATA16,			1,	0xE6,	0,	0,		0},
		{AM_REG_DATA16,			1,	0xE6,	0,	0,		0},
		{AM_RWI_MEM,			1,	0x84,	0,	0,		0},
		{AM_MEM_RWI,			1,	0x94,	0,	0,		0},
		{AM_DEC_RWI_RW,			1,	0x88,	0,	0,		0},
	},
	M_MOVB[]=
	{
		{AM_RB_RB,			1,	0xF1,	0,	0,		0},
		{AM_RB_RWI,			1,	0xA9,	0,	0,		0},
		{AM_RWI_RB,			1,	0xB9,	0,	0,		0},
		{AM_RWI_RWI,			1,	0xC9,	0,	0,		0},
		{AM_RWI16_RB,			1,	0xE4,	0,	0,		0},
		{AM_RB_RWI16,			1,	0xF4,	0,	0,		0},
		{AM_RB_RWI_INC,			1,	0x99,	0,	0,		0},
		{AM_RWI_INC_RWI,		1,	0xD9,	0,	0,		0},
		{AM_RWI_RWI_INC,		1,	0xE9,	0,	0,		0},
		{AM_RB_MEM,			1,	0xF3,	0,	0,		0},
		{AM_REG_MEM,			1,	0xF3,	0,	0,		0},
		{AM_MEM_RB,			1,	0xF7,	0,	0,		0},
		{AM_MEM_REG,			1,	0xF7,	0,	0,		0},
		{AM_RB_DATA4,			1,	0xE1,	0,	0,		0},
		{AM_RB_DATA8,			1,	0xE7,	0,	0,		0},
		{AM_REG_DATA8,			1,	0xE7,	0,	0,		0},
		{AM_RWI_MEM,			1,	0xA4,	0,	0,		0},
		{AM_MEM_RWI,			1,	0xB4,	0,	0,		0},
		{AM_DEC_RWI_RB,			1,	0x89,	0,	0,		0},
	},
	M_MOVBS[]=
	{
		{AM_RW_RB,			1,	0xD0,	0,	0,		0},
		{AM_RW_MEM,			1,	0xD2,	0,	0,		0},
		{AM_REG_MEM,			1,	0xD2,	0,	0,		0},
		{AM_MEM_RB,			1,	0xD5,	0,	0,		0},
		{AM_MEM_REG,			1,	0xD5,	0,	0,		0},
	},
	M_MOVBZ[]=
	{
		{AM_RW_RB,			1,	0xC0,	0,	0,		0},
		{AM_RW_MEM,			1,	0xC2,	0,	0,		0},
		{AM_REG_MEM,			1,	0xC2,	0,	0,		0},
		{AM_MEM_RB,			1,	0xC5,	0,	0,		0},
		{AM_MEM_REG,			1,	0xC5,	0,	0,		0},
	},
	M_MUL[]=
	{
		{AM_RW_RW,			1,	0x0B,	0,	0,		0},
	},
	M_MULU[]=
	{
		{AM_RW_RW,			1,	0x1B,	0,	0,		0},
	},
	M_NEG[]=
	{
		{AM_RW,			1,	0x81,	0,	0,		0},
	},
	M_NEGB[]=
	{
		{AM_RB,			1,	0xA1,	0,	0,		0},
	},
	M_NOP[]=
	{
		{AM_IMP,			2,	0xCC00,	0,	0,		0},
	},
	M_OR[]=
	{
		{AM_RW_RW,		1,	0x70,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x78,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x78,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x78,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x76,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x76,	0,	0,		0},
		{AM_REG_MEM,		1,	0x72,	0,	0,		4},
		{AM_MEM_RW,		1,	0x74,	0,	0,		4},
		{AM_MEM_REG,		1,	0x74,	0,	0,		4},
	},
	M_ORB[]=
	{
		{AM_RB_RB,		1,	0x71,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x79,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x79,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x79,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x77,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x77,	0,	0,		0},
		{AM_REG_MEM,		1,	0x73,	0,	0,		4},
		{AM_MEM_RB,		1,	0x75,	0,	0,		4},
		{AM_MEM_REG,		1,	0x75,	0,	0,		4},
	},

	M_PCALL[]=
	{
		{AM_RW_CADDR,		1,	0xE2,	0,	0,		0},
		{AM_REG_CADDR,		1,	0xE2,	0,	0,		0},
	},
	M_POP[]=
	{
		{AM_REG,			1,	0xFC,	0,	0,		4},
	},
	M_PUSH[]=
	{
		{AM_REG,			1,	0xEC,	0,	0,		4},
	},
	M_PRIOR[]=
	{
		{AM_RW_RW,			1,	0x2B,	0,	0,		0},
	},
	M_PWRDN[]=
	{
		{AM_IMP,		4,	0x97689797,	0,	0,		0},
	},


	M_RET[]=
	{
		{AM_IMP,			2,	0xCB00,	0,	0,		0},
	},
	M_RETI[]=
	{
		{AM_IMP,			2,	0xFB88,	0,	0,		0},
	},
	M_RETP[]=
	{
		{AM_REG,			1,	0xEB,	0,	0,		0},
	},
	M_RETS[]=
	{
		{AM_IMP,			2,	0xDB00,	0,	0,		0},
	},
	M_ROL[]=
	{
		{AM_RW_RW,		1,	0x0C,	0,	0,		0},
		{AM_RW_DATA4,		1,	0x1C,	0,	0,		0},
	},
	M_ROR[]=
	{
		{AM_RW_RW,		1,	0x2C,	0,	0,		0},
		{AM_RW_DATA4,		1,	0x3C,	0,	0,		0},
	},
	M_SCXT[]=
	{
		{AM_RW_DATA16,		1,	0xC6,	0,	0,		0},
		{AM_REG_DATA16,		1,	0xC6,	0,	0,		0},
		{AM_RW_MEM,		1,	0xD6,	0,	0,		0},
		{AM_REG_MEM,		1,	0xD6,	0,	0,		0},
	},
	M_SHL[]=
	{
		{AM_RW_RW,		1,	0x4C,	0,	0,		0},
		{AM_RW_DATA4,		1,	0x5C,	0,	0,		0},
	},
	M_SHR[]=
	{
		{AM_RW_RW,		1,	0x6C,	0,	0,		0},
		{AM_RW_DATA4,		1,	0x7C,	0,	0,		0},
	},
	M_SRST[]=
	{
		{AM_IMP,		4,	0xB748B7B7,	0,	0,		0},
	},

	M_SRVWDT[]=
	{
		{AM_IMP,		4,	0xA758A7A7,	0,	0,		0},
	},

	M_SUB[]=
	{
		{AM_RW_RW,		1,	0x20,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x28,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x28,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x28,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x26,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x26,	0,	0,		0},
		{AM_RW_MEM,		1,	0x22,	0,	0,		4},
		{AM_REG_MEM,		1,	0x22,	0,	0,		4},
		{AM_MEM_RW,		1,	0x24,	0,	0,		4},
		{AM_MEM_REG,		1,	0x24,	0,	0,		4},
	},
	M_SUBB[]=
	{
		{AM_RB_RB,		1,	0x21,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x29,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x29,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x29,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x27,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x27,	0,	0,		0},
		{AM_RB_MEM,		1,	0x23,	0,	0,		4},
		{AM_REG_MEM,		1,	0x23,	0,	0,		4},
		{AM_MEM_RB,		1,	0x25,	0,	0,		4},
		{AM_MEM_REG,		1,	0x25,	0,	0,		4},
	},
	M_SUBC[]=
	{
		{AM_RW_RW,		1,	0x30,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x38,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x38,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x38,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x36,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x36,	0,	0,		0},
		{AM_RW_MEM,		1,	0x32,	0,	0,		4},
		{AM_REG_MEM,		1,	0x32,	0,	0,		4},
		{AM_MEM_RW,		1,	0x34,	0,	0,		4},
		{AM_MEM_REG,		1,	0x34,	0,	0,		4},
	},
	M_SUBCB[]=
	{
		{AM_RB_RB,		1,	0x31,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x39,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x39,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x39,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x37,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x37,	0,	0,		0},
		{AM_RB_MEM,		1,	0x33,	0,	0,		4},
		{AM_REG_MEM,		1,	0x33,	0,	0,		4},
		{AM_MEM_RB,		1,	0x35,	0,	0,		4},
		{AM_MEM_REG,		1,	0x35,	0,	0,		4},
	},
	M_TRAP[]=
	{
		{AM_TRAP7,		1,	0x9B,	0,	0,		4},
		
	},
	M_XOR[]=
	{
		{AM_RW_RW,		1,	0x50,	0,	0,		0},
		{AM_RW_RWI2,		1,	0x58,	0,	0,		0},
		{AM_RW_RWI2_INC,	1,	0x58,	0,	0,		0},
		{AM_RW_DATA3,		1,	0x58,	0,	0,		0},
		{AM_RW_DATA16,		1,	0x56,	0,	0,		0},
		{AM_REG_DATA16,		1,	0x56,	0,	0,		0},
		{AM_RW_MEM,		1,	0x52,	0,	0,		4},
		{AM_REG_MEM,		1,	0x52,	0,	0,		4},
		{AM_MEM_RW,		1,	0x54,	0,	0,		4},
		{AM_MEM_REG,		1,	0x54,	0,	0,		4},
	},
	M_XORB[]=
	{
		{AM_RB_RB,		1,	0x51,	0,	0,		0},
		{AM_RB_RWI2,		1,	0x59,	0,	0,		0},
		{AM_RB_RWI2_INC,	1,	0x59,	0,	0,		0},
		{AM_RB_DATA3,		1,	0x59,	0,	0,		0},
		{AM_RB_DATA8,		1,	0x57,	0,	0,		0},
		{AM_REG_DATA8,		1,	0x57,	0,	0,		0},
		{AM_RB_MEM,		1,	0x53,	0,	0,		4},
		{AM_REG_MEM,		1,	0x53,	0,	0,		4},
		{AM_MEM_RB,		1,	0x55,	0,	0,		4},
		{AM_MEM_REG,		1,	0x55,	0,	0,		4},		
	};




static OPCODE
	Opcodes[]=
	{

		{"add",		MODES(M_ADD)	},
		{"addb",	MODES(M_ADDB)	},
		{"addc",	MODES(M_ADDC)	},
		{"addcb",	MODES(M_ADDCB)	},
		{"and",		MODES(M_AND)	},
		{"andb",	MODES(M_ANDB)	},
		{"ashr",	MODES(M_ASHR)	},
		{"atomic",	MODES(M_ATOMIC)	},
		{"band",	MODES(M_BAND)	},
		{"bclr",	MODES(M_BCLR)	},
		{"bcmp",	MODES(M_BCMP)	},
		{"bfldh",	MODES(M_BFLDH)	},
		{"bfldl",	MODES(M_BFLDL)	},
		{"bmov",	MODES(M_BMOV)	},
		{"bmovn",	MODES(M_BMOVN)	},
		{"bor",		MODES(M_BOR)	},
		{"bset",	MODES(M_BSET)	},
		{"bxor",	MODES(M_BXOR)	},
		{"calla",	MODES(M_CALLA)	},
		{"calli",	MODES(M_CALLI)	},
		{"callr",	MODES(M_CALLR)	},
		{"calls",	MODES(M_CALLS)	},
		{"cmp",		MODES(M_CMP)	},
		{"cmpb",	MODES(M_CMPB)	},
		{"cmpd1",	MODES(M_CMPD1)	},
		{"cmpd2",	MODES(M_CMPD2)	},
		{"cmpi1",	MODES(M_CMPI1)	},
		{"cmpi2",	MODES(M_CMPI2)	},
		{"cpl",		MODES(M_CPL)	},
		{"cplb",	MODES(M_CPLB)	},
		{"diswdt",	MODES(M_DISWDT)		},
		{"div",		MODES(M_DIV)		},
		{"divl",	MODES(M_DIVL)		},
		{"divlu",	MODES(M_DIVLU)		},
		{"divu",	MODES(M_DIVU)		},
		{"einit",	MODES(M_EINIT)		},
		{"extp",	MODES(M_EXTP)		},
		{"extpr",	MODES(M_EXTPR)		},
		{"extr",	MODES(M_EXTR)		},
		{"exts",	MODES(M_EXTS)		},
		{"extsr",	MODES(M_EXTSR)		},
		{"idle",	MODES(M_IDLE)		},
		{"jb",		MODES(M_JB)		},
		{"jbc",		MODES(M_JBC)		},
		{"jnb",		MODES(M_JNB)		},
		{"jnbs",	MODES(M_JNBS)		},
		{"jmpa",	MODES(M_JMPA)		},
		{"jmpi",	MODES(M_JMPI)		},
		{"jmpr",	MODES(M_JMPR)		},
		{"jmps",	MODES(M_JMPS)		},
		{"mov",		MODES(M_MOV)		},
		{"movb",	MODES(M_MOVB)		},
		{"movbs",	MODES(M_MOVBS)		},
		{"movbz",	MODES(M_MOVBZ)		},
		{"mul",		MODES(M_MUL)		},
		{"mulu",	MODES(M_MULU)		},
		{"neg",		MODES(M_NEG)	},
		{"negb",	MODES(M_NEGB)	},
		{"nop",		MODES(M_NOP)	},
		{"or",		MODES(M_OR)		},
		{"orb",		MODES(M_ORB)		},
		{"pcall",	MODES(M_PCALL)	},
		{"pop",		MODES(M_POP)	},
		{"prior",	MODES(M_PRIOR)	},
		{"push",	MODES(M_PUSH)	},
		{"pwrdn",	MODES(M_PWRDN)	},
		{"ret",		MODES(M_RET)	},
		{"reti",	MODES(M_RETI)	},
		{"rets",	MODES(M_RETS)	},
		{"retp",	MODES(M_RETP)	},
		{"rol",		MODES(M_ROL)	},
		{"ror",		MODES(M_ROR)	},
		{"scxt",	MODES(M_SCXT)	},
		{"shl",		MODES(M_SHL)	},
		{"shr",		MODES(M_SHR)	},
		{"srst",	MODES(M_SRST)	},
		{"srvwdt",	MODES(M_SRVWDT)	},
		{"sub",		MODES(M_SUB)	},
		{"subb",	MODES(M_SUBB)	},
		{"subc",	MODES(M_SUBC)	},
		{"subcb",	MODES(M_SUBCB)	},
		{"trap",	MODES(M_TRAP)	},
		{"xor",		MODES(M_XOR)	},
		{"xorb",	MODES(M_XORB)	},
	};

//-------------------------------------------------------------------
//
// generate a one to four byte opcode as appropriate
// This will return false only if a "hard" error occurs

static bool GenerateOpcode(unsigned int value, unsigned char opcodeLen, LISTING_RECORD *listingRecord)
{
	bool fail = false;

	if(opcodeLen == 4)
	{
		fail = !GenerateByte((value>>24)&0xff,listingRecord);
	}
	if(opcodeLen >= 3)
	{
		fail = !GenerateByte((value>>16)&0xff,listingRecord);
	}
	if(opcodeLen >= 2)
	{
		fail = !GenerateByte((value>>8)&0xff,listingRecord);
	}
	if(!fail)
	{
		fail = !GenerateByte(value&0xff,listingRecord);
	}
	return(!fail);
}

//
// build a long memory address using a relevant DPP if any
// This will return false if no valid DPPn content is found

static bool BuildDPPAddress(unsigned int value, unsigned int *mem)
{
	unsigned int
		temp;

	temp=value>>14;
	value=value&0x3fff;
	if(temp==dpp0)
	{
		*mem=value;
		return(true);
	}
	else
	if(temp==dpp1)
	{
		*mem=value|0x4000;
		return(true);
	}
	else
	if(temp==dpp2)
	{
		*mem=value|0x8000;
		return(true);
	}
	else
	if(temp==dpp3)
	{
		*mem=value|0xc000;
		return(true);
	}
	return(false);			// mem is undefined here
}

//
// generate a long memory address
// This will return false if no valid DPPn content is found

static bool GenerateDPPAddress(OPERAND *operand,unsigned int value,LISTING_RECORD *listingRecord)
{
unsigned int mem;

	if( operand->dpp!=-1)
	{
		value=(value&0x3fff)|(operand->dpp<<14);
		return(GenerateWord(value,listingRecord,false));
	}
	else
	if(!BuildDPPAddress(value,&mem))
	{
		 return(false);
	}
	return(GenerateWord(mem,listingRecord,false));
}




//-------------------------------------------------------------------
//
// skip whitespace then check for end of string, fail if not
//
static bool ParseEOS(const char *line,unsigned int *lineIndex)
{
	SkipWhiteSpace(line,lineIndex);				// move past any white space
	return(!line[*lineIndex]);				// return true only if at end of string
}

//-------------------------------------------------------------------
//
// Try to parse out what looks like a register
// This will match a pattern from registerList
// NOTE: if what was located does not look like a register return false
//
static bool ParseRegister(const char *line,unsigned int *lineIndex,REGISTER *reg)
{
	unsigned int inputIndex;
	unsigned int i;

	SkipWhiteSpace(line,lineIndex);				// move past any white space
	inputIndex=*lineIndex;

	for(i = 0; i < elementsof(registerList); i++)
	{
		int len = strlen(registerList[i].name);

		if(strncasecmp(&line[inputIndex],registerList[i].name,len)==0)
		{
			reg->type=registerList[i].type;
			inputIndex+=len;
			break;
		}
	}

	if(inputIndex!=*lineIndex)	// was something located?
	{
		if(!IsLabelChar(line[inputIndex]))	// make sure the character following this does not look like part of a label
		{
			*lineIndex=inputIndex;
			return(true);
		}
	}
	return(false);
}

static bool CheckBitRange(int value,bool generateMessage,bool isError)
// see if value can be used as a bit number (0-15)
// if not, return false, and generate a warning or error if told to
{
	if( (value>=0) && (value<=15) )
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Bit number (0x%X) out of range\n",value);
	}
	return(false);
}


//-------------------------------------------------------------------
//
// Try to parse out what looks like a flag
// This will match a pattern from flagList
// NOTE: if what was located does not look like a flag return false
//
static bool ParseFlag(const char *line,unsigned int *lineIndex,REGISTER *reg)
{
	unsigned int inputIndex;
	unsigned int i;

	SkipWhiteSpace(line,lineIndex);
	inputIndex=*lineIndex;

	for(i = 0; i < elementsof(flagList); i++)
	{
		int len = strlen(flagList[i].name);

		if(strncasecmp(&line[inputIndex],flagList[i].name,len)==0)
		{
			reg->type=flagList[i].type;
			inputIndex+=len;
			break;
		}
	}

	if(inputIndex!=*lineIndex)	// was something located?
	{
		if(!IsLabelChar(line[inputIndex]))	// make sure the character following this does not look like part of a label
		{
			*lineIndex=inputIndex;
			return(true);
		}
	}
	return(false);
}




bool ParseBracketedString(const char *line,unsigned int *lineIndex,char *string)
// Parse out a string which is enclosed in brackets
// line must begin with an open bracket, and will be
// placed into string until a matching close bracket is seen
// if a comment, this will stop parsing, and
// return false
// NOTE: this understands that there may be quoted material between the
// brackets, and abides by the quotes
// NOTE: the brackets that enclose the string are not included in
// the parsed output
// NOTE: This code was copied from parser.c and slightly modified
//
{
	bool
		fail;
	unsigned int
		initialIndex,
		workingIndex;
	unsigned int
		parenDepth;
	unsigned int
		stringLength;

	fail=false;
	parenDepth=0;
	SkipWhiteSpace(line,lineIndex);
	if(line[*lineIndex]=='[')				// open bracket?
	{
		parenDepth++;
		initialIndex=workingIndex=(*lineIndex)+1;
		while(parenDepth&&!ParseComment(line,&workingIndex))
		{
			switch(line[workingIndex])
			{
				case '[':
					parenDepth++;
					break;
				case ']':
					parenDepth--;
					break;
				case '\'':
					fail=!ParseQuotedString(line,&workingIndex,'\'','\'',string,&stringLength);
					break;
				case '"':
					fail=!ParseQuotedString(line,&workingIndex,'"','"',string,&stringLength);
					break;
			}
			workingIndex++;
		}
		if(!parenDepth)							// finished?
		{
			*lineIndex=workingIndex;
			workingIndex--;						// step back off the last parenthesis
			strncpy(string,&line[initialIndex],workingIndex-initialIndex);
			string[workingIndex-initialIndex]='\0';
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


//-------------------------------------------------------------------
//
// Parse a prefixed expression
// This will take care of a leading DPPn:
//
static bool ParsePrefixedExpression(const char *line,unsigned int *lineIndex,int *value,int *dppn, bool *unresolved)
{
	const TOKEN
		*match;
	unsigned int
		index;

	SkipWhiteSpace(line,lineIndex);
	index=*lineIndex;
	*dppn=-1;
	if((match=MatchBuriedToken(line,lineIndex,dppList)))
	{
		SkipWhiteSpace(line,lineIndex);
		if(line[*lineIndex]==':')
		{
			if(!segmented) return(false);	// Don't accept the prefix in nonsegmented mode 
			(*lineIndex)++;
			if(!ParseExpression(line,lineIndex,value,unresolved)) return(false);
			*dppn=match->value;
			return(true);
		}
		else
		{
			*dppn=-1;
			*lineIndex=index;		// get back to where DPPn started, it was no prefix
			return(ParseExpression(line,lineIndex,value,unresolved));	// now parse it all
		}
	}
	else
	{
		*dppn=-1;
		return(ParseExpression(line,lineIndex,value,unresolved));	// do it the normal way
	}
}

//-------------------------------------------------------------------
//
// Parse an operand as the next thing on line
// return it in operand
// NOTE: if what was located does not look like an operand return false
//
static bool ParseOperand(const char *line,unsigned int *lineIndex,OPERAND *operand)
{
	char
		string[MAX_STRING];
	unsigned int
		inputIndex;
	int
		value,dppn;
	bool
		unresolved;
	REGISTER
		reg;
	unsigned int
		stringIndex;

	operand->dpp=-1;

	if(ParseBracketedString(line,lineIndex,string))
	{
		stringIndex = 0;
		if(string[stringIndex]=='-')
		{
			stringIndex++;
			if(ParseRegister(string,&stringIndex,&reg))
			{
				operand->type = OT_DEC_RW_INDIRECT;
				operand->value=reg.type;
				return(ParseEOS(string,&stringIndex));	// fail if anything else in there
			}
			return false;
		}


		stringIndex = 0;
		if(ParseRegister(string,&stringIndex,&reg))
		{
			operand->type = OT_RW_INDIRECT;
			operand->value=reg.type;

			if(string[stringIndex]=='+')
			{
				operand->type = OT_RW_INC_INDIRECT;
				stringIndex++;
				if(string[stringIndex]=='#')
				{
					stringIndex++;
					if(ParsePrefixedExpression(string,&stringIndex,&value,&dppn,&unresolved))
					{
						if(value&0xFFFF0000) return false;
						operand->type=OT_RW_DATA16_INDIRECT;
						operand->offset=value;
						operand->unresolved=unresolved;
						operand->dpp=dppn;
					}
				}
			}
			return(ParseEOS(string,&stringIndex));	// fail if anything else in there
		}
	}
	else
	{
		inputIndex=*lineIndex;
		if(line[inputIndex]=='/')	// not (as in NOT bit value)
		{
			inputIndex++;
			if(ParseExpression(line,&inputIndex,&value,&unresolved))
			{
				operand->type=OT_NOTVALUE;
				operand->value=value;
				operand->unresolved=unresolved;
				*lineIndex=inputIndex;
				return(true);
			}
		}
		else
		if(line[inputIndex]=='#')		// immediate?
		{
			inputIndex++;

			if(ParseParentheticString(line,&inputIndex,string))
			{
				stringIndex = 0;
				if(ParsePrefixedExpression(string,&stringIndex,&value,&dppn,&unresolved))
				{
					*lineIndex=inputIndex;
					operand->type=OT_DATA32;	
					operand->value=value;
					operand->unresolved=unresolved;
					operand->dpp=dppn;
					if(value&0xFFFF0000) return(true);
					if(value&0xFFFFFF00)
					{
						operand->type=OT_DATA16;
						return(true);
					}
					if(value&0xFFFFFFF0)
					{
						operand->type=OT_DATA8;
						return(true);
					}
					if(value&0xFFFFFFF8)
					{
						operand->type=OT_DATA4;
						return(true);
					}
					if(value&0xFFFFFFFC)
					{
						operand->type=OT_DATA3;
						return(true);
					}
					operand->type=OT_DATA2;	
						return(true);
				}
			}
			else
			if(ParseRegister(line,&inputIndex,&reg))			// does it look like a register?
			{
				if(reg.type==R_SEG)
				{
					inputIndex++;
					if(ParseExpression(line,&inputIndex,&value,&unresolved))
					{
						if(value&0xff000000) return false;
						operand->type=OT_DATA8;
						operand->value=((value>>16)&0xff);
						if(operand->value<16) operand->type=OT_DATA4;
						operand->unresolved=unresolved;
						*lineIndex=inputIndex;
						return(true);
					}
				} else
				if(reg.type==R_SOF)
				{
					inputIndex++;
					if(ParseExpression(line,&inputIndex,&value,&unresolved))
					{
						if(value&0xff000000) return false;
						operand->type=OT_DATA16;
						operand->value=(value&0xffff);
						operand->unresolved=unresolved;
						*lineIndex=inputIndex;
						return(true);
					}
				} else
				if(reg.type==R_PAG)
				{
					inputIndex++;
					if(ParseExpression(line,&inputIndex,&value,&unresolved))
					{
						if(value&0xff000000) return false;
						operand->type=OT_DATA16;
						operand->value=((value>>14)&0x3fff);
						if(operand->value<16) operand->type=OT_DATA4;
						operand->unresolved=unresolved;
						*lineIndex=inputIndex;
						return(true);
					}
				} else
				if(reg.type==R_POF)
				{
					inputIndex++;
					if(ParseExpression(line,&inputIndex,&value,&unresolved))
					{
						if(value&0xff000000) return false;
						operand->type=OT_DATA16;
						operand->value=(value&0x3fff);
						operand->unresolved=unresolved;
						*lineIndex=inputIndex;
						return(true);
					}
				} else
				{
					return false;
				}
			}
			else
			if(ParsePrefixedExpression(line,&inputIndex,&value,&dppn,&unresolved))
			{
				operand->type=OT_DATA32;
				operand->value=value;
				operand->dpp=dppn;
				operand->unresolved=unresolved;
				*lineIndex=inputIndex;
				if(value&0xFFFF0000) return(true);
				if(value&0xFFFFFF00)
				{
					operand->type=OT_DATA16;
					return(true);
				}
				if(value&0xFFFFFFF0)
				{
					operand->type=OT_DATA8;
					return(true);
				}
				if(value&0xFFFFFFF8)
				{
					operand->type=OT_DATA4;
					return(true);
				}
				if(value&0xFFFFFFFC)
				{
					operand->type=OT_DATA3;
					return(true);
				}
				operand->type=OT_DATA2;	
				return(true);
			}
		}
		else
		if(ParseRegister(line,&inputIndex,&reg))			// does it look like a register?
		{
//// Look for SEG and SOF here !

			SkipWhiteSpace(line,&inputIndex);
			if(line[inputIndex]=='.')
			{
				operand->value=reg.type;
				inputIndex++;
				if(ParseExpression(line,&inputIndex,&value,&unresolved))	// see if it looks like reg.value
				{
					if(!CheckBitRange(value,true,true)) return(false);
					operand->type=OT_BITADDR_RW_DIRECT;
					operand->bit=value;
					*lineIndex=inputIndex;
					return(true);
				}
			} else
			{
				if( (operand->value=reg.type)>=R_R15 ) operand->type=OT_RW_DIRECT;
				else operand->type=OT_RB_DIRECT;
				operand->value=reg.type;
				*lineIndex=inputIndex;
				return(true);
			}
		}
		else if(ParseFlag(line,&inputIndex,&reg))					// see if it looks like a flag
		{
			operand->type=OT_FLAG;
			operand->value=reg.type;
			*lineIndex=inputIndex;
			return(true);
		}
		else
		if(ParsePrefixedExpression(line,&inputIndex,&value,&dppn,&unresolved))	// see if it looks like a value
		{
			operand->type=OT_VALUE;
			operand->value=value;
			operand->unresolved=unresolved;
			operand->dpp=dppn;
			SkipWhiteSpace(line,&inputIndex);
			*lineIndex=inputIndex;
			if(line[inputIndex]=='.')
			{
				inputIndex++;
				if(ParseExpression(line,&inputIndex,&value,&unresolved))	// see if it looks like value.value
				{
					if(!CheckBitRange(value,true,true)) return(false);
					operand->type=OT_BITADDR_DIRECT;
					operand->bit=value;
					*lineIndex=inputIndex;
					return(true);
				}
			}
			return(true);
		}
	}
	return(false);
}


//-------------------------------------------------------------------
//
// opcode handling
//
//-------------------------------------------------------------------
// THIS IS A COPY of HandleDW() in support.c modified to call ParsePrefixedExpression
//
static bool HandlePDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool bigEndian)
// Declare words of data in the given endian
{
	int
		dpp,
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
				fail=!GenerateWord(outputString[i],listingRecord,bigEndian);
				i++;
			}
		}
		else if(ParsePrefixedExpression(line,lineIndex,&value,&dpp,&unresolved))
		{
			if( dpp!=-1)
			{
				value=(value&0x3fff)|(dpp<<14);
				fail=!GenerateWord(value,listingRecord,false);
			}
			else
			{
				fail=!GenerateWord(value,listingRecord,false);
			}
		}
		else
		{
			ReportBadOperands();
			done=true;
		}
		if(!done&&!fail)				// look for separator, or comment
		{
			if(ParseComment(line,lineIndex))
			{
				done=true;
			}
			else if(ParseCommaSeparator(line,lineIndex))
			{
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
// THIS IS A COPY of HandleLEDW() in support.c modified to call HandlePDW()
//
bool HandlePLEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring words of data in little endian
{
	return(HandlePDW(opcodeName,line,lineIndex,lineLabel,listingRecord,false));
}


// THIS IS A COPY OF HandleEQU() in pseudo.c
//
static bool HandleDefr(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// EQU statement, set label
// If there is a hard error, return false
{
        int
                value;
        bool
                unresolved;
        bool
                fail;

        fail=false;
        if(contextStack->active)
        {
                if(lineLabel)
                {
                        if(!lineLabel->isLocal)
                        {
                                if(ParseExpression(line,lineIndex,&value,&unresolved))
                                {
                                        if(ParseComment(line,lineIndex))                                        // make sure there's nothing else on the line
                                        {
                                                if(!intermediatePass)
                                                {
                                                        CreateListStringValue(listingRecord,value,unresolved);
                                                }
                                                fail=!AssignConstant(lineLabel->name,value,!unresolved);        // assign it (even if it is unresolved, since this way we can remember that there was an attempt to equate it)
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
                        else
                        {
                                AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
                        }
                }
                else
                {
                        AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
                }
        }
        return(!fail);
}




static bool HandleExtern(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Ignore this as we rely on 'INCLUDE' for linkage. Keep it for documentation.
// This is also used for other ignored opcodes that can have a label that we will ignore too !
// This never fails :-)
{
	return(true);
}

static bool HandleSegmented(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Set the segmented flag and 'assume nothing'
// 
// If there is a problem, report it and return false
{
	bool
		fail;

	fail=false;
	if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}
		segmented=true;
		dpp0=dpp1=dpp2=dpp3=-1;
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}
static bool HandleNonSegmented(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Reset the segmented flag and 'assume 0 1 2 3'
// 
// If there is a problem, report it and return false
{
	bool
		fail;

	fail=false;
	if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
	{
		if(lineLabel)
		{
			ReportDisallowedLabel(lineLabel);
		}
		segmented=false;
		dpp0=0;
		dpp1=1;
		dpp2=2;
		dpp3=3;
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleAssume(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Set up the DPPn registers
//
// If there is a problem, report it and return false
{

	const TOKEN *match;
	int
		value,
		dppn;
	bool
		fail;
	bool
		unresolved;


	fail=false;

	if(segmented)
	{
		if(!lineLabel)
		{
			SkipWhiteSpace(line,lineIndex);
			if((match=MatchBuriedToken(line,lineIndex,nothing)))
			{
				dpp0=dpp1=dpp2=dpp3=-1;
			}
			else
			if(ParsePrefixedExpression(line,lineIndex,&value,&dppn,&unresolved))
			{
				if(ParseComment(line,lineIndex))			// make sure there's nothing else on the line
				{
					if(dppn==-1)
					{
						AssemblyComplaint(NULL,true,"NOTHING or DPPn: prefix expected\n");
					}
					else
					{
						value=value>>14;
						CreateListStringValue(listingRecord,value,unresolved);
						switch(dppn)
						{
							case 0:	dpp0=value;
								break;
							case 1:	dpp1=value;
								break;
							case 2:	dpp2=value;
								break;
							case 3:	dpp3=value;
								break;
							default:
								fail=true;
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
		else
		{
			ReportDisallowedLabel(lineLabel);
		}
		
	}
	else
	{
		AssemblyComplaint(NULL,true,"ASSUME is not allowed in non-segmented mode\n");
	}
	return(!fail);
}

static bool HandleEven(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Align the PC to a a word address by adding one byte of zero
// If there is a problem, report it and return false
{
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(ParseComment(line,lineIndex))	// make sure there's nothing else on the line
		{
			if(lineLabel)
			{
				ReportDisallowedLabel(lineLabel);
			}

			if((currentSegment->currentPC)&1)
			{
				fail = !GenerateByte(0,listingRecord);
			}
		}
		else
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}


//// THIS IS A COPY OF THE 'ALIAS' CODE IN pseudo.c
static bool HandleLIT(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// LIT statement, set definition
// If the textstring is to be quoted it has to be with single quotes!
// If there is a hard error, return false
{
	ALIAS_RECORD
		*aliasMatch;
	char
		aliasValue[MAX_STRING];
	unsigned int
		stringLength;
	bool
		fail;

	fail=false;
	if(contextStack->active)
	{
		if(lineLabel)
		{
			if(!lineLabel->isLocal)
			{
				if(ParseFirstNameElement(line,lineIndex,aliasValue)||ParseQuotedString(line,lineIndex,'\'','\'',aliasValue,&stringLength))	// think of operands as a list of one element, or as a quoted string
				{
					if(ParseComment(line,lineIndex))				// must be at the end of the line now
					{
						if(!(aliasMatch=MatchAlias(lineLabel->name)))		// make sure it does not already exist
						{
							if(!CreateAlias(lineLabel->name,aliasValue))	// create the alias, complain if there's trouble
							{
								ReportComplaint(true,"Failed to create LIT record\n");
								fail=true;
							}
						}
						else
						{
							AssemblyComplaint(NULL,true,"LIT '%s' already defined\n",lineLabel);
							AssemblySupplement(&aliasMatch->whereFrom,"Conflicting LIT was defined here\n");
						}
					}
					else
					{
						ReportBadOperands();
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"Bad LIT replacement declaration\n");
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"'%s' requires a non-local label\n",opcodeName);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"'%s' requires a label\n",opcodeName);
		}
	}
	return(!fail);
}



bool IsData2(OPERAND *operand)
{
	return( operand->type == OT_DATA2);
}
bool IsData3(OPERAND *operand)
{
	if( IsData2(operand) ) return(true);
	return( operand->type == OT_DATA3);
}
bool IsData4(OPERAND *operand)
{
	if( IsData3(operand) ) return(true);
	return( operand->type == OT_DATA4);
}
bool IsData8(OPERAND *operand)
{
	if( IsData4(operand) ) return(true);
	return( operand->type == OT_DATA8);
}
bool IsData16(OPERAND *operand)
{
	if( IsData8(operand) ) return(true);
	return( operand->type == OT_DATA16);
}

bool IsCaddr(OPERAND *operand)
{
	if( !IsData16(operand)&&(operand->type!=OT_VALUE) ) return(false);
	return( !(operand->value & 1) );
}


//
// Test if 'value' is a valid SFR/ESFR address
//
bool IsRegValue(int value)
{
//	if(value<0x100) return(true);
	if( (value>=0xfe00)&&(value<=0xffff) ) return(true);
	if( (value>=0xf000)&&(value<=0xf1ff) ) return(true);
	return(false);
}

//
// Make a word address offset if in the SFR/ESFR address ranges
//
int RegValue(int value)
{
	if( (value>=0xfe00)&&(value<=0xffff) )
	{
		return((value-0xfe00)>>1);
	}
	else
	if( (value>=0xf000)&&(value<=0xf1ff) )
	{
		return((value-0xf000)>>1);
	} else return(value);
}


//
// Test if 'value' is a valid SFR/ESFR or RAM address for bit operations
//
bool IsBitoffValue(int value)
{
	if(value<0x100) return(true);
	if( (value>=0xff00)&&(value<=0xffde) ) return(true);
	if( (value>=0xf100)&&(value<=0xf1de) ) return(true);
	if( (value>=0xfd00)&&(value<=0xfdfe) ) return(true);
	return(false);
}

//
// Test if a memory address conforms to the selected page mode
//
bool IsValidMem(OPERAND *operand)
{
	unsigned int
		temp;


	if(operand->type!=OT_VALUE) return(false);		// not a memory address at all
	temp=operand->value;
	if(!segmented&&(temp<0x10000)) return(true);		// within the first 64k

	if(!segmented) return(false);				// outside the first 64k
	if(operand->dpp!=-1) return(true);			// prefix used

	temp=temp>>14;
	if(temp==dpp0)
	{
		return(true);
	}
	else
	if(temp==dpp1)
	{
		return(true);
	}
	else
	if(temp==dpp2)
	{
		return(true);
	}
	else
	if(temp==dpp3)
	{
		return(true);
	}
	return(false);						// no matching dpp	
}



//
// Make a word address offset if in the SFR/ESFR or RAM address ranges for bit operations
//
int BitoffValue(int value)
{
	if( (value>=0xff00)&&(value<=0xffde) )
	{
		return((value-0xfe00)>>1);
	}
	else
	if( (value>=0xf100)&&(value<=0xf1de) )
	{
		return((value-0xfe00)>>1);
	}
	else
	if( (value>=0xfd00)&&(value<=0xfdfee) )
	{
		return((value-0xfd00)>>1);
	} else return(value);
}

//-------------------------------------------------------------------
//
// Given an addressing mode record, and a set of operands, generate code (or an error message if something is
// out of range)
// This will only return false on a 'hard' error
//
static bool HandleAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *destOperand,OPERAND *srcOperand,OPERAND *thrdOperand,LISTING_RECORD *listingRecord)
{
	bool
		fail;
	int
		value;

	fail=false;
	switch(addressingMode->mode)
	{
		case AM_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value = (destOperand->value)/2 - (currentSegment->currentPC)/2+currentSegment->codeGenOffset - 1;
				Check8RelativeRange(value,true,true);
			}
			fail = !GenerateOpcode((addressingMode->baseOpcode),addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(value,listingRecord);
			}
			break;

		case AM_NOFLAG_ABSOLUTE:
			value=0;
			fail = !GenerateOpcode((addressingMode->baseOpcode),1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(0,listingRecord);
			}
			if(!fail)
			{
				fail = !GenerateWord((destOperand->value),listingRecord,false);
			}
			break;
		case AM_FLAG_ABSOLUTE:
			value=0;
			fail = !GenerateOpcode((addressingMode->baseOpcode),1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte((destOperand->value)<<4,listingRecord);
			}
			if(!fail)
			{
				fail = !GenerateWord((srcOperand->value),listingRecord,false);
			}
			break;
		case AM_NOFLAG_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value = ( destOperand->value/2 - currentSegment->currentPC/2+currentSegment->codeGenOffset ) - 1;
				Check8RelativeRange(value,true,true);
			}
			fail = !GenerateByte(addressingMode->baseOpcode,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(value,listingRecord);
			}
			break;
		case AM_FLAG_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value = ( srcOperand->value/2 - currentSegment->currentPC/2+currentSegment->codeGenOffset ) - 1;
				Check8RelativeRange(value,true,true);
			}
			fail = !GenerateByte((addressingMode->baseOpcode)|((destOperand->value)<<4),listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(value,listingRecord);
			}
			break;
		case AM_NOFLAG_RWI:
			value=0;
			fail = !GenerateOpcode((addressingMode->baseOpcode),1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte((rcode[destOperand->value]&0x0f),listingRecord);
			}
			break;
		case AM_FLAG_RWI:
			value=0;
			fail = !GenerateOpcode((addressingMode->baseOpcode),1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( ( ((destOperand->value)<<4) | (rcode[srcOperand->value]&0x0f) ),listingRecord );
			}
			break;


		case AM_IMP:
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_BITADDR_REL:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=destOperand->value;
				if( destOperand->type== OT_BITADDR_RW_DIRECT ) 
				{
					value=rcode[value] | 0xF0;
				} else 
				{
					value=BitoffValue(value);
				}
				fail = !GenerateByte( value, listingRecord );
			}
			if(!fail)
			{
				value=0;
				if(currentSegment)
				{
					value = ( srcOperand->value/2 - currentSegment->currentPC/2+currentSegment->codeGenOffset ) - 1;
					Check8RelativeRange(value,true,true);
				}
				fail = !GenerateByte(value,listingRecord);
			}
			if(!fail)
			{
				fail = !GenerateByte( (destOperand->bit)<<4, listingRecord );
			}
			break;

		case AM_BITADDR_DIRECT:
			value = addressingMode->baseOpcode | destOperand->bit<<4;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=destOperand->value;
				if( destOperand->type== OT_BITADDR_RW_DIRECT )
				{
					value=rcode[value] | 0xF0;
				} else
				{
					value=BitoffValue(value);
				}
				fail = !GenerateByte( value, listingRecord );
			}
			break;

		case AM_BITADDR_BITADDR:
			fail = !GenerateOpcode(addressingMode->baseOpcode,1,listingRecord);
			if(!fail)
			{
				value=srcOperand->value;
				if( srcOperand->type== OT_BITADDR_RW_DIRECT )
				{
					value=rcode[value] | 0xF0;
				} else
				{
					value=BitoffValue(value);
				}
				fail = !GenerateByte( value,listingRecord );
			}
			if(!fail)
			{
				value=destOperand->value;
				if( destOperand->type== OT_BITADDR_RW_DIRECT )
				{
					value=rcode[value] | 0xF0;
				} else
				{
					value=BitoffValue(value);
				}	
				fail = !GenerateByte( value,listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( destOperand->bit|srcOperand->bit<<4,listingRecord );
			}
			break;

		case AM_IRANG2:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value= ( (destOperand->value -1)<<4 )|(addressingMode->trailOpcode);
				fail = !GenerateByte( value,listingRecord );
			}
			break;

		case AM_TRAP7:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value= destOperand->value;
				fail = !GenerateByte( value,listingRecord );
			}
			break;
		case AM_PAG_IRANG2:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value= ( (srcOperand->value -1)<<4 )|addressingMode->trailOpcode;
				fail = !GenerateOpcode( value,1, listingRecord );
			}
			if(!fail)
			{
				fail=!GenerateWord( destOperand->value,listingRecord,false);
			}
			break;
		case AM_SEG_IRANG2:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value= ( (srcOperand->value -1)<<4 )|addressingMode->trailOpcode;
				fail = !GenerateByte( value, listingRecord );
			}
			if(!fail)
			{
				fail=!GenerateByte( destOperand->value,listingRecord);
			}
			if(!fail)
			{
				fail=!GenerateOpcode( 0 ,1,listingRecord);
			}
			break;
		case AM_RW_IRANG2:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( (((srcOperand->value)-1)<<4)|(rcode[destOperand->value])|(addressingMode->trailOpcode), listingRecord );
			}
			break;
		case AM_RW_DATA3:
		case AM_RB_DATA3:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)|(rcode[destOperand->value]<<4), listingRecord );
			}
			break;
		case AM_RW_DATA4:
		case AM_RB_DATA4:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( ((srcOperand->value)<<4)|rcode[destOperand->value], listingRecord );
			}
			break;

		case AM_RW_DATA16:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( 0xF0|rcode[destOperand->value],1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateWord( (srcOperand->value), listingRecord,false );
			}
			break;
		case AM_RB_DATA8:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( 0xF0|rcode[destOperand->value], listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value), listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( 0 , listingRecord );
			}
			break;
		case AM_REG_DATA16:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( RegValue(destOperand->value), listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateWord( (srcOperand->value), listingRecord ,false);
			}	
			break;
		case AM_REG_DATA8:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( RegValue(destOperand->value), listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)&0xFF,listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( 0 , listingRecord );
			}
			break;

		case AM_RW_DATA8_DATA8_L:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=destOperand->value;
				if( destOperand->type== OT_RW_DIRECT ) value=rcode[value] | 0xF0;
				fail = !GenerateByte( value,listingRecord );
			}

			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)&0xFF, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (thrdOperand->value )&0xFF, listingRecord );
			}

			break;
		case AM_REG_DATA8_DATA8_L:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=BitoffValue(destOperand->value);
				fail = !GenerateByte( value, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)&0xFF, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (thrdOperand->value )&0xFF, listingRecord );
			}
			break;
		case AM_RW_DATA8_DATA8_H:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=destOperand->value;
				if( destOperand->type== OT_RW_DIRECT ) value=rcode[value] | 0xF0;
				fail = !GenerateByte( value, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (thrdOperand->value )&0xFF,listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)&0xFF, listingRecord );
			}
			break;
		case AM_REG_DATA8_DATA8_H:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				value=BitoffValue(destOperand->value);
				fail = !GenerateByte( value,listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (thrdOperand->value )&0xFF, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->value)&0xFF, listingRecord );
			}
			break;

		case AM_REG:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				if(destOperand->type==OT_RW_DIRECT)
				{
					value=rcode[(destOperand->value)]|0xF0;
				}
				else
				if(destOperand->type==OT_VALUE)
				{
					value=RegValue(destOperand->value);
				}
				if(!fail)
				{
					fail = !GenerateByte( value,listingRecord );
				}
			}
			break;

		case AM_RW:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( rcode[(destOperand->value)]<<4,listingRecord );
			}
			break;
		case AM_RB:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[destOperand->value])<<4, listingRecord );
			}
			break;

		case AM_RW_RWI2:
		case AM_RB_RWI2:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[destOperand->value]<<4)|rcode[srcOperand->value]|8, listingRecord );
			}
			break;

		case AM_RW_RWI2_INC:
		case AM_RB_RWI2_INC:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[destOperand->value]<<4)|rcode[srcOperand->value]|0xC, listingRecord );
			}
			break;

		case AM_RW_RW:
		case AM_RB_RB:
		case AM_RB_RWI:
		case AM_RB_RWI_INC:
		case AM_RW_RWI:
		case AM_RW_RWI_INC:
		case AM_RWI_RWI:
		case AM_RWI_INC_RWI:
		case AM_RWI_RWI_INC:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[destOperand->value]<<4)|rcode[srcOperand->value],listingRecord );
			}
			break;

		case AM_RW_RWI16:
		case AM_RB_RWI16:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( (rcode[destOperand->value]<<4)|rcode[srcOperand->value], listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (srcOperand->offset)&0xff, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( ((srcOperand->offset)>>8)&0xff, listingRecord );
			}
			break;


		case AM_RWI_RW:
		case AM_DEC_RWI_RW:
		case AM_RWI_RB:
		case AM_DEC_RWI_RB:
		case AM_RW_RB:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[srcOperand->value]<<4)|rcode[destOperand->value], listingRecord );
			}
			break;

		case AM_RWI16_RW:
		case AM_RWI16_RB:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				 fail = !GenerateByte( (rcode[srcOperand->value]<<4)|rcode[destOperand->value], listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( (destOperand->offset)&0xff, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateByte( ((destOperand->offset)>>8)&0xff,listingRecord );
			}
			break;

		case AM_REG_CADDR:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( RegValue(destOperand->value),1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateWord( (srcOperand->value), listingRecord,false );
			}
			break;
		case AM_SEG_CADDR:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( destOperand->value,1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateWord( (srcOperand->value), listingRecord ,false);
			}
			break;

		case AM_RW_CADDR:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
				fail = !GenerateOpcode( rcode[destOperand->value]|0xF0,1, listingRecord );
			if(!fail)
			{
				fail = !GenerateWord( (srcOperand->value), listingRecord, false );
			}
			break;
		case AM_RB_MEM:
		case AM_RW_MEM:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( rcode[destOperand->value]|0xF0, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( srcOperand,srcOperand->value,listingRecord );
			}
			break;
		case AM_REG_MEM:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( RegValue(destOperand->value), listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( srcOperand,srcOperand->value,listingRecord );
			}
			break;
		case AM_MEM_RW:
		case AM_MEM_RB:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( rcode[srcOperand->value]|0xF0,1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( destOperand,destOperand->value,listingRecord );
			}
			break;
		case AM_MEM_REG:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte( RegValue(srcOperand->value), listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( destOperand,destOperand->value,listingRecord );
			}
			break;
		case AM_MEM_RWI:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( rcode[srcOperand->value],1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( destOperand,destOperand->value,listingRecord );
			}
			break;
		case AM_RWI_MEM:
			value = addressingMode->baseOpcode;
			fail = !GenerateOpcode(value,1,listingRecord);
			if(!fail)
			{
				fail = !GenerateOpcode( rcode[destOperand->value],1, listingRecord );
			}
			if(!fail)
			{
				fail = !GenerateDPPAddress( srcOperand,srcOperand->value,listingRecord );
			}
			break;
	}
	return(!fail);
}

//-------------------------------------------------------------------
//
// See if the given addressing mode matches the passed operands
// return true for a match, false if no match
//
static bool OperandsMatchAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *destOperand,OPERAND *srcOperand,OPERAND *thrdOperand)
{
	switch(addressingMode->mode)
	{
 		case AM_IMP:
			return(numOperands==0);
			break;
 		case AM_RELATIVE:
 			return( (numOperands==1) && (destOperand->type==OT_VALUE) );
			break;
		case AM_FLAG_RWI:
 			return( (numOperands==2) && (destOperand->type==OT_FLAG) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_NOFLAG_RWI:
 			return( (numOperands==1) && (destOperand->type==OT_RW_INDIRECT) );
			break;
  		case AM_NOFLAG_RELATIVE:
		case AM_NOFLAG_ABSOLUTE:
 			return( (numOperands==1) && destOperand->type==OT_VALUE );
			break;
		case AM_FLAG_RELATIVE:
 			return( (numOperands==2) && (destOperand->type==OT_FLAG) && (srcOperand->type==OT_VALUE) );
			break;
		case AM_FLAG_ABSOLUTE:
 			return( (numOperands==2) && (destOperand->type==OT_FLAG) && IsCaddr(srcOperand) );
			break;
		case AM_RB_DATA8:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && IsData8(srcOperand) );
			break;
		case AM_RW_DATA16:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && IsData16(srcOperand) );
			break;
		case AM_REG_DATA16:
			if( !IsRegValue(destOperand->value) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && IsData16(srcOperand) );
			break;
		case AM_REG_DATA8:
			if( !IsRegValue(destOperand->value) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && IsData8(srcOperand) );
			break;
		case AM_RW_DATA3:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && IsData3(srcOperand) );
			break;
		case AM_RB_DATA3:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && IsData3(srcOperand) );
			break;
		case AM_RW_DATA4:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && IsData4(srcOperand) );
			break;
		case AM_RW:
			return ( (numOperands==1) && (destOperand->type==OT_RW_DIRECT) );
			break;
		case AM_REG:
//			if( !IsRegValue(destOperand->value) ) return(false);
			if ( (numOperands==1) && (destOperand->type==OT_RW_DIRECT) ) return true;
			if( !IsRegValue(destOperand->value) ) return(false);
			return ( (numOperands==1) && (destOperand->type==OT_VALUE) );
			break;
		case AM_RB:
			return( (numOperands==1) && (destOperand->type==OT_RB_DIRECT) );
			break;
		case AM_RB_DATA4:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && IsData4(srcOperand) );
			break;
		case AM_RWI_RWI:
			return( (numOperands==2) && (destOperand->type==OT_RW_INDIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RW_RW:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_DIRECT) );
			break;
		case AM_RB_RB:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_RW_RB:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_RB_RWI:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RW_RWI2:
			if( (srcOperand->type==OT_RW_INDIRECT) && (rcode[srcOperand->value]>3) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RB_RWI2:
			if( (srcOperand->type==OT_RW_INDIRECT) && (rcode[srcOperand->value]>3) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RW_RWI:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RW_RWI_INC:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_INC_INDIRECT) );
			break;
		case AM_RB_RWI_INC:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RW_INC_INDIRECT) );
			break;
		case AM_RW_RWI2_INC:
			if( (srcOperand->type==OT_RW_INC_INDIRECT) && (rcode[srcOperand->value]>3) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_INC_INDIRECT) );
			break;
		case AM_RB_RWI2_INC:
			if( (srcOperand->type==OT_RW_INC_INDIRECT) && (rcode[srcOperand->value]>3) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RW_INC_INDIRECT) );
			break;
		case AM_RWI_RWI_INC:
			return( (numOperands==2) && (destOperand->type==OT_RW_INDIRECT) && (srcOperand->type==OT_RW_INC_INDIRECT) );
			break;
		case AM_RWI_INC_RWI:
			return( (numOperands==2) && (destOperand->type==OT_RW_INC_INDIRECT) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_RWI_RW:
			return( (numOperands==2) && (destOperand->type==OT_RW_INDIRECT) && (srcOperand->type==OT_RW_DIRECT) );
			break;
		case AM_DEC_RWI_RW:
			return( (numOperands==2) && (destOperand->type==OT_DEC_RW_INDIRECT) && (srcOperand->type==OT_RW_DIRECT) );
			break;
		case AM_RWI16_RW:
			return( (numOperands==2) && (destOperand->type==OT_RW_DATA16_INDIRECT) && (srcOperand->type==OT_RW_DIRECT) );
			break;
		case AM_RWI16_RB:
			return( (numOperands==2) && (destOperand->type==OT_RW_DATA16_INDIRECT) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_RW_RWI16:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_RW_DATA16_INDIRECT) );
			break;
		case AM_RB_RWI16:
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_RW_DATA16_INDIRECT) );
			break;
		case AM_RWI_RB:
			return( (numOperands==2) && (destOperand->type==OT_RW_INDIRECT) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_DEC_RWI_RB:
			return( (numOperands==2) && (destOperand->type==OT_DEC_RW_INDIRECT) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_REG_CADDR:
			if( !IsRegValue(destOperand->value) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && IsCaddr(srcOperand) );
			break;
		case AM_RW_CADDR:
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && IsCaddr(srcOperand) );
			break;
		case AM_SEG_CADDR:
			return( (numOperands==2) && IsData8(destOperand) && IsCaddr(srcOperand) );
			break;
		case AM_BITADDR_REL:
			if( !IsBitoffValue(destOperand->value ) ) return(false);
			if( (numOperands==2) && (destOperand->type==OT_BITADDR_RW_DIRECT) && (srcOperand->type==OT_VALUE )  ) return true;
			return( (numOperands==2) && (destOperand->type==OT_BITADDR_DIRECT) && (srcOperand->type==OT_VALUE ) );
			break;
		case AM_REG_DATA8_DATA8_H:
		case AM_REG_DATA8_DATA8_L:
			if( !IsBitoffValue(destOperand->value) ) return(false);
			if( (numOperands==3) && (destOperand->type==OT_VALUE) && IsData8(srcOperand) && IsData8(thrdOperand)  ) return(true);
			return( (numOperands==3) && (destOperand->type==OT_RW_DIRECT) && IsData8(srcOperand) && IsData8(thrdOperand) );
			break;
		case AM_BITADDR_DIRECT:
			if( (numOperands==1) && (destOperand->type==OT_BITADDR_RW_DIRECT) ) return true;
			if(!IsBitoffValue(destOperand->value)) return(false);
			return( (numOperands==1) && (destOperand->type==OT_BITADDR_DIRECT) );
			break;
		case AM_BITADDR_BITADDR:
			if( !IsBitoffValue(destOperand->value) ) return(false);
			if( !IsBitoffValue(srcOperand->value) ) return(false);
			if( (numOperands==2) && (destOperand->type==OT_BITADDR_RW_DIRECT) && (srcOperand->type==OT_BITADDR_RW_DIRECT) ) return(true);
			if( (numOperands==2) && (destOperand->type==OT_BITADDR_DIRECT) && (srcOperand->type==OT_BITADDR_RW_DIRECT) ) return(true);
			if( (numOperands==2) && (destOperand->type==OT_BITADDR_RW_DIRECT) && (srcOperand->type==OT_BITADDR_DIRECT) ) return(true);
			return( (numOperands==2) && (destOperand->type==OT_BITADDR_DIRECT)&&(srcOperand->type==OT_BITADDR_DIRECT) );
			break;
		case AM_RB_MEM:
			if( !IsValidMem(srcOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RB_DIRECT) && (srcOperand->type==OT_VALUE) );
			break;
		case AM_RW_MEM:
			if( !IsValidMem(srcOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && (srcOperand->type==OT_VALUE) );
			break;
		case AM_RWI_MEM:
			if( !IsValidMem(srcOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_REG_MEM:
			if( !IsValidMem(srcOperand) ) return(false);
			if( !IsRegValue(destOperand->value) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_VALUE) );
			break;
		case AM_MEM_RW:
			if( !IsValidMem(destOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_RW_DIRECT) );
			break;
		case AM_MEM_RWI:
			if( !IsValidMem(destOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_RW_INDIRECT) );
			break;
		case AM_MEM_RB:
			if( !IsValidMem(destOperand) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_RB_DIRECT) );
			break;
		case AM_MEM_REG:
			if( !IsValidMem(destOperand) ) return(false);
			if( !IsRegValue(srcOperand->value) ) return(false);
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_VALUE) );
			break;
		case AM_TRAP7:
			if(destOperand->value>0x7f) return false;
			return( (numOperands==1) && IsData8(destOperand) );
			break;
		case AM_IRANG2:
			if((destOperand->value==0)||(destOperand->value>4)) return false;
			return( (numOperands==1) && IsData3(destOperand) );
			break;
		case AM_RW_IRANG2:
			if((srcOperand->value==0)||(srcOperand->value>4)) return false;
			return( (numOperands==2) && (destOperand->type==OT_RW_DIRECT) && IsData3(srcOperand) );
			break;
		case AM_PAG_IRANG2:
			if(destOperand->value>1023) return false;
			if((srcOperand->value==0)||(srcOperand->value>4)) return false;
			return( (numOperands==2) && IsData16(destOperand) && IsData3(srcOperand) );
			break;
		case AM_SEG_IRANG2:
			if((srcOperand->value==0)||(srcOperand->value>4)) return false;
			return( (numOperands==2) && IsData8(destOperand) && IsData3(srcOperand) );
			break;
		default:
			break;
	}
	return(false);
}

static bool ParseOperands(const char *line,unsigned int *lineIndex,unsigned int *numOperands,OPERAND *destOperand,OPERAND *srcOperand,OPERAND *thrdOperand)
// parse from 0 to 3 operands from the line
// return the operands parsed
// numOperands indicates the number of operands successfully parsed
// If something does not look right, return false
{
	bool
		fail;

	fail=false;
	*numOperands=0;
	if(!ParseComment(line,lineIndex))	// make sure not at end of line
	{
		if(ParseOperand(line,lineIndex,destOperand))
		{

			*numOperands=1;
			if(!ParseComment(line,lineIndex))	// make sure not at end of line
			{
				if(ParseCommaSeparator(line,lineIndex))
				{
					if(!ParseComment(line,lineIndex))	// make sure not at end of line
					{
						if(ParseOperand(line,lineIndex,srcOperand))
						{
							*numOperands=2;
							if(!ParseComment(line,lineIndex))	// make sure not at end of line
							{
								if(ParseCommaSeparator(line,lineIndex))
								{
									if(!ParseComment(line,lineIndex))	// make sure not at end of line
									{
										if(ParseOperand(line,lineIndex,thrdOperand))
										{

											*numOperands=3;
											if(!ParseComment(line,lineIndex))	// make sure not at end of line
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
		numOperands;
	OPERAND
		destOperand,
		srcOperand,
		thrdOperand;
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
			if(ParseOperands(line,lineIndex,&numOperands,&destOperand,&srcOperand,&thrdOperand))	// fetch operands for opcode
			{
				done=false;
				for(i=0;!done&&(i<opcode->numModes);i++)
				{
					if(OperandsMatchAddressingMode(&(opcode->addressingModes[i]),numOperands,&destOperand,&srcOperand,&thrdOperand))
					{
						result=HandleAddressingMode(&(opcode->addressingModes[i]),numOperands,&destOperand,&srcOperand,&thrdOperand,listingRecord);
						done=true;
					}
				}
				if(!done)
				{
					AssemblyComplaint(NULL,true,"Invalid addressing mode(s)\n");
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Invalid operand %i\n",numOperands+1);
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
// This is called as the first thing on every pass
{
	dpp0=0;
	dpp1=1;
	dpp2=2;
	dpp3=3;
	segmented=false;

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
//	STDisposeSymbolTable(opcodeZ180Symbols);	// may be recycled later
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
					return(true);
				}

#if 0 // Keep this here. It may be needed later if we add more processors
				if(!fail)
				{
					if((opcodeZ180Symbols=STNewSymbolTable(elementsof(AdditionalZ180Opcodes))))
					{
						for(i=0;!fail&&(i<elementsof(AdditionalZ180Opcodes));i++)
						{
							if(!STAddEntryAtEnd(opcodeZ180Symbols,AdditionalZ180Opcodes[i].name,&AdditionalZ180Opcodes[i]))
							{
								fail=true;
							}
						}
						if(!fail)
						{
							return(true);
						}
						STDisposeSymbolTable(opcodeZ180Symbols);
					}
				}
#endif
				STDisposeSymbolTable(opcodeSymbols);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

// processors handled here (the constructors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("INFINEON C166",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"c166",NULL),
//		PROCESSOR(&processorFamily,"z180",&opcodeZ180Symbols),
	};
