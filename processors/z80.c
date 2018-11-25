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
//   send comments and bug reports regarding this module to:
//     apines@cosmodog.com
//
//-------------------------------------------------------------------
//
//	2 January, 2000: first release
//  16 April, 2000: added support for Z180/64180
//  30 September, 2000: corrected bug which permitted forward relative
//    jumps beyond 127
//
//-------------------------------------------------------------------
//
//
//-------------------------------------------------------------------
//
//  Remaining Issues:
//
//    1) the z80 technical manual reveals some inconsistencies,
//      such as add with carry B to A is specified as:
//         ADC  A,B
//      but logical 'and' B with A is specified as:
//         AND  B
//		This is done since the destination for AND is always A.
//      The implementation here adheres strictly to the spec; if a
//      compelling argument can be made for relaxing it I'm willing
//      to make the change, so that, for example, either
//         ADC  B
//      and
//         AND  A,B
//      could be accepted.
//
//    2) need to verify that all errors are properly detected
//
//    3) could reduce some redundancy, especially with IX and IY
//
//-------------------------------------------------------------------
//
//  some useful things to know about the Z80:
//
//  o byte order for 16 bit values is low then high (little endian)
//  o instruction format for two operand instructions is:
//      opcode      dest, src
//  o immediate values are not preceeded by '#', e.g.:
//      ld    b,0xa5
//    copies the value 0xa5 into the b register
//  o extended addresses are in parenthesis, e.g.:
//      ld    b,(0x5000)
//    copies the value at 0x5000 into the b register
//
//  REGISTERS:
//	  PC:  16-bit program counter
//    SP:  16-bit stack pointer
//    IX:  16-bit index register
//    IY:  16-bit index register
//    I:   8-bit interrupt page address register
//	  R:   8-bit memory refresh register
//    A:   main 8-bit accumulator
//    F:   8-bit flags associated with A
//    A':  alternate 8-bit accumulator
//    F':   8-bit flags associated with A'
//    BC:  main 8/16-bit general purpose register pair
//    DE:  main 8/16-bit general purpose register pair
//    HL:  main 8/16-bit general purpose register pair
//    BC': alternate 8/16-bit general purpose register pair
//    DE': alternate 8/16-bit general purpose register pair
//    HL': alternate 8/16-bit general purpose register pair
//
//  FLAG REGISTER BITS:
//    bit  name function
//     7    S    sign flag (1 = result negative)
//     6    Z    zero flag (1 = result zero)
//     5    X    not used
//     4    H    half-carry flag
//     3    X    not used
//     2    P/V  parity/overflow flag
//     1    N    add/subtract flag (used by decimal adjust instruction, DAA)
//     0    C    carry flag
//
//  ADDRESSING MODES:
//    immediate:           n		opcode(s)  operand
//    immediate extended:  nn		opcode(s)  operand(low)  operand(high)
//    modified page zero:  			opcode
//    relative:            d		opcode     operand
//    extended:            (nn)		opcode(s)  operand(low)  operand(high)
//    indexed:             (ix+d)	opcodes    displacement
//    register:            r		opcode(s)
//    implied:             			opcode(s)
//    register indirect:   			opcode(s)
//
//    combinations of addressing modes are allowed, such as:
//      ld (hl),a
//     which copies the contents of accumulator a (register) to
//     the location pointed to by hl (register indirect)
//
//-------------------------------------------------------------------
//
//  an ugly kludge deals with the fact that it's difficult to
//    differentiate between the carry flag, C, and the C register.
//
//  it works but I'm open to suggestions for improvements
//
//-------------------------------------------------------------------
//
//  the Z180 is (mostly) code compatible with the Z80 with a few
//    additional instructions.  BE AWARE that the DAA, RLD, and RRD
//    instructions do not behave identically on the Z180:
//
//  DAA -- if you execute DAA after DEC on 0x00 the Z80 result is
//    0x99 while the Z180 result is 0xf9
//  RLD/RRD -- Z80 flags reflect contents of the accumulator while
//    z180 flags reflect contents of location pointed to by HL
//
//-------------------------------------------------------------------

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols,
	*opcodeZ180Symbols;

static PROCESSOR
	*currentProcessor;


// 8-bit register bit codes (for imbedding in opcodes)

#define RCODE_B		0
#define RCODE_C		1
#define RCODE_D		2
#define RCODE_E		3
#define RCODE_H		4
#define RCODE_L		5
#define RCODE_A		7

// 16-bit register bit codes (for imbedding in opcodes)

#define RCODE_BC	0
#define RCODE_DE	1
#define RCODE_HL	2
#define RCODE_SP	3		// usually 3 codes for SP, but on pop/push it codes for AF
#define RCODE_AF	3

#define RCODE_IX	0
#define RCODE_IY	1

// status flag bit codes

#define RCODE_NZ	0
#define RCODE_Z		1
#define RCODE_NC	2
#define RCODE_CRY	3
#define RCODE_PO	4
#define RCODE_PE	5
#define RCODE_P		6
#define RCODE_M		7

// codes for special registers

#define RCODE_I		0
#define RCODE_R		1

struct REGISTER
{
	unsigned int
		type;		// type of operand located
	int
		value;		// register offset
};

// enumerated register types (used when parsing)

enum
{
	R_A,			//	A
	R_B,			//	B
	R_C,			//	C
	R_D,			//	D
	R_E,			//	E
	R_H,			//	H
	R_L,			//	L
	R_AF,			//  AF
	R_BC,			//	BC
	R_DE,			//	DE
	R_HL,			//	HL
	R_SP,			//	SP
	R_IX,			//	IX
	R_IY,			//	IY
	R_I,			//	I
	R_R,			//	R
	R_AFP,			//  AF'
};


enum
{
	F_NZ,			// non-zero
	F_Z,			// zero
	F_NC,			// no carry
	F_C,			// carry
	F_PO,			// parity odd
	F_PE,			// parity even
	F_P,			// plus
	F_M,			// minus
};


// enumerated operand types (used when parsing)

enum
{
	OT_A,					//	A <-- must be in the same order as R_A through R_R, above
	OT_B,					//	B
	OT_C,					//	C
	OT_D,					//	D
	OT_E,					//	E
	OT_H,					//	H
	OT_L,					//	L
	OT_AF,					//	AF
	OT_BC,					//	BC
	OT_DE,					//	DE
	OT_HL,					//	HL
	OT_SP,					//	SP
	OT_IX,					//	IX
	OT_IY,					//	IY
	OT_I,					//	I
	OT_R,					//	R

	OT_AFP,					//	AF'

	OT_NZ,					// non-zero  <-- again, same order as above
	OT_Z,					// zero
	OT_NC,					// no carry
	OT_CRY,					// carry
	OT_PO,					// parity odd
	OT_PE,					// parity even
	OT_P,					// plus
	OT_M,					// minus

	OT_BC_INDIRECT,			//	(BC)
	OT_DE_INDIRECT,			//	(DE)
	OT_HL_INDIRECT,			//	(HL)
	OT_IX_INDIRECT,			//	(IX+d)
	OT_IY_INDIRECT,			//	(IY+d)
	OT_SP_INDIRECT,			//	(SP)
	OT_C_INDIRECT,			//  (C)

	OT_INDIRECT,			// (xxxx) or (xx)
	
	OT_VALUE,				// xxxx
	OT_NOTVALUE,			// /xxxx
};


