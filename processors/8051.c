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


// Generate code for Intel 8051 family and Dallas 80C390
//
// Dallas 80C390 support courtesy of Neil Darlow

#include	"include.h"

// ACALL/AJMP operand size (805x=2K, 80C390=512K)
#define	SMALLPAGE	2048
#define	LARGEPAGE	524288

// LCALL/LJMP/MOV DPTR,#XXXX operand size (805x=64K, 80C390=16M)
#define	SMALLMEMORY	65536
#define	LARGEMEMORY	16777216

struct PROCESSOR_DATA
{
	unsigned int
		pageSize;					// page size of the given processor
	unsigned int
		memorySize;					// memory size of the given processor
};

static PROCESSOR_DATA
	*currentProcessor;				// points at the record for the currently selected processor

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols;

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
	R_A,			// A
	R_AB,			// AB
	R_DPTR,			// DPTR
	R_Rn,			// Rn
	R_CARRY,		// C
	R_PC,			// PC
};


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
	OT_A,					// A
	OT_AB,					// AB
	OT_DPTR,				// DPTR
	OT_REGISTER,			// Rn
	OT_INDIRECT,			// @Ri
	OT_CARRY,				// C
	OT_IMMEDIATE,			// #xxxx
	OT_VALUE_BIT,			// xxxx.yy
	OT_VALUE_NOTBIT,		// xxxx./yy
	OT_VALUE,				// xxxx
	OT_NOTVALUE,			// /xxxx
	OT_INDIRECTDPTR,		// @DPTR
	OT_INDIRECTAPLUSDPTR,	// @A+DPTR
	OT_INDIRECTAPLUSPC,		// @A+PC
};

// enumerated addressing modes (yikes!)

enum
{
	AM_IMPLIED,						// no operands
	AM_A,							// A
	AM_AB,							// AB
	AM_DPTR,						// DPTR
	AM_REGISTER,					// Rn
	AM_INDIRECT,					// @Ri
	AM_DIRECT,						// xx
	AM_BIT,							// addr.bit or bitaddr
	AM_ADDR_PAGE,					// 11 or 19 bit page address
	AM_ADDR_ABSOLUTE,				// 16 or 24 bit absolute address
	AM_CARRY,						// C
	AM_RELATIVE,					// 8 bit relative offset
	AM_INDIRECTAPLUSDPTR,			// @A+DPTR

	AM_A_REGISTER,					// A,Rn
	AM_A_INDIRECT,					// A,@Ri
	AM_A_DIRECT,					// A,direct
	AM_A_IMMEDIATE,					// A,#xx
	AM_A_INDIRECTDPTR,				// A,@DPTR
	AM_A_INDIRECTAPLUSDPTR,			// A,@A+DPTR
	AM_A_INDIRECTAPLUSPC,			// A,@A+PC
	AM_A_DIRECT_RELATIVE,			// A,xx,8 bit relative offset
	AM_A_IMMEDIATE_RELATIVE,		// A,#xx,8 bit relative offset

	AM_DPTR_IMMEDIATE,				// DPTR,#xxxx or #xxxxxx

	AM_REGISTER_A,					// Rn,A
	AM_REGISTER_DIRECT,				// Rn,xx
	AM_REGISTER_IMMEDIATE,			// Rn,#xx
	AM_REGISTER_RELATIVE,			// Rn,8 bit relative offset
	AM_REGISTER_IMMEDIATE_RELATIVE,	// Rn,#xx,8 bit relative offset

	AM_INDIRECT_A,					// @Ri,A
	AM_INDIRECT_DIRECT,				// @Ri,Rn
	AM_INDIRECT_IMMEDIATE,			// @Ri,#xx
	AM_INDIRECT_IMMEDIATE_RELATIVE,	// @Ri,#xx,8 bit relative offset

	AM_DIRECT_A,					// xx,A
	AM_DIRECT_REGISTER,				// xx,Rn
	AM_DIRECT_INDIRECT,				// xx,@Ri
	AM_DIRECT_DIRECT,				// xx,xx
	AM_DIRECT_IMMEDIATE,			// xx,#xx
	AM_DIRECT_RELATIVE,				// xx,8 bit relative offset

	AM_BIT_CARRY,					// addr.bit or bitaddr,C
	AM_BIT_RELATIVE,				// addr.bit or bitaddr,8 bit relative offset

	AM_CARRY_BIT,					// C,addr.bit or bitaddr
	AM_CARRY_NOTBIT,				// C,/addr.bit or /bitaddr

	AM_INDIRECTDPTR_A,				// @DPTR,A
};

struct ADDRESSING_MODE
{
	unsigned int
		mode;
	unsigned char
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


static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"db",		HandleDB},
		{"dc.b",	HandleDB},
		{"dw",		HandleBEDW},		// words are big endian (yes 8051 uses big endian for most things)
		{"dc.w",	HandleBEDW},
		{"ds",		HandleDS},
		{"ds.b",	HandleDS},
		{"ds.w",	HandleDSW},
		{"incbin",	HandleIncbin},
	};


#define	MODES(modeArray)	elementsof(modeArray),&modeArray[0]

static ADDRESSING_MODE
	M_ACALL[]=
	{
		{AM_ADDR_PAGE,					0x11},
	},
	M_ADD[]=
	{
		{AM_A_REGISTER,					0x28},
		{AM_A_DIRECT,					0x25},
		{AM_A_INDIRECT,					0x26},
		{AM_A_IMMEDIATE,				0x24},
	},
	M_ADDC[]=
	{
		{AM_A_REGISTER,					0x38},
		{AM_A_DIRECT,					0x35},
		{AM_A_INDIRECT,					0x36},
		{AM_A_IMMEDIATE,				0x34},
	},
	M_AJMP[]=
	{
		{AM_ADDR_PAGE,					0x01},
	},
	M_ANL[]=
	{
		{AM_A_REGISTER,					0x58},
		{AM_A_DIRECT,					0x55},
		{AM_A_INDIRECT,					0x56},
		{AM_A_IMMEDIATE,				0x54},
		{AM_DIRECT_A,					0x52},
		{AM_DIRECT_IMMEDIATE,			0x53},
		{AM_CARRY_BIT,					0x82},
		{AM_CARRY_NOTBIT,				0xB0},
	},
	M_CJNE[]=
	{
		{AM_A_DIRECT_RELATIVE,			0xB5},
		{AM_A_IMMEDIATE_RELATIVE,		0xB4},
		{AM_REGISTER_IMMEDIATE_RELATIVE,0xB8},
		{AM_INDIRECT_IMMEDIATE_RELATIVE,0xB6},
	},
	M_CLR[]=
	{
		{AM_A,							0xE4},
		{AM_CARRY,						0xC3},
		{AM_BIT,						0xC2},
	},
	M_CPL[]=
	{
		{AM_A,							0xF4},
		{AM_CARRY,						0xB3},
		{AM_BIT,						0xB2},
	},
	M_DA[]=
	{
		{AM_A,							0xD4},
	},
	M_DEC[]=
	{
		{AM_A,							0x14},
		{AM_REGISTER,					0x18},
		{AM_DIRECT,						0x15},
		{AM_INDIRECT,					0x16},
	},
	M_DIV[]=
	{
		{AM_AB,							0x84},
	},
	M_DJNZ[]=
	{
		{AM_REGISTER_RELATIVE,			0xD8},
		{AM_DIRECT_RELATIVE,			0xD5},
	},
	M_INC[]=
	{
		{AM_A,							0x04},
		{AM_REGISTER,					0x08},
		{AM_DIRECT,						0x05},
		{AM_INDIRECT,					0x06},
		{AM_DPTR,						0xA3},
	},
	M_JB[]=
	{
		{AM_BIT_RELATIVE,				0x20},
	},
	M_JBC[]=
	{
		{AM_BIT_RELATIVE,				0x10},
	},
	M_JC[]=
	{
		{AM_RELATIVE,					0x40},
	},
	M_JMP[]=
	{
		{AM_INDIRECTAPLUSDPTR,			0x73},
	},
	M_JNB[]=
	{
		{AM_BIT_RELATIVE,				0x30},
	},
	M_JNC[]=
	{
		{AM_RELATIVE,					0x50},
	},
	M_JNZ[]=
	{
		{AM_RELATIVE,					0x70},
	},
	M_JZ[]=
	{
		{AM_RELATIVE,					0x60},
	},
	M_LCALL[]=
	{
		{AM_ADDR_ABSOLUTE,				0x12},
	},
	M_LJMP[]=
	{
		{AM_ADDR_ABSOLUTE,				0x02},
	},
	M_MOV[]=
	{
		{AM_A_REGISTER,					0xE8},
		{AM_A_DIRECT,					0xE5},
		{AM_A_INDIRECT,					0xE6},
		{AM_A_IMMEDIATE,				0x74},
		{AM_REGISTER_A,					0xF8},
		{AM_REGISTER_DIRECT,			0xA8},
		{AM_REGISTER_IMMEDIATE,			0x78},
		{AM_DIRECT_A,					0xF5},
		{AM_DIRECT_REGISTER,			0x88},
		{AM_DIRECT_DIRECT,				0x85},
		{AM_DIRECT_INDIRECT,			0x86},
		{AM_DIRECT_IMMEDIATE,			0x75},
		{AM_INDIRECT_A,					0xF6},
		{AM_INDIRECT_DIRECT,			0xA6},
		{AM_INDIRECT_IMMEDIATE,			0x76},
		{AM_CARRY_BIT,					0xA2},
		{AM_BIT_CARRY,					0x92},
		{AM_DPTR_IMMEDIATE,				0x90},
	},
	M_MOVC[]=
	{
		{AM_A_INDIRECTAPLUSDPTR,		0x93},
		{AM_A_INDIRECTAPLUSPC,			0x83},
	},
	M_MOVX[]=
	{
		{AM_A_INDIRECT,					0xE2},
		{AM_A_INDIRECTDPTR,				0xE0},
		{AM_INDIRECT_A,					0xF2},
		{AM_INDIRECTDPTR_A,				0xF0},
	},
	M_MUL[]=
	{
		{AM_AB,							0xA4},
	},
	M_NOP[]=
	{
		{AM_IMPLIED,					0x00},
	},
	M_ORL[]=
	{
		{AM_A_REGISTER,					0x48},
		{AM_A_DIRECT,					0x45},
		{AM_A_INDIRECT,					0x46},
		{AM_A_IMMEDIATE,				0x44},
		{AM_DIRECT_A,					0x42},
		{AM_DIRECT_IMMEDIATE,			0x43},
		{AM_CARRY_BIT,					0x72},
		{AM_CARRY_NOTBIT,				0xA0},
	},
	M_POP[]=
	{
		{AM_DIRECT,						0xD0},
	},
	M_PUSH[]=
	{
		{AM_DIRECT,						0xC0},
	},
	M_RET[]=
	{
		{AM_IMPLIED,					0x22},
	},
	M_RETI[]=
	{
		{AM_IMPLIED,					0x32},
	},
	M_RL[]=
	{
		{AM_A,							0x23},
	},
	M_RLC[]=
	{
		{AM_A,							0x33},
	},
	M_RR[]=
	{
		{AM_A,							0x03},
	},
	M_RRC[]=
	{
		{AM_A,							0x13},
	},
	M_SETB[]=
	{
		{AM_CARRY,						0xD3},
		{AM_BIT,						0xD2},
	},
	M_SJMP[]=
	{
		{AM_RELATIVE,					0x80},
	},
	M_SUBB[]=
	{
		{AM_A_REGISTER,					0x98},
		{AM_A_DIRECT,					0x95},
		{AM_A_INDIRECT,					0x96},
		{AM_A_IMMEDIATE,				0x94},
	},
	M_SWAP[]=
	{
		{AM_A,							0xC4},
	},
	M_XCH[]=
	{
		{AM_A_REGISTER,					0xC8},
		{AM_A_DIRECT,					0xC5},
		{AM_A_INDIRECT,					0xC6},
	},
	M_XCHD[]=
	{
		{AM_A_INDIRECT,					0xD6},
	},
	M_XRL[]=
	{
		{AM_A_REGISTER,					0x68},
		{AM_A_DIRECT,					0x65},
		{AM_A_INDIRECT,					0x66},
		{AM_A_IMMEDIATE,				0x64},
		{AM_DIRECT_A,					0x62},
		{AM_DIRECT_IMMEDIATE,			0x63},
	};