static const unsigned short rcode[] =		// indexed by OT_nnnn
{
	RCODE_A,	// registers
	RCODE_B,
	RCODE_C,
	RCODE_D,
	RCODE_E,
	RCODE_H,
	RCODE_L,
	RCODE_AF,
	RCODE_BC,
	RCODE_DE,
	RCODE_HL,
	RCODE_SP,
	RCODE_IX,
	RCODE_IY,
	RCODE_I,
	RCODE_R,

	0,			// no code for AF'

	RCODE_NZ,	// flags
	RCODE_Z,
	RCODE_NC,
	RCODE_CRY,
	RCODE_PO,
	RCODE_PE,
	RCODE_P,
	RCODE_M,

	RCODE_BC,	// indirect codes
	RCODE_DE,
	RCODE_HL,
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
	{R_AFP,	"AF'"	},
	{R_AF,	"AF"	},
	{R_BC,	"BC"	},		// list multi-character names first
	{R_DE,	"DE"	},		//  so, for example, we match BC
	{R_HL,	"HL"	},		//  instead of just B, where appropriate
	{R_IX,	"IX"	},
	{R_IY,	"IY"	},
	{R_SP,	"SP"	},
	{R_A,	"A"		},
	{R_B,	"B"		},
	{R_C,	"C"		},
	{R_D,	"D"		},
	{R_E,	"E"		},
	{R_H,	"H"		},
	{R_L,	"L"		},
	{R_R,	"R"		},
	{R_I,	"I"		},
};



static const REGISTER_NAME flagList[] =
{
	{F_NZ,	"NZ"	},		// non-zero
	{F_Z,	"Z"		},		// zero
	{F_NC,	"NC"	},		// no carry
	{F_C,	"C"		},		// carry
	{F_PO,	"PO"	},		// parity odd
	{F_PE,	"PE"	},		// parity even
	{F_P,	"P"		},		// plus
	{F_M,	"M"		},		// minus
};



struct OPERAND
{
	unsigned int
		type;		// type of operand located
	int
		value,		// first immediate value, non-immediate value, or register offset
		bit;		// bit number if present
	bool
		unresolved,		// if value was an expression, this tells if it was unresolved
		bitUnresolved;	// if bit number was an expression, this tells if it was unresolved
};


// enumerated addressing modes (yikes!)

enum
{
	AM_IMP,				// implied (no operands)
	AM_IMM,				// immediate (operate on accumulator)
	AM_HL_IND,			// (HL) -- HL indirect (operate on accumulator)
	AM_IX_IND,			// (IX)
	AM_IY_IND,			// (IY)
	AM_IX_IDX,			// (IX+d)
	AM_IY_IDX,			// (IY+d)

	AM_RELATIVE,				// relative (for branching)

	// address modes with register as the destination
	AM_REG8,			// 8-bit register -- A, B, C, D, E, H, or L
	AM_REG16,			// 16-bit register -- BC, DE, HL, or SP
	AM_REG16P,			// 16-bit register -- BC, DE, HL, or AF (used for push/pop only)
	AM_REGIX,			// 16-bit register -- IX
	AM_REGIY,			// 16-bit register -- IY

	AM_FLAG,			// status flag

	AM_RST,				// special case for restart (rst) opcode
	AM_IM,				// special case for interrupt mode

	AM_EXTENDED,		// extended (16-bit)

	AM_BIT_REG8,		// bit,8-bit register:		bit 3,A
	AM_BIT_HL_IND,		// bit,HL indirect:			bit 3,(HL)
	AM_BIT_IX_IND,		// bit,8-bit indexed IX:	bit 3,(IX+d)
	AM_BIT_IY_IND,		// bit,8-bit indexed IY:	bit 3,(IY+d)

	AM_FLAG_EXTENDED,	// status flag,extended (e.g., call nz,label)
	AM_FLAG_RELATIVE,	// status flag,relative (e.g., jr c,label)

	AM_A_IND8,			// A,(nn)
	AM_IND8_A,			// (nn),A
	AM_REG_C,			// r,(C)
	AM_C_REG,			// (C),r

	AM_A_IR,			// A,I or A,R
	AM_IR_A,			// I,A or R,A

	AM_SP_IND_HL,		// (SP),HL
	AM_SP_IND_IX,		// (SP),IX
	AM_SP_IND_IY,		// (SP),IY
	AM_DE_HL,			// DE,HL
	AM_AF_AFP,			// AF,AF'

	AM_A_REG8,			// A,8-bit register:			adc	A,B
	AM_A_IMM,			// immediate:					adc	A,n
	AM_A_HL_IND,		// A,HL indirect:				adc	A,(HL)
	AM_A_IX_IDX,		// A,8-bit indexed IX:			adc	A,(IX+d)
	AM_A_IY_IDX,		// A,8-bit indexed IY:			adc	A,(IY+d)
	AM_A_BCDE_IND,		// A,BC or DE indirect:			ld	A,(BC)
	AM_A_EXTENDED,		// A,extended (16-bit):			ld	A,(nn)
	AM_BCDE_IND_A,		// BC or DE indirect,A:			ld	(BC),A
	AM_EXTENDED_A,		// extended,A:					ld	(nn),A

	AM_REG8_REG8,		// register, register:  		ld	A,B
	AM_REG8_IMM,		// register, immediate:			ld	D,n
	AM_REG8_HL_IND,		// register, HL indirect:		ld	D,(HL)
	AM_REG8_IX_IDX,		// register, IX indexed:		ld	D,(IX+d)
	AM_REG8_IY_IDX,		// register, IY indexed:		ld	D,(IY+d)

	AM_REG16_IMM,		// register pair, immediate:	ld	DE,n
	AM_IXIY_IMM,		// register pair, immediate:	ld	IX,n
	AM_HL_EXTENDED,		// HL, extended:				ld	HL,(nn)
	AM_REG16_EXTENDED,	// register pair, extended:		ld	BC,(nn)
	AM_IXIY_EXTENDED,	// register pair, extended:		ld	IX,(nn)
	AM_EXTENDED_HL,		// extended, HL:				ld	(nn),HL
	AM_EXTENDED_REG16,	// extended, register pair:		ld	(nn),BC
	AM_EXTENDED_IXIY,	// extended, register pair:		ld	(nn),IX

	AM_HL_IND_REG8,		// HL indirect, register:		ld	(HL),D
	AM_IX_IDX_REG8,		// IX indexed, register:		ld	(IX+d),D
	AM_IY_IDX_REG8,		// IY indexed, register:		ld	(IY+d),D
	AM_HL_IND_IMM,		// HL indirect, immediate:		ld	(HL),n
	AM_IX_IDX_IMM,		// IX indexed, immediate:		ld	(IX+d),n
	AM_IY_IDX_IMM,		// IY indexed, immediate:		ld	(IY+d),n
	
	AM_SP_HL,			// 								ld	sp,hl
	AM_SP_IXIY,			// 								ld	sp,ix
	
	AM_HL_REG16,		// HL, register pair:			add	hl,bc
	AM_IX_REG16,		// IX, register pair:			add	ix,bc
	AM_IY_REG16,		// IY, register pair:			add	iy,bc

	// some z180-only modes
	AM_REG8_IND8,		// r,(nn)
	AM_IND8_REG8,		// (nn),r
	AM_SS,				// ss
};

struct ADDRESSING_MODE
{
	unsigned int
		mode;			// address mode
	unsigned char
		opcodeLen;			// number of bytes in the opcode (excluding trailer)
	unsigned short
		baseOpcode;			// base opcode for this combination of modes
	unsigned char
		trailLen;			// number of trailing bytes (1 or 0)
	unsigned char
		trailOpcode;		// trailing opcode byte
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
		{"db",		HandleDB},
		{"dc.b",	HandleDB},
		{"dw",		HandleLEDW},		// words are little endian
		{"dc.w",	HandleLEDW},
		{"ds",		HandleDS},
		{"ds.b",	HandleDS},
		{"ds.w",	HandleDSW},
		{"incbin",	HandleIncbin},
	};