static OPCODE
	Opcodes[]=
	{
		// opcodes

		{"acall",	MODES(M_ACALL)	},
		{"add",		MODES(M_ADD)	},
		{"addc",	MODES(M_ADDC)	},
		{"ajmp",	MODES(M_AJMP)	},
		{"anl",		MODES(M_ANL)	},
		{"cjne",	MODES(M_CJNE)	},
		{"clr",		MODES(M_CLR)	},
		{"cpl",		MODES(M_CPL)	},
		{"da",		MODES(M_DA)		},
		{"dec",		MODES(M_DEC)	},
		{"div",		MODES(M_DIV)	},
		{"djnz",	MODES(M_DJNZ)	},
		{"inc",		MODES(M_INC)	},
		{"jb",		MODES(M_JB)		},
		{"jbc",		MODES(M_JBC)	},
		{"jc",		MODES(M_JC)		},
		{"jmp",		MODES(M_JMP)	},
		{"jnb",		MODES(M_JNB)	},
		{"jnc",		MODES(M_JNC)	},
		{"jnz",		MODES(M_JNZ)	},
		{"jz",		MODES(M_JZ)		},
		{"lcall",	MODES(M_LCALL)	},
		{"ljmp",	MODES(M_LJMP)	},
		{"mov",		MODES(M_MOV)	},
		{"movc",	MODES(M_MOVC)	},
		{"movx",	MODES(M_MOVX)	},
		{"mul",		MODES(M_MUL)	},
		{"nop",		MODES(M_NOP)	},
		{"orl",		MODES(M_ORL)	},
		{"pop",		MODES(M_POP)	},
		{"push",	MODES(M_PUSH)	},
		{"ret",		MODES(M_RET)	},
		{"reti",	MODES(M_RETI)	},
		{"rl",		MODES(M_RL)		},
		{"rlc",		MODES(M_RLC)	},
		{"rr",		MODES(M_RR)		},
		{"rrc",		MODES(M_RRC)	},
		{"setb",	MODES(M_SETB)	},
		{"sjmp",	MODES(M_SJMP)	},
		{"subb",	MODES(M_SUBB)	},
		{"swap",	MODES(M_SWAP)	},
		{"xch",		MODES(M_XCH)	},
		{"xchd",	MODES(M_XCHD)	},
		{"xrl",		MODES(M_XRL)	},
	};