#define	MODES(modeArray)	sizeof(modeArray)/sizeof(ADDRESSING_MODE),&modeArray[0]

static ADDRESSING_MODE
	M_ADC[]=
	{
		{AM_A_REG8,			1,	0x0088,	0,	0,		0},
		{AM_A_IMM,			1,	0x00ce,	0,	0,		0},
		{AM_A_HL_IND,		1,	0x008e,	0,	0,		0},
		{AM_A_IX_IDX,		2,	0xdd8e,	0,	0,		0},
		{AM_A_IY_IDX,		2,	0xfd8e,	0,	0,		0},
		{AM_HL_REG16,		2,	0xed4a,	0,	0,		4},
	},
	M_ADD[]=
	{
		{AM_A_REG8,			1,	0x0080,	0,	0,		0},
		{AM_A_IMM,			1,	0x00c6,	0,	0,		0},
		{AM_A_HL_IND,		1,	0x0086,	0,	0,		0},
		{AM_A_IX_IDX,		2,	0xdd86,	0,	0,		0},
		{AM_A_IY_IDX,		2,	0xfd86,	0,	0,		0},
		{AM_HL_REG16,		1,	0x0009,	0,	0,		4},
		{AM_IX_REG16,		2,	0xdd09,	0,	0,		4},
		{AM_IY_REG16,		2,	0xfd09,	0,	0,		4},
	},
	M_AND[]=
	{
		{AM_REG8,			1,	0x00a0,	0,	0,		0},
		{AM_IMM,			1,	0x00e6,	0,	0,		0},
		{AM_HL_IND,			1,	0x00a6,	0,	0,		0},
		{AM_IX_IDX,			2,	0xdda6,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfda6,	0,	0,		0},
	},
	M_BIT[]=
	{
		{AM_BIT_REG8,		2,	0xcb40,	0,	0,		0},
		{AM_BIT_HL_IND,		2,	0xcb46,	0,	0,		0},
		{AM_BIT_IX_IND,		2,	0xddcb,	1,	0x46,	0},
		{AM_BIT_IY_IND,		2,	0xfdcb,	1,	0x46,	0},
	},
	M_CALL[]=
	{
		{AM_EXTENDED,		1,	0x00cd,	0,	0,		0},
		{AM_FLAG_EXTENDED,	1,	0x00c4,	0,	0,		3},
	},
	M_CCF[]=
	{
		{AM_IMP,			1,	0x003f,	0,	0,		0},
	},
	M_CP[]=
	{
		{AM_REG8,			1,	0x00b8,	0,	0,		0},
		{AM_IMM,			1,	0x00fe,	0,	0,		0},
		{AM_HL_IND,			1,	0x00be,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddbe,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfdbe,	0,	0,		0},
	},
	M_CPD[]=
	{
		{AM_IMP,			2,	0xeda9,	0,	0,		0},
	},
	M_CPDR[]=
	{
		{AM_IMP,			2,	0xedb9,	0,	0,		0},
	},
	M_CPI[]=
	{
		{AM_IMP,			2,	0xeda1,	0,	0,		0},
	},
	M_CPIR[]=
	{
		{AM_IMP,			2,	0xedb1,	0,	0,		0},
	},
	M_CPL[]=
	{
		{AM_IMP,			1,	0x002f,	0,	0,		0},
	},
	M_DAA[]=
	{
		{AM_IMP,			1,	0x0027,	0,	0,		0},
	},
	M_DEC[]=
	{
		{AM_REG8,			1,	0x0005,	0,	0,		3},
		{AM_REG16,			1,	0x000b,	0,	0,		4},
		{AM_REGIX,			2,	0xdd2b,	0,	0,		0},
		{AM_REGIY,			2,	0xfd2b,	0,	0,		0},
		{AM_HL_IND,			1,	0x0035,	0,	0,		0},
		{AM_IX_IDX,			2,	0xdd35,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfd35,	0,	0,		0},
	},
	M_DI[]=
	{
		{AM_IMP,			1,	0x00f3,	0,	0,		0},
	},
	M_DJNZ[]=
	{
		{AM_RELATIVE,			1,	0x0010,	0,	0,		0},
	},
	M_EI[]=
	{
		{AM_IMP,			1,	0x00fb,	0,	0,		0},
	},
	M_EX[]=
	{
		{AM_SP_IND_HL,		1,	0x00e3,	0,	0,		0},
		{AM_SP_IND_IX,		2,	0xdde3,	0,	0,		0},
		{AM_SP_IND_IY,		2,	0xfde3,	0,	0,		0},
		{AM_DE_HL,			1,	0x00eb,	0,	0,		0},
		{AM_AF_AFP,			1,	0x0008,	0,	0,		0},
	},
	M_EXX[]=
	{
		{AM_IMP,			1,	0x00d9,	0,	0,		0},
	},
	M_HALT[]=
	{
		{AM_IMP,			1,	0x0076,	0,	0,		0},
	},
	M_IM[]=
	{
		{AM_IM,				1,	0x00ed,	0,	0,		0},
	},
	M_IN[]=
	{
		{AM_A_IND8,			1,	0x00db,	0,	0,		0},
		{AM_REG_C,			2,	0xed40,	0,	0,		3},
	},
	M_INC[]=
	{
		{AM_REG8,			1,	0x0004,	0,	0,		3},
		{AM_REG16,			1,	0x0003,	0,	0,		4},
		{AM_REGIX,			2,	0xdd23,	0,	0,		0},
		{AM_REGIY,			2,	0xfd23,	0,	0,		0},
		{AM_HL_IND,			1,	0x0034,	0,	0,		0},
		{AM_IX_IDX,			2,	0xdd34,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfd34,	0,	0,		0},
	},
	M_IND[]=
	{
		{AM_IMP,			2,	0xedaa,	0,	0,		0},
	},
	M_INDR[]=
	{
		{AM_IMP,			2,	0xedba,	0,	0,		0},
	},
	M_INI[]=
	{
		{AM_IMP,			2,	0xeda2,	0,	0,		0},
	},
	M_INIR[]=
	{
		{AM_IMP,			2,	0xedb2,	0,	0,		0},
	},
	M_JP[]=
	{
		{AM_EXTENDED,		1,	0x00c3,	0,	0,		0},
		{AM_FLAG_EXTENDED,	1,	0x00c2,	0,	0,		3},
		{AM_HL_IND,			1,	0x00e9,	0,	0,		0},
		{AM_IX_IND,			2,	0xdde9,	0,	0,		0},
		{AM_IY_IND,			2,	0xfde9,	0,	0,		0},
	},
	M_JR[]=
	{
		{AM_RELATIVE,		1,	0x0018,	0,	0,		0},
		{AM_FLAG_RELATIVE,	1,	0x0020,	0,	0,		3},
	},
	M_LD[]=
	{
		{AM_REG8_REG8,		1,	0x0040,	0,	0,		3},
		{AM_REG8_IMM,		1,	0x0006,	0,	0,		3},
		{AM_REG8_HL_IND,	1,	0x0046,	0,	0,		3},
		{AM_REG8_IX_IDX,	2,	0xdd46,	0,	0,		3},
		{AM_REG8_IY_IDX,	2,	0xfd46,	0,	0,		3},
		{AM_HL_IND_REG8,	1,	0x0070,	0,	0,		0},
		{AM_IX_IDX_REG8,	2,	0xdd70,	0,	0,		0},
		{AM_IY_IDX_REG8,	2,	0xfd70,	0,	0,		0},
		{AM_HL_IND_IMM,		1,	0x0036,	0,	0,		0},
		{AM_IX_IDX_IMM,		2,	0xdd36,	0,	0,		0},
		{AM_IY_IDX_IMM,		2,	0xfd36,	0,	0,		0},
		{AM_A_BCDE_IND,		1,	0x000a,	0,	0,		4},
		{AM_A_EXTENDED,		1,	0x003a,	0,	0,		0},
		{AM_BCDE_IND_A,		1,	0x0002,	0,	0,		4},
		{AM_EXTENDED_A,		1,	0x0032,	0,	0,		0},
		{AM_A_IR,			2,	0xed57,	0,	0,		3},
		{AM_IR_A,			2,	0xed47,	0,	0,		3},
		{AM_REG16_IMM,		1,	0x0001,	0,	0,		4},
		{AM_IXIY_IMM,		2,	0xdd21,	0,	0,		13},
		{AM_HL_EXTENDED,	1,	0x002a,	0,	0,		0},		// put this before REG16_EXT (it's shorter)
		{AM_REG16_EXTENDED,	2,	0xed4b,	0,	0,		4},
		{AM_IXIY_EXTENDED,	2,	0xdd2a,	0,	0,		13},
		{AM_EXTENDED_HL	,	1,	0x0022,	0,	0,		0},		// put this before EXT_REG16 (it's shorter)
		{AM_EXTENDED_REG16,	2,	0xed43,	0,	0,		4},
		{AM_EXTENDED_IXIY,	2,	0xdd22,	0,	0,		13},
		{AM_SP_HL,			1,	0x00f9,	0,	0,		0},
		{AM_SP_IXIY,		2,	0xddf9,	0,	0,		13},
	},
	M_LDD[]=
	{
		{AM_IMP,			2,	0xeda8,	0,	0,		0},
	},
	M_LDDR[]=
	{
		{AM_IMP,			2,	0xedb8,	0,	0,		0},
	},
	M_LDI[]=
	{
		{AM_IMP,			2,	0xeda0,	0,	0,		0},
	},
	M_LDIR[]=
	{
		{AM_IMP,			2,	0xedb0,	0,	0,		0},
	},
	M_NEG[]=
	{
		{AM_IMP,			2,	0xed44,	0,	0,		0},
	},
	M_NOP[]=
	{
		{AM_IMP,			1,	0x0000,	0,	0,		0},
	},
	M_OR[]=
	{
		{AM_REG8,			1,	0x00b0,	0,	0,		0},
		{AM_IMM,			1,	0x00f6,	0,	0,		0},
		{AM_HL_IND,			1,	0x00b6,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddb6,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfdb6,	0,	0,		0},
	},
	M_OTDR[]=
	{
		{AM_IMP,			2,	0xedbb,	0,	0,		0},
	},
	M_OTIR[]=
	{
		{AM_IMP,			2,	0xedb3,	0,	0,		0},
	},
	M_OUT[]=
	{
		{AM_IND8_A,			1,	0x00d3,	0,	0,		0},
		{AM_C_REG,			2,	0xed41,	0,	0,		3},
	},
	M_OUTD[]=
	{
		{AM_IMP,			2,	0xedab,	0,	0,		0},
	},
	M_OUTI[]=
	{
		{AM_IMP,			2,	0xeda3,	0,	0,		0},
	},
	M_POP[]=
	{
		{AM_REG16P,			1,	0x00c1,	0,	0,		4},
		{AM_REGIX,			2,	0xdde1,	0,	0,		0},
		{AM_REGIY,			2,	0xfde1,	0,	0,		0},
	},
	M_PUSH[]=
	{
		{AM_REG16P,			1,	0x00c5,	0,	0,		4},
		{AM_REGIX,			2,	0xdde5,	0,	0,		0},
		{AM_REGIY,			2,	0xfde5,	0,	0,		0},
	},
	M_RES[]=
	{
		{AM_BIT_REG8,		2,	0xcb80,	0,	0,		0},
		{AM_BIT_HL_IND,		2,	0xcb86,	0,	0,		0},
		{AM_BIT_IX_IND,		2,	0xddcb,	1,	0x86,	0},
		{AM_BIT_IY_IND,		2,	0xfdcb,	1,	0x86,	0},
	},
	M_RET[]=
	{
		{AM_IMP,			1,	0x00c9,	0,	0,		0},
		{AM_FLAG,			1,	0x00c0,	0,	0,		3},
	},
	M_RETI[]=
	{
		{AM_IMP,			2,	0xed4d,	0,	0,		0},
	},
	M_RETN[]=
	{
		{AM_IMP,			2,	0xed45,	0,	0,		0},
	},
	M_RL[]=
	{
		{AM_REG8,			2,	0xcb10,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb16,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x16,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x16,	0},
	},
	M_RLA[]=
	{
		{AM_IMP,			1,	0x0017,	0,	0,		0},
	},
	M_RLC[]=
	{
		{AM_REG8,			2,	0xcb00,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb06,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x06,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x06,	0},
	},
	M_RLCA[]=
	{
		{AM_IMP,			1,	0x0007,	0,	0,		0},
	},
	M_RLD[]=
	{
		{AM_IMP,			2,	0xed6f,	0,	0,		0},
	},
	M_RR[]=
	{
		{AM_REG8,			2,	0xcb18,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb1e,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x1e,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x1e,	0},
	},
	M_RRA[]=
	{
		{AM_IMP,			1,	0x001f,	0,	0,		0},
	},
	M_RRC[]=
	{
		{AM_REG8,			2,	0xcb08,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb0e,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x0e,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x0e,	0},
	},
	M_RRCA[]=
	{
		{AM_IMP,			1,	0x000f,	0,	0,		0},
	},
	M_RRD[]=
	{
		{AM_IMP,			2,	0xed67,	0,	0,		0},
	},
	M_RST[]=
	{
		{AM_RST,			1,	0x00c7,	0,	0,		0},
	},
	M_SCF[]=
	{
		{AM_IMP,			1,	0x0037,	0,	0,		0},
	},
	M_SET[]=
	{
		{AM_BIT_REG8,		2,	0xcbc0,	0,	0,		0},
		{AM_BIT_HL_IND,		2,	0xcbc6,	0,	0,		0},
		{AM_BIT_IX_IND,		2,	0xddcb,	1,	0xc6,	0},
		{AM_BIT_IY_IND,		2,	0xfdcb,	1,	0xc6,	0},
	},
	M_SLA[]=
	{
		{AM_REG8,			2,	0xcb20,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb26,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x26,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x26,	0},
	},
	M_SRA[]=
	{
		{AM_REG8,			2,	0xcb28,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb2e,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x2e,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x2e,	0},
	},
	M_SRL[]=
	{
		{AM_REG8,			2,	0xcb38,	0,	0,		0},
		{AM_HL_IND,			2,	0xcb3e,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddcb,	1,	0x3e,	0},
		{AM_IY_IDX,			2,	0xfdcb,	1,	0x3e,	0},
	},
	M_SBC[]=
	{
		{AM_A_REG8,			1,	0x0098,	0,	0,		0},
		{AM_A_IMM,			1,	0x00de,	0,	0,		0},
		{AM_A_HL_IND,		1,	0x009e,	0,	0,		0},
		{AM_A_IX_IDX,		2,	0xdd9e,	0,	0,		0},
		{AM_A_IY_IDX,		2,	0xfd9e,	0,	0,		0},
		{AM_HL_REG16,		2,	0xed42,	0,	0,		4},
	},
	M_SUB[]=
	{
		{AM_REG8,			1,	0x0090,	0,	0,		0},
		{AM_IMM,			1,	0x00d6,	0,	0,		0},
		{AM_HL_IND,			1,	0x0096,	0,	0,		0},
		{AM_IX_IDX,			2,	0xdd96,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfd96,	0,	0,		0},
	},
	M_XOR[]=
	{
		{AM_REG8,			1,	0x00a8,	0,	0,		0},
		{AM_IMM,			1,	0x00ee,	0,	0,		0},
		{AM_HL_IND,			1,	0x00ae,	0,	0,		0},
		{AM_IX_IDX,			2,	0xddae,	0,	0,		0},
		{AM_IY_IDX,			2,	0xfdae,	0,	0,		0},
	},
	M_IN0[]=
	{
		{AM_REG8_IND8,		2,	0xed00,	0,	0,		3},
	},
	M_MLT[]=
	{
		{AM_SS,				2,	0xed4c,	0,	0,		4},
	},
	M_OTDM[]=
	{
		{AM_IMP,			2,	0xed8b,	0,	0,		0},
	},
	M_OTDMR[]=
	{
		{AM_IMP,			2,	0xed9b,	0,	0,		0},
	},
	M_OTIM[]=
	{
		{AM_IMP,			2,	0xed83,	0,	0,		0},
	},
	M_OTIMR[]=
	{
		{AM_IMP,			2,	0xed93,	0,	0,		0},
	},
	M_OUT0[]=
	{
		{AM_IND8_REG8,		2,	0xed01,	0,	0,		3},
	},
	M_SLP[]=
	{
		{AM_IMP,			2,	0xed76,	0,	0,		0},
	},
	M_TST[]=
	{
		{AM_A_REG8,			2,	0xed04,	0,	0,		3},
		{AM_A_HL_IND,		2,	0xed34,	0,	0,		0},
		{AM_A_IMM,			2,	0xed64,	0,	0,		0},
	},
	M_TSTIO[]=
	{
		{AM_IMM,			2,	0xed74,	0,	0,		0},
	};


static OPCODE
	Opcodes[]=
	{
		{"adc",		MODES(M_ADC)	},
		{"add",		MODES(M_ADD)	},
		{"and",		MODES(M_AND)	},
		{"bit",		MODES(M_BIT)	},
		{"call",	MODES(M_CALL)	},
		{"ccf",		MODES(M_CCF)	},
		{"cp",		MODES(M_CP)		},
		{"cpd",		MODES(M_CPD)	},
		{"cpdr",	MODES(M_CPDR)	},
		{"cpi",		MODES(M_CPI)	},
		{"cpir",	MODES(M_CPIR)	},
		{"cpl",		MODES(M_CPL)	},
		{"daa",		MODES(M_DAA)	},
		{"dec",		MODES(M_DEC)	},
		{"di",		MODES(M_DI)		},
		{"djnz",	MODES(M_DJNZ)	},
		{"ei",		MODES(M_EI)		},
		{"ex",		MODES(M_EX)		},
		{"exx",		MODES(M_EXX)	},
		{"halt",	MODES(M_HALT)	},
		{"im",		MODES(M_IM)		},
		{"in",		MODES(M_IN)		},
		{"inc",		MODES(M_INC)	},
		{"ind",		MODES(M_IND)	},
		{"indr",	MODES(M_INDR)	},
		{"ini",		MODES(M_INI)	},
		{"inir",	MODES(M_INIR)	},
		{"jp",		MODES(M_JP)		},
		{"jr",		MODES(M_JR)		},
		{"ld",		MODES(M_LD)		},
		{"ldd",		MODES(M_LDD)	},
		{"lddr",	MODES(M_LDDR)	},
		{"ldi",		MODES(M_LDI)	},
		{"ldir",	MODES(M_LDIR)	},
		{"neg",		MODES(M_NEG)	},
		{"nop",		MODES(M_NOP)	},
		{"or",		MODES(M_OR)		},
		{"otdr",	MODES(M_OTDR)	},
		{"otir",	MODES(M_OTIR)	},
		{"out",		MODES(M_OUT)	},
		{"outd",	MODES(M_OUTD)	},
		{"outi",	MODES(M_OUTI)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"res",		MODES(M_RES)	},
		{"ret",		MODES(M_RET)	},
		{"reti",	MODES(M_RETI)	},
		{"retn",	MODES(M_RETN)	},
		{"rl",		MODES(M_RL)		},
		{"rla",		MODES(M_RLA)	},
		{"rlc",		MODES(M_RLC)	},
		{"rlca",	MODES(M_RLCA)	},
		{"rld",		MODES(M_RLD)	},
		{"rr",		MODES(M_RR)		},
		{"rra",		MODES(M_RRA)	},
		{"rrc",		MODES(M_RRC)	},
		{"rrca",	MODES(M_RRCA)	},
		{"rrd",		MODES(M_RRD)	},
		{"rst",		MODES(M_RST)	},
		{"sbc",		MODES(M_SBC)	},
		{"scf",		MODES(M_SCF)	},
		{"set",		MODES(M_SET)	},
		{"sla",		MODES(M_SLA)	},
		{"sra",		MODES(M_SRA)	},
		{"srl",		MODES(M_SRL)	},
		{"sub",		MODES(M_SUB)	},
		{"xor",		MODES(M_XOR)	},
	};