static bool ParseRegister(const char *line,unsigned int *lineIndex,REGISTER *reg)
// Try to parse out what looks like a register
// This will match:
// A
// AB
// DPTR
// Rn  (where n is 0-7)
// C
// PC
// NOTE: if what was located does not look like a register return false
// NOTE: this is fairly ugly and hard coded
{
	unsigned int
		inputIndex;

	inputIndex=*lineIndex;
	if(strncasecmp(&line[inputIndex],"AB",2)==0)
	{
		reg->type=R_AB;
		inputIndex+=2;
	}
	else if(strncasecmp(&line[inputIndex],"A",1)==0)
	{
		reg->type=R_A;
		inputIndex+=1;
	}
	else if(strncasecmp(&line[inputIndex],"DPTR",4)==0)
	{
		reg->type=R_DPTR;
		inputIndex+=4;
	}
	else if(strncasecmp(&line[inputIndex],"R",1)==0)
	{
		if(line[inputIndex+1]>='0'&&line[inputIndex+1]<='7')
		{
			reg->type=R_Rn;
			reg->value=line[inputIndex+1]-'0';
			inputIndex+=2;
		}
	}
	else if(strncasecmp(&line[inputIndex],"C",1)==0)
	{
		reg->type=R_CARRY;
		inputIndex+=1;
	}
	else if(strncasecmp(&line[inputIndex],"PC",2)==0)
	{
		reg->type=R_PC;
		inputIndex+=2;
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

static bool ParseIndirectOperand(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Parse an operand which is indirect register, or register combination:
// @Ri
// @DPTR
// @A+DPTR
// @A+PC
// return it in operand
// NOTE: if what was located does not look like an indirect operand return false
{
	unsigned int
		inputIndex;
	REGISTER
		reg;

	inputIndex=*lineIndex;
	if(line[inputIndex]=='@')	// make sure this looks indirect
	{
		inputIndex++;
		if(!ParseComment(line,&inputIndex))	// skip white space, make sure not at end of line
		{
			if(ParseRegister(line,&inputIndex,&reg))	// grab a register
			{
				switch(reg.type)
				{
					case R_A:
						if(!ParseComment(line,&inputIndex))	// skip white space, make sure not at end of line
						{
							if(line[inputIndex]=='+')		// need to see +
							{
								inputIndex++;
								if(!ParseComment(line,&inputIndex))	// skip white space, make sure not at end of line
								{
									if(ParseRegister(line,&inputIndex,&reg))	// grab a register
									{
										if(reg.type==R_DPTR)
										{
											operand->type=OT_INDIRECTAPLUSDPTR;
											*lineIndex=inputIndex;
											return(true);
										}
										else if(reg.type==R_PC)
										{
											operand->type=OT_INDIRECTAPLUSPC;
											*lineIndex=inputIndex;
											return(true);
										}
									}
								}
							}
						}
						break;
					case R_DPTR:
						operand->type=OT_INDIRECTDPTR;
						*lineIndex=inputIndex;
						return(true);
						break;
					case R_Rn:
						operand->type=OT_INDIRECT;
						operand->value=reg.value;
						*lineIndex=inputIndex;
						return(true);
						break;
				}
			}
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
	REGISTER
		reg;

	if(ParseIndirectOperand(line,lineIndex,operand))
	{
		return(true);
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
		else if(line[inputIndex]=='#')
		{
			inputIndex++;
			if(ParseExpression(line,&inputIndex,&value,&unresolved))
			{
				operand->type=OT_IMMEDIATE;
				operand->value=value;
				operand->unresolved=unresolved;
				*lineIndex=inputIndex;
				return(true);
			}
		}
		else if(ParseRegister(line,&inputIndex,&reg))	// does it look like a register?
		{
			switch(reg.type)
			{
				case R_A:
					operand->type=OT_A;
					*lineIndex=inputIndex;
					return(true);
					break;
				case R_AB:
					operand->type=OT_AB;
					*lineIndex=inputIndex;
					return(true);
					break;
				case R_DPTR:
					operand->type=OT_DPTR;
					*lineIndex=inputIndex;
					return(true);
					break;
				case R_Rn:
					operand->type=OT_REGISTER;
					operand->value=reg.value;
					*lineIndex=inputIndex;
					return(true);
					break;
				case R_CARRY:
					operand->type=OT_CARRY;
					*lineIndex=inputIndex;
					return(true);
					break;
			}
		}
		else if(ParseExpression(line,&inputIndex,&value,&unresolved))	// see if it looks like a value
		{
			operand->type=OT_VALUE;
			operand->value=value;
			operand->unresolved=unresolved;
			if(!ParseComment(line,&inputIndex))	// see if optionally followed by a '.' (to indicate bit offset)
			{
				if(line[inputIndex]=='.')
				{
					inputIndex++;
					operand->type=OT_VALUE_BIT;
					if(!ParseComment(line,&inputIndex))	// step over white space, make sure not at end
					{
						if(line[inputIndex]=='/')
						{
							inputIndex++;
							operand->type=OT_VALUE_NOTBIT;
						}
						if(ParseExpression(line,&inputIndex,&value,&unresolved))
						{
							operand->bit=value;
							operand->bitUnresolved=unresolved;
							*lineIndex=inputIndex;
							return(true);
						}
					}
				}
				else
				{
					*lineIndex=inputIndex;
					return(true);
				}
			}
			else
			{
				*lineIndex=inputIndex;
				return(true);
			}
		}
	}
	return(false);
}

static bool CheckIndirectRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 1 bit indirect register
// complain if not
{
	if(value>=0&&value<2)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Indirect register out of range\n");
	}
	return(false);
}

static bool CheckPageRelativeRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as an 11/19 bit relative offset
// complain if not
{
	if(value>=0&&value<(int)currentProcessor->pageSize)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Relative offset out of %dK page\n",currentProcessor->pageSize==SMALLPAGE?2:512);
	}
	return(false);
}

static bool CheckUnsignedLargeRange(int value,bool generateMessage,bool isError)
// see if value looks like a 24 bit quantity
// if not, return false, and generate a warning or error if told to
{
	if(value>=0&&value<LARGEMEMORY)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Unsigned large value (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckBitAddressRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as an 8 bit bit address
// complain if not
{
	if(value>=0&&value<256)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Bit address (0x%X) out of range\n",value);
	}
	return(false);
}

// opcode handling

static int MakeBitNumber(OPERAND *operand)
// Make operand (which must be OT_VALUE, OT_NOTVALUE, OT_VALUE_BIT or OT_VALUE_NOTBIT)
// into a bit number
// If the operand is out of range, complain.
// return the bit number
{
	int
		value;

	if(operand->type==OT_VALUE||operand->type==OT_NOTVALUE)
	{
		value=operand->value;
		CheckBitAddressRange(value,true,true);
	}
	else	// OT_VALUE_BIT, or OT_VALUE_NOTBIT
	{
		value=0;
		if(operand->bit>=0&&operand->bit<8)
		{
			if((operand->value>=0x20&&operand->value<0x30)||(operand->value>=0x80&&operand->value<0x100&&((operand->value&0x07)==0x00)))
			{
				if(operand->value<0x30)
				{
					value=(operand->value-0x20)*8+operand->bit;
				}
				else
				{
					value=operand->value+operand->bit;
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Bit address (0x%X) out of range\n",operand->value);
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"Bit index (0x%X) out of range\n",operand->bit);
		}
	}
	return(value);
}

static bool HandleAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *operand1,OPERAND *operand2,OPERAND *operand3,LISTING_RECORD *listingRecord)
// Given an addressing mode record, and a set of operands, generate code (or an error message if something is
// out of range)
// This will only return false on a 'hard' error
{
	bool
		fail;
	int
		value,
		value2;

	fail=false;
	switch(addressingMode->mode)
	{
		case AM_IMPLIED:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_A:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_AB:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_DPTR:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_REGISTER:
			fail=!GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord);
			break;
		case AM_INDIRECT:
			CheckIndirectRange(operand1->value,true,true);
			fail=!GenerateByte(addressingMode->baseOpcode|(operand1->value&1),listingRecord);
			break;
		case AM_DIRECT:
			CheckUnsignedByteRange(operand1->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(operand1->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_BIT:
			value=MakeBitNumber(operand1);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_ADDR_PAGE:
			value=0;
			if(currentSegment)
			{
				value=operand1->value-(((currentSegment->currentPC+currentSegment->codeGenOffset)+2)&(currentProcessor->memorySize-currentProcessor->pageSize));
				CheckPageRelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode|(currentProcessor->pageSize==SMALLPAGE?((value&0x700)>>3):((value&0x70000)>>11)),listingRecord))
			{
				if(currentProcessor->pageSize==LARGEPAGE)
				{
					fail=!GenerateByte((value&0xFF00)>>8,listingRecord);
				}
				if(!fail)
				{
					fail=!GenerateByte(value&0xFF,listingRecord);
				}
			}
			else
			{
				fail=true;
			}
			break;
		case AM_ADDR_ABSOLUTE:
			if(currentProcessor->memorySize==SMALLMEMORY)
			{
				CheckUnsignedWordRange(operand1->value,true,true);
			}
			else
			{
				CheckUnsignedLargeRange(operand1->value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if((currentProcessor->memorySize!=LARGEMEMORY)||GenerateByte((operand1->value&0xFF0000)>>16,listingRecord))
				{
					if(GenerateByte(operand1->value>>8,listingRecord))
					{
						fail=!GenerateByte(operand1->value&0xFF,listingRecord);
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
			break;
		case AM_CARRY:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value=operand1->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+2);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_INDIRECTAPLUSDPTR:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_A_REGISTER:
			fail=!GenerateByte(addressingMode->baseOpcode|operand2->value,listingRecord);
			break;
		case AM_A_INDIRECT:
			CheckIndirectRange(operand2->value,true,true);
			fail=!GenerateByte(addressingMode->baseOpcode|(operand2->value&1),listingRecord);
			break;
		case AM_A_DIRECT:
			CheckUnsignedByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_A_IMMEDIATE:
			CheckByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_A_INDIRECTDPTR:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_A_INDIRECTAPLUSDPTR:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_A_INDIRECTAPLUSPC:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_A_DIRECT_RELATIVE:
			CheckUnsignedByteRange(operand2->value,true,true);
			value=0;
			if(currentSegment)
			{
				value=operand3->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(operand2->value&0xFF,listingRecord))
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
			break;
		case AM_A_IMMEDIATE_RELATIVE:
			CheckByteRange(operand2->value,true,true);
			value=0;
			if(currentSegment)
			{
				value=operand3->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(operand2->value&0xFF,listingRecord))
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
			break;
		case AM_DPTR_IMMEDIATE:
			if(currentProcessor->memorySize==SMALLMEMORY)
			{
				CheckUnsignedWordRange(operand2->value,true,true);
			}
			else
			{
				CheckUnsignedLargeRange(operand2->value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if((currentProcessor->memorySize!=LARGEMEMORY)||GenerateByte((operand2->value&0xFF0000)>>16,listingRecord))
				{
					if(GenerateByte(operand2->value>>8,listingRecord))
					{
						fail=!GenerateByte(operand2->value&0xFF,listingRecord);
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
			break;
		case AM_REGISTER_A:
			fail=!GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord);
			break;
		case AM_REGISTER_DIRECT:
			CheckUnsignedByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_REGISTER_IMMEDIATE:
			CheckByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_REGISTER_RELATIVE:
			value=0;
			if(currentSegment)
			{
				value=operand2->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+2);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_REGISTER_IMMEDIATE_RELATIVE:
			CheckByteRange(operand2->value,true,true);
			value=0;
			if(currentSegment)
			{
				value=operand3->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode|operand1->value,listingRecord))
			{
				if(GenerateByte(operand2->value&0xFF,listingRecord))
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
			break;
		case AM_INDIRECT_A:
			CheckIndirectRange(operand1->value,true,true);
			fail=!GenerateByte(addressingMode->baseOpcode|(operand1->value&1),listingRecord);
			break;
		case AM_INDIRECT_DIRECT:
			CheckIndirectRange(operand1->value,true,true);
			CheckUnsignedByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|(operand1->value&1),listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_INDIRECT_IMMEDIATE:
			CheckIndirectRange(operand1->value,true,true);
			CheckByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|(operand1->value&1),listingRecord))
			{
				fail=!GenerateByte(operand2->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_INDIRECT_IMMEDIATE_RELATIVE:
			CheckIndirectRange(operand1->value,true,true);
			CheckByteRange(operand2->value,true,true);
			value=0;
			if(currentSegment)
			{
				value=operand3->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode|(operand1->value&1),listingRecord))
			{
				if(GenerateByte(operand2->value&0xFF,listingRecord))
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
			break;
		case AM_DIRECT_A:
			CheckUnsignedByteRange(operand1->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(operand1->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_DIRECT_REGISTER:
			CheckUnsignedByteRange(operand1->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|operand2->value,listingRecord))
			{
				fail=!GenerateByte(operand1->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_DIRECT_INDIRECT:
			CheckUnsignedByteRange(operand1->value,true,true);
			CheckIndirectRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode|(operand2->value&1),listingRecord))
			{
				fail=!GenerateByte(operand1->value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_DIRECT_DIRECT:	// @@@ Philips data sheet SEEMS to get src and dest backwards (this follows that data sheet)
			CheckUnsignedByteRange(operand1->value,true,true);
			CheckUnsignedByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(operand2->value&0xFF,listingRecord))
				{
					fail=!GenerateByte(operand1->value&0xFF,listingRecord);
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
			break;
		case AM_DIRECT_IMMEDIATE:
			CheckUnsignedByteRange(operand1->value,true,true);
			CheckByteRange(operand2->value,true,true);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(operand1->value&0xFF,listingRecord))
				{
					fail=!GenerateByte(operand2->value&0xFF,listingRecord);
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
			break;
		case AM_DIRECT_RELATIVE:
			CheckUnsignedByteRange(operand1->value,true,true);
			value=0;
			if(currentSegment)
			{
				value=operand2->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(operand1->value&0xFF,listingRecord))
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
			break;
		case AM_BIT_CARRY:
			value=MakeBitNumber(operand1);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_BIT_RELATIVE:
			value=MakeBitNumber(operand1);
			value2=0;
			if(currentSegment)
			{
				value2=operand2->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+3);
				Check8RelativeRange(value2,true,true);
			}
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				if(GenerateByte(value&0xFF,listingRecord))
				{
					fail=!GenerateByte(value2&0xFF,listingRecord);
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
			break;
		case AM_CARRY_BIT:
			value=MakeBitNumber(operand2);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_CARRY_NOTBIT:
			value=MakeBitNumber(operand2);
			if(GenerateByte(addressingMode->baseOpcode,listingRecord))
			{
				fail=!GenerateByte(value&0xFF,listingRecord);
			}
			else
			{
				fail=true;
			}
			break;
		case AM_INDIRECTDPTR_A:
			fail=!GenerateByte(addressingMode->baseOpcode,listingRecord);
			break;
	}
	return(!fail);
}

static bool OperandsMatchAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *operand1,OPERAND *operand2,OPERAND *operand3)
// See if the given addressing mode matches the passed operands
// return true for a match, false if no match
{
	switch(addressingMode->mode)
	{
		case AM_IMPLIED:
			return(numOperands==0);
			break;
		case AM_A:
			return((numOperands==1)&&(operand1->type==OT_A));
			break;
		case AM_AB:
			return((numOperands==1)&&(operand1->type==OT_AB));
			break;
		case AM_DPTR:
			return((numOperands==1)&&(operand1->type==OT_DPTR));
			break;
		case AM_REGISTER:
			return((numOperands==1)&&(operand1->type==OT_REGISTER));
			break;
		case AM_INDIRECT:
			return((numOperands==1)&&(operand1->type==OT_INDIRECT));
			break;
		case AM_DIRECT:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_BIT:
			return((numOperands==1)&&((operand1->type==OT_VALUE)||(operand1->type==OT_VALUE_BIT)));
			break;
		case AM_ADDR_PAGE:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_ADDR_ABSOLUTE:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_CARRY:
			return((numOperands==1)&&(operand1->type==OT_CARRY));
			break;
		case AM_RELATIVE:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_INDIRECTAPLUSDPTR:
			return((numOperands==1)&&(operand1->type==OT_INDIRECTAPLUSDPTR));
			break;
		case AM_A_REGISTER:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_REGISTER));
			break;
		case AM_A_INDIRECT:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_INDIRECT));
			break;
		case AM_A_DIRECT:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_VALUE));
			break;
		case AM_A_IMMEDIATE:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_IMMEDIATE));
			break;
		case AM_A_INDIRECTDPTR:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_INDIRECTDPTR));
			break;
		case AM_A_INDIRECTAPLUSDPTR:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_INDIRECTAPLUSDPTR));
			break;
		case AM_A_INDIRECTAPLUSPC:
			return((numOperands==2)&&(operand1->type==OT_A)&&(operand2->type==OT_INDIRECTAPLUSPC));
			break;
		case AM_A_DIRECT_RELATIVE:
			return((numOperands==3)&&(operand1->type==OT_A)&&(operand2->type==OT_VALUE)&&(operand3->type==OT_VALUE));
			break;
		case AM_A_IMMEDIATE_RELATIVE:
			return((numOperands==3)&&(operand1->type==OT_A)&&(operand2->type==OT_IMMEDIATE)&&(operand3->type==OT_VALUE));
			break;
		case AM_DPTR_IMMEDIATE:
			return((numOperands==2)&&(operand1->type==OT_DPTR)&&(operand2->type==OT_IMMEDIATE));
			break;
		case AM_REGISTER_A:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_A));
			break;
		case AM_REGISTER_DIRECT:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REGISTER_IMMEDIATE:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_IMMEDIATE));
			break;
		case AM_REGISTER_RELATIVE:
			return((numOperands==2)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_VALUE));
			break;
		case AM_REGISTER_IMMEDIATE_RELATIVE:
			return((numOperands==3)&&(operand1->type==OT_REGISTER)&&(operand2->type==OT_IMMEDIATE)&&(operand3->type==OT_VALUE));
			break;
		case AM_INDIRECT_A:
			return((numOperands==2)&&(operand1->type==OT_INDIRECT)&&(operand2->type==OT_A));
			break;
		case AM_INDIRECT_DIRECT:
			return((numOperands==2)&&(operand1->type==OT_INDIRECT)&&(operand2->type==OT_VALUE));
			break;
		case AM_INDIRECT_IMMEDIATE:
			return((numOperands==2)&&(operand1->type==OT_INDIRECT)&&(operand2->type==OT_IMMEDIATE));
			break;
		case AM_INDIRECT_IMMEDIATE_RELATIVE:
			return((numOperands==3)&&(operand1->type==OT_INDIRECT)&&(operand2->type==OT_IMMEDIATE)&&(operand3->type==OT_VALUE));
			break;
		case AM_DIRECT_A:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_A));
			break;
		case AM_DIRECT_REGISTER:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_REGISTER));
			break;
		case AM_DIRECT_INDIRECT:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_INDIRECT));
			break;
		case AM_DIRECT_DIRECT:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_VALUE));
			break;
		case AM_DIRECT_IMMEDIATE:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_IMMEDIATE));
			break;
		case AM_DIRECT_RELATIVE:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_VALUE));
			break;
		case AM_BIT_CARRY:
			return((numOperands==2)&&((operand1->type==OT_VALUE)||(operand1->type==OT_VALUE_BIT))&&(operand2->type==OT_CARRY));
			break;
		case AM_BIT_RELATIVE:
			return((numOperands==2)&&((operand1->type==OT_VALUE)||(operand1->type==OT_VALUE_BIT))&&(operand2->type==OT_VALUE));
			break;
		case AM_CARRY_BIT:
			return((numOperands==2)&&(operand1->type==OT_CARRY)&&((operand2->type==OT_VALUE)||(operand2->type==OT_VALUE_BIT)));
			break;
		case AM_CARRY_NOTBIT:
			return((numOperands==2)&&(operand1->type==OT_CARRY)&&((operand2->type==OT_NOTVALUE)||(operand2->type==OT_VALUE_NOTBIT)));
			break;
		case AM_INDIRECTDPTR_A:
			return((numOperands==2)&&(operand1->type==OT_INDIRECTDPTR)&&(operand2->type==OT_A));
			break;
	}
	return(false);
}