// additional opcodes only available on the Z18x/64180
static OPCODE
	AdditionalZ180Opcodes[]=
	{
		{"in0",		MODES(M_IN0)		},
		{"mlt",		MODES(M_MLT)		},
		{"otdm",	MODES(M_OTDM)		},
		{"otdmr",	MODES(M_OTDMR)		},
		{"otim",	MODES(M_OTIM)		},
		{"otimr",	MODES(M_OTIMR)		},
		{"out0",	MODES(M_OUT0)		},
		{"slp",		MODES(M_SLP)		},
		{"tst",		MODES(M_TST)		},
		{"tstio",	MODES(M_TSTIO)		},
	};

//-------------------------------------------------------------------
//
// generate a one or two byte opcode as appropriate
// This will return false only if a "hard" error occurs
//
static bool GenerateOpcode(unsigned short value, unsigned char opcodeLen, LISTING_RECORD *listingRecord)
{
	bool fail = false;

	if(opcodeLen == 2)
	{
		fail = !GenerateByte((value>>8)&0xff,listingRecord);
	}
	if(!fail)
	{
		fail = !GenerateByte(value&0xff,listingRecord);
	}
	return(!fail);
}


//-------------------------------------------------------------------
//
// generate a one or two byte opcode as appropriate
// This will return false only if a "hard" error occurs
//
static bool GenerateOpcodeTrail(unsigned char value, unsigned char opcodeLen, LISTING_RECORD *listingRecord)
{
	bool fail = false;

	if(opcodeLen)
	{
		fail = !GenerateByte(value,listingRecord);
	}
	return(!fail);
}

//-------------------------------------------------------------------
//
// parse out a displacement separator (+), return true if one was found
//
static bool ParseDisplacementSeparator(const char *line,unsigned int *lineIndex)
{
	SkipWhiteSpace(line,lineIndex);				// move past any white space
	if(line[*lineIndex]=='+')					// does this look like an opcode separator?
	{
		(*lineIndex)++;							// step over it
		return(true);
	}
	return(false);
}

//-------------------------------------------------------------------
//
// skip whitespace then check for end of string, fail if not
//
static bool ParseEOS(const char *line,unsigned int *lineIndex)
{
	SkipWhiteSpace(line,lineIndex);			// move past any white space
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

	SkipWhiteSpace(line,lineIndex);		// move past any white space
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
		value;
	bool
		unresolved;
	REGISTER
		reg;
	unsigned int
		stringIndex;

	if(ParseParentheticString(line,lineIndex,string))
	{
		stringIndex = 0;
		if(ParseRegister(string,&stringIndex,&reg))
		{
			switch(reg.type)
			{
				case R_C:
					operand->type = OT_C_INDIRECT;			//   opcode (C)
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
				case R_BC:
					operand->type = OT_BC_INDIRECT;			//   opcode (BC)
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
				case R_DE:
					operand->type = OT_DE_INDIRECT;			//   opcode (DE)
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
				case R_HL:
					operand->type = OT_HL_INDIRECT;			//   opcode (HL)
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
				case R_SP:
					operand->type = OT_SP_INDIRECT;			//   opcode (SP)
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
				case R_IX:
					operand->type=OT_IX_INDIRECT;			//   opcode (IX+d)
					if(ParseDisplacementSeparator(string,&stringIndex))		// if '+'
					{
						if(ParseExpression(string,&stringIndex,&value,&unresolved))
						{
							operand->value=value;
							operand->unresolved=unresolved;
						}
					}
					else
					{
						operand->value=0;			// no displacement, assume 0
						operand->unresolved=false;
					}
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there

				case R_IY:
					operand->type=OT_IY_INDIRECT;	//   opcode (IY+d)
					if(ParseDisplacementSeparator(string,&stringIndex))		// if '+'
					{
						if(ParseExpression(string,&stringIndex,&value,&unresolved))
						{
							operand->value=value;
							operand->unresolved=unresolved;
						}
					}
					else
					{
						operand->value=0;			// no displacement, assume 0
					}
					return(ParseEOS(string,&stringIndex));	// fail if anything else in there
			}
		}
		else
		{
			if(ParseExpression(string,&stringIndex,&value,&unresolved))
			{
				operand->type=OT_INDIRECT;			// an indirect value
				operand->value=value;
				operand->unresolved=unresolved;
				return(true);
			}
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
#if 0		// immediate is not preceeded by # in z80-land -- may want to allow it anyway, someday
		else if(line[inputIndex]=='#')		// immediate?
		{
			inputIndex++;
			if(ParseExpression(line,&inputIndex,&value,&unresolved))
			{
				operand->type=OT_VALUE;		// z80 doesn't require the '#' to indicate immediate
				operand->value=value;
				operand->unresolved=unresolved;
				*lineIndex=inputIndex;
				return(true);
			}
		}
#endif
		else if(ParseRegister(line,&inputIndex,&reg))			// does it look like a register?
		{
			operand->type=reg.type-R_A+OT_A;						// kind of bad form; dependent on the order being the same
			*lineIndex=inputIndex;
			return(true);
		}
		else if(ParseFlag(line,&inputIndex,&reg))
		{
			operand->type=reg.type-F_NZ+OT_NZ;					// again, kind of bad form; dependent on the order being the same
			*lineIndex=inputIndex;
			return(true);
		}
		else if(ParseExpression(line,&inputIndex,&value,&unresolved))	// see if it looks like a value
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


//-------------------------------------------------------------------
//
// opcode handling
//
//-------------------------------------------------------------------

bool CheckRstRange(int value,bool generateMessage,bool isError)
// see if value can be used as a restart address
// if not, return false, and generate a warning or error if told to
{
	if(!(value&~0x38))		// okay as long as no other bits are set
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Page zero restart address (0x%X) invalid\n",value);
	}
	return(false);
}

bool CheckBitRange(int value,bool generateMessage,bool isError)
// see if value can be used as a bit number (0-7)
// if not, return false, and generate a warning or error if told to
{
	if( (value>=0) && (value<=7) )
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
// Given an addressing mode record, and a set of operands, generate code (or an error message if something is
// out of range)
// This will only return false on a 'hard' error
//
static bool HandleAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *destOperand,OPERAND *srcOperand,LISTING_RECORD *listingRecord)
{
	bool
		fail;
	int
		value;

	fail=false;
	switch(addressingMode->mode)
	{
		case AM_IMP:
		case AM_REGIX:
		case AM_REGIY:
		case AM_SP_IND_HL:
		case AM_SP_IND_IX:
		case AM_SP_IND_IY:
		case AM_DE_HL:
		case AM_AF_AFP:
		case AM_SP_HL:
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_IMM:
			CheckByteRange(destOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
			}
			break;

		case AM_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value = destOperand->value - (currentSegment->currentPC+currentSegment->codeGenOffset) - 2;
				Check8RelativeRange(value,true,true);
			}
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(value,listingRecord);
			}
			break;

		case AM_A_IMM:
			CheckByteRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
			}
			break;

		case AM_EXTENDED:
			CheckWordRange(destOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value&0xff,listingRecord);		// store address little endian
				if(!fail)
				{
					fail = !GenerateByte((destOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		case AM_A_EXTENDED:
			CheckWordRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value&0xff,listingRecord);		// store address little endian
				if(!fail)
				{
					fail = !GenerateByte((srcOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		case AM_EXTENDED_A:
			CheckWordRange(destOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value&0xff,listingRecord);		// store address little endian
				if(!fail)
				{
					fail = !GenerateByte((destOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		case AM_HL_IND:
		case AM_A_HL_IND:
		case AM_IX_IND:
		case AM_IY_IND:
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			break;
	
		case AM_A_BCDE_IND:
		case AM_SP_IXIY:
		case AM_A_REG8:
		case AM_A_IR:
		case AM_C_REG:
		case AM_HL_IND_REG8:
		case AM_HL_REG16:
		case AM_IX_REG16:
		case AM_IY_REG16:
			value = addressingMode->baseOpcode | (rcode[srcOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_IX_IDX:
		case AM_IY_IDX:
			CheckByteRange(destOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
				if(!fail)
				{
					fail = !GenerateOpcodeTrail(addressingMode->trailOpcode,addressingMode->trailLen,listingRecord);
				}
			}
			break;

		case AM_A_IX_IDX:
		case AM_A_IY_IDX:
			CheckByteRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
				if(!fail)
				{
					fail = !GenerateOpcodeTrail(addressingMode->trailOpcode,addressingMode->trailLen,listingRecord);
				}
			}
			break;

		case AM_REG8_IX_IDX:
		case AM_REG8_IY_IDX:
			CheckByteRange(srcOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
				if(!fail)
				{
					fail = !GenerateOpcodeTrail(addressingMode->trailOpcode,addressingMode->trailLen,listingRecord);
				}
			}
			break;

		case AM_IX_IDX_REG8:
		case AM_IY_IDX_REG8:
			CheckByteRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[srcOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
				if(!fail)
				{
					fail = !GenerateOpcodeTrail(addressingMode->trailOpcode,addressingMode->trailLen,listingRecord);
				}
			}
			break;

		case AM_IX_IDX_IMM:
		case AM_IY_IDX_IMM:
			CheckByteRange(destOperand->value,true,true);	// ensure displacement is okay
			CheckByteRange(srcOperand->value,true,true);	// ensure value is okay
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
				if(!fail)
				{
					fail = !GenerateByte(srcOperand->value,listingRecord);
				}
			}
			break;

		case AM_BCDE_IND_A:
		case AM_REG8:
		case AM_REG_C:
		case AM_REG8_HL_IND:
		case AM_REG16:
		case AM_REG16P:
		case AM_SS:
		case AM_IR_A:
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_HL_IND_IMM:
			CheckByteRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
			}
			break;

		case AM_REG8_REG8:
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset) | rcode[srcOperand->type]);
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_REG8_IMM:
			CheckByteRange(srcOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset) );
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
			}
			break;

		case AM_REG16_IMM:
		case AM_IXIY_IMM:
		case AM_HL_EXTENDED:
		case AM_REG16_EXTENDED:
		case AM_IXIY_EXTENDED:
			CheckWordRange(srcOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset) );
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value&0xff,listingRecord);
				if(!fail)
				{
					fail = !GenerateByte((srcOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		case AM_EXTENDED_HL:
		case AM_EXTENDED_REG16:
		case AM_EXTENDED_IXIY:
			CheckWordRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[srcOperand->type]<<(addressingMode->regOffset) );
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value&0xff,listingRecord);
				if(!fail)
				{
					fail = !GenerateByte((destOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		// nasty kludge to deal with the fact that it's sometimes hard to differentiate between C, the flag and C, the register
		case AM_FLAG:
			if(destOperand->type == OT_C)
			{
				value = addressingMode->baseOpcode | (rcode[OT_CRY]<<(addressingMode->regOffset));
			}
			else
			{
				value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			}
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_FLAG_EXTENDED:
			CheckWordRange(srcOperand->value,true,true);
			if(destOperand->type == OT_C)					// kludge to deal with C
			{
				value = addressingMode->baseOpcode | (rcode[OT_CRY]<<(addressingMode->regOffset));
			}
			else
			{
				value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			}
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value&0xff,listingRecord);		// store address little endian
				if(!fail)
				{
					fail = !GenerateByte((srcOperand->value>>8)&0xff,listingRecord);
				}
			}
			break;

		case AM_FLAG_RELATIVE:
			if(destOperand->type == OT_C)					// kludge to deal with C
			{
				value = addressingMode->baseOpcode | (rcode[OT_CRY]<<(addressingMode->regOffset));
			}
			else
			{
				value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			}
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				value=0;
				if(currentSegment)
				{
					value = srcOperand->value - (currentSegment->currentPC+currentSegment->codeGenOffset) - 1;
					Check8RelativeRange(value,true,true);
				}
				fail = !GenerateByte(value,listingRecord);
			}
			break;

		case AM_IM:
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			switch(destOperand->value)
			{
				case 0:
					fail = !GenerateByte(0x46,listingRecord);
					break;
				case 1:
					fail = !GenerateByte(0x56,listingRecord);
					break;
				case 2:
					fail = !GenerateByte(0x5e,listingRecord);
					break;
				default:
					fail = true;
					break;
			}
			break;

		case AM_RST:
			CheckRstRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | destOperand->value;
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_BIT_REG8:
			CheckBitRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | (destOperand->value<<3) | (rcode[srcOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_BIT_HL_IND:
			CheckBitRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | (destOperand->value<<3);
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			break;

		case AM_BIT_IX_IND:
		case AM_BIT_IY_IND:
			CheckBitRange(destOperand->value,true,true);
			CheckByteRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
				if(!fail)
				{
					value = addressingMode->trailOpcode | (destOperand->value<<3);
					fail = !GenerateOpcodeTrail(value,addressingMode->trailLen,listingRecord);
				}
			}
			break;

		case AM_A_IND8:
			CheckByteRange(srcOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
			}
			break;

		case AM_IND8_A:
			CheckByteRange(destOperand->value,true,true);
			fail = !GenerateOpcode(addressingMode->baseOpcode,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
			}
			break;
			
		case AM_REG8_IND8:
			CheckByteRange(srcOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[destOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(srcOperand->value,listingRecord);
			}
			break;

		case AM_IND8_REG8:
			CheckByteRange(destOperand->value,true,true);
			value = addressingMode->baseOpcode | (rcode[srcOperand->type]<<(addressingMode->regOffset));
			fail = !GenerateOpcode(value,addressingMode->opcodeLen,listingRecord);
			if(!fail)
			{
				fail = !GenerateByte(destOperand->value,listingRecord);
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
static bool OperandsMatchAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *destOperand,OPERAND *srcOperand)
{
	switch(addressingMode->mode)
	{
		case AM_IMP:
			return(numOperands==0);
			break;

		case AM_IMM:
			return( (numOperands==1) && (destOperand->type==OT_VALUE) );
			break;

		case AM_A_IMM:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_VALUE) );
			break;

 		case AM_RELATIVE:
 			return( (numOperands==1) && (destOperand->type==OT_VALUE) );
			break;

		case AM_HL_IND:
			return( (numOperands==1) && (destOperand->type==OT_HL_INDIRECT) );
			break;

		case AM_A_HL_IND:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_HL_INDIRECT) );
			break;

		case AM_IX_IND:
			return( (numOperands==1) && (destOperand->type==OT_IX_INDIRECT) && (destOperand->value==0) );
			break;

		case AM_IY_IND:
			return( (numOperands==1) && (destOperand->type==OT_IY_INDIRECT) && (destOperand->value==0) );
			break;

		case AM_A_BCDE_IND:
			return( (numOperands==2) && (destOperand->type==OT_A) && ( (srcOperand->type==OT_BC_INDIRECT) || (srcOperand->type==OT_DE_INDIRECT) ) );
			break;

		case AM_BCDE_IND_A:
			return( (numOperands==2) && ( (destOperand->type==OT_BC_INDIRECT) || (destOperand->type==OT_DE_INDIRECT) ) && (srcOperand->type==OT_A) );
			break;

		case AM_IX_IDX:
			return( (numOperands==1) && (destOperand->type==OT_IX_INDIRECT) );
			break;

		case AM_A_IX_IDX:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_IX_INDIRECT) );
			break;

		case AM_REG8_IX_IDX:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_IX_INDIRECT) );
			break;
			
		case AM_IX_IDX_REG8:
			return( (numOperands==2) && (destOperand->type==OT_IX_INDIRECT) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;
			
		case AM_IX_IDX_IMM:
			return( (numOperands==2) && (destOperand->type==OT_IX_INDIRECT) && (srcOperand->type==OT_VALUE) );
			break;
			
		case AM_IY_IDX:
			return( (numOperands==1) && (destOperand->type==OT_IY_INDIRECT) );
			break;

		case AM_A_IY_IDX:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_IY_INDIRECT) );
			break;

		case AM_REG8_IY_IDX:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_IY_INDIRECT) );
			break;
			
		case AM_IY_IDX_REG8:
			return( (numOperands==2) && (destOperand->type==OT_IY_INDIRECT) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;
			
		case AM_IY_IDX_IMM:
			return( (numOperands==2) && (destOperand->type==OT_IY_INDIRECT) && (srcOperand->type==OT_VALUE) );
			break;
			
		case AM_REG8:
			return( (numOperands==1) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) );
			break;

		case AM_REG16:
			return( (numOperands==1) && (((destOperand->type>=OT_BC) && (destOperand->type<=OT_HL)) || (destOperand->type==OT_SP)) );
			break;

		case AM_REG16P:
			return( (numOperands==1) && (((destOperand->type>=OT_BC) && (destOperand->type<=OT_HL)) || (destOperand->type==OT_AF)) );
			break;

		case AM_REGIX:
			return( (numOperands==1) && (destOperand->type==OT_IX) );
			break;

		case AM_REGIY:
			return( (numOperands==1) && (destOperand->type==OT_IY) );
			break;

		case AM_REG8_REG8:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L));
			break;

		case AM_REG8_IMM:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_VALUE) );
			break;

		case AM_REG16_IMM:
			return( (numOperands==2) && (destOperand->type>=OT_BC) && (destOperand->type<=OT_SP) && (srcOperand->type==OT_VALUE) );
			break;

		case AM_HL_EXTENDED:
			return( (numOperands==2) && (destOperand->type==OT_HL) && (srcOperand->type==OT_INDIRECT) );
			break;

		case AM_REG16_EXTENDED:
			return( (numOperands==2) && (destOperand->type>=OT_BC) && (destOperand->type<=OT_SP) && (srcOperand->type==OT_INDIRECT) );
			break;

		case AM_IXIY_IMM:
			return( (numOperands==2) && (destOperand->type>=OT_IX) && (destOperand->type<=OT_IY) && (srcOperand->type==OT_VALUE) );
			break;

		case AM_IXIY_EXTENDED:
			return( (numOperands==2) && (destOperand->type>=OT_IX) && (destOperand->type<=OT_IY) && (srcOperand->type==OT_INDIRECT) );
			break;

		case AM_EXTENDED_HL:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type==OT_HL) );
			break;

		case AM_EXTENDED_REG16:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type>=OT_BC) && (srcOperand->type<=OT_SP) );
			break;

		case AM_EXTENDED_IXIY:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type>=OT_IX) && (srcOperand->type<=OT_IY) );
			break;

		// nasty kludge to deal with the fact that it's sometimes hard to differentiate between C, the flag and C, the register
		case AM_FLAG:
			return( (numOperands==1) && (((destOperand->type>=OT_NZ) && (destOperand->type<=OT_M)) || (destOperand->type==OT_C) ) );
			break;

		case AM_FLAG_EXTENDED:
			return( (numOperands==2) && (((destOperand->type>=OT_NZ) && (destOperand->type<=OT_M)) || ((destOperand->type==OT_C) && (srcOperand->type==OT_VALUE))) );
			break;

		case AM_FLAG_RELATIVE:
			return( (numOperands==2) && (((destOperand->type>=OT_NZ) && (destOperand->type<=OT_CRY)) || ((destOperand->type==OT_C) && (srcOperand->type==OT_VALUE))) );
			break;
			
		case AM_EXTENDED:
		case AM_IM:
		case AM_RST:
			return( (numOperands==1) && (destOperand->type==OT_VALUE) );
			break;

		case AM_A_EXTENDED:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_INDIRECT) );
			break;

		case AM_EXTENDED_A:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type==OT_A) );
			break;

		case AM_BIT_REG8:
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;

		case AM_BIT_HL_IND:
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_HL_INDIRECT) );
			break;

		case AM_BIT_IX_IND:
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_IX_INDIRECT) );
			break;

		case AM_BIT_IY_IND:
			return( (numOperands==2) && (destOperand->type==OT_VALUE) && (srcOperand->type==OT_IY_INDIRECT) );
			break;

		case AM_A_IND8:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type==OT_INDIRECT) );
			break;

		case AM_IND8_A:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type==OT_A) );
			break;

		case AM_REG_C:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_C_INDIRECT) );
			break;

		case AM_C_REG:
			return( (numOperands==2) && (destOperand->type==OT_C_INDIRECT) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;

		case AM_SP_IND_HL:
			return( (numOperands==2) && (destOperand->type==OT_SP_INDIRECT) && (srcOperand->type==OT_HL) );
			break;

		case AM_SP_IND_IX:
			return( (numOperands==2) && (destOperand->type==OT_SP_INDIRECT) && (srcOperand->type==OT_IX) );
			break;

		case AM_SP_IND_IY:
			return( (numOperands==2) && (destOperand->type==OT_SP_INDIRECT) && (srcOperand->type==OT_IY) );
			break;

		case AM_DE_HL:
			return( (numOperands==2) && (destOperand->type==OT_DE) && (srcOperand->type==OT_HL) );
			break;

		case AM_AF_AFP:
			return( (numOperands==2) && (destOperand->type==OT_AF) && (srcOperand->type==OT_AFP) );
			break;

		case AM_A_REG8:
			return( (numOperands==2) && (destOperand->type==OT_A) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;

		case AM_A_IR:
			return( (numOperands==2) && (destOperand->type==OT_A) && ((srcOperand->type==OT_I) || (srcOperand->type==OT_R)) );
			break;
			
		case AM_IR_A:
			return( (numOperands==2) && ((destOperand->type==OT_I) || (destOperand->type==OT_R)) && (srcOperand->type==OT_A) );
			break;
			
		case AM_REG8_HL_IND:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_HL_INDIRECT) );
			break;

		case AM_HL_IND_REG8:
			return( (numOperands==2) && (destOperand->type==OT_HL_INDIRECT) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;

		case AM_HL_REG16:
			return( (numOperands==2) && (destOperand->type==OT_HL) && (srcOperand->type>=OT_BC) && (srcOperand->type<=OT_SP) );
			break;

		case AM_IX_REG16:
			return( (numOperands==2) && (destOperand->type==OT_IX) && (srcOperand->type>=OT_BC) && (srcOperand->type<=OT_SP) );
			break;

		case AM_IY_REG16:
			return( (numOperands==2) && (destOperand->type==OT_IY) && (srcOperand->type>=OT_BC) && (srcOperand->type<=OT_SP) );
			break;

		case AM_HL_IND_IMM:
			return( (numOperands==2) && (destOperand->type==OT_HL_INDIRECT) && (srcOperand->type==OT_VALUE) );
			break;

		case AM_SP_HL:
			return( (numOperands==2) && (destOperand->type==OT_SP) && (srcOperand->type==OT_HL) );
			break;

		case AM_SP_IXIY:
			return( (numOperands==2) && (destOperand->type==OT_SP) && (srcOperand->type>=OT_IX) && (srcOperand->type<=OT_IY) );
			break;
			
		case AM_REG8_IND8:
			return( (numOperands==2) && (destOperand->type>=OT_A) && (destOperand->type<=OT_L) && (srcOperand->type==OT_INDIRECT) );
			break;
			
		case AM_IND8_REG8:
			return( (numOperands==2) && (destOperand->type==OT_INDIRECT) && (srcOperand->type>=OT_A) && (srcOperand->type<=OT_L) );
			break;
			
		case AM_SS:
			return( (numOperands==1) && (destOperand->type>=OT_BC) && (destOperand->type<=OT_SP) );
			break;

	}
	return(false);
}

static bool ParseOperands(const char *line,unsigned int *lineIndex,unsigned int *numOperands,OPERAND *destOperand,OPERAND *srcOperand)
// parse from 0 to 3 operands from the line
// return the operands parsed
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
		srcOperand;
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
			if(ParseOperands(line,lineIndex,&numOperands,&destOperand,&srcOperand))	// fetch operands for opcode
			{
				done=false;
				for(i=0;!done&&(i<opcode->numModes);i++)
				{
					if(OperandsMatchAddressingMode(&(opcode->addressingModes[i]),numOperands,&destOperand,&srcOperand))
					{
						result=HandleAddressingMode(&(opcode->addressingModes[i]),numOperands,&destOperand,&srcOperand,listingRecord);
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
	return(true);
}

static void DeselectProcessor(PROCESSOR *processor)
// A processor in this family is being deselected
{
}

static void UnInitFamily()
// undo what InitFamily did
{
	STDisposeSymbolTable(opcodeZ180Symbols);
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
	processorFamily("Zilog Z80",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"z80",NULL),
		PROCESSOR(&processorFamily,"z180",&opcodeZ180Symbols),
	};