static bool ParseOperands(const char *line,unsigned int *lineIndex,unsigned int *numOperands,OPERAND *operand1,OPERAND *operand2,OPERAND *operand3)
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
							if(!ParseComment(line,lineIndex))	// make sure not at end of line
							{
								if(ParseCommaSeparator(line,lineIndex))
								{
									if(!ParseComment(line,lineIndex))	// make sure not at end of line
									{
										if(ParseOperand(line,lineIndex,operand3))
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
	return(!fail);
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
		numOperands;
	OPERAND
		operand1,
		operand2,
		operand3;
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
			if(ParseOperands(line,lineIndex,&numOperands,&operand1,&operand2,&operand3))	// fetch operands for opcode
			{
				done=false;
				for(i=0;!done&&(i<opcode->numModes);i++)
				{
					if(OperandsMatchAddressingMode(&(opcode->addressingModes[i]),numOperands,&operand1,&operand2,&operand3))
					{
						result=HandleAddressingMode(&(opcode->addressingModes[i]),numOperands,&operand1,&operand2,&operand3,listingRecord);
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
	currentProcessor=(PROCESSOR_DATA *)processor->processorData;	// remember information about which processor has been selected
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
				STDisposeSymbolTable(opcodeSymbols);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

static const PROCESSOR_DATA
	up8031		={SMALLPAGE,SMALLMEMORY},
	up8032		={SMALLPAGE,SMALLMEMORY},
	up8051		={SMALLPAGE,SMALLMEMORY},
	up8052		={SMALLPAGE,SMALLMEMORY},
	up80c390	={LARGEPAGE,LARGEMEMORY};

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Intel 8051",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"8031",&up8031),
		PROCESSOR(&processorFamily,"8032",&up8032),
		PROCESSOR(&processorFamily,"8051",&up8051),
		PROCESSOR(&processorFamily,"8052",&up8052),
		PROCESSOR(&processorFamily,"80c390",&up80c390),
	};
