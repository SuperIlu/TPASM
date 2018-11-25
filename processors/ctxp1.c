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


// Generate code for the Core Technologies Xilinx Processor - 1

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols;

struct ADDRESSING_MODE
{
	unsigned int
		mode;
	unsigned short
		baseOpcode;
};

struct OPERAND
{
	unsigned int
		type;			// type of operand located
	int
		value;			// immediate value, absolute address, register index
	bool
		unresolved;		// if value was an expression, this tells if it was unresolved
};

// enumerated operand types (used when parsing)

enum
{
	OT_REG,							// reg
	OT_IREG,						// [reg]
	OT_C,							// C flag
	OT_IMM,							// #value
	OT_VALUE,						// value
};

// enumerated addressing modes

enum
{
	AM_IMPLIED,						// <no operands>
	AM_REG_REG,						// reg,reg
	AM_REG_IMM4,					// reg,#value
	AM_REG_IMM8,					// reg,#value
	AM_ADDR12,						// 12 bit address
	AM_REG_ADDR8,					// reg,address
	AM_ADDR8_REG,					// address,reg
	AM_REG_IREG,					// reg,[reg]
	AM_IREG_REG,					// [reg],reg
	AM_REGA,						// reg (in A slot)
	AM_REGB,						// reg (in B slot)
	AM_IMM4,						// #value
	AM_REL8,						// 8 bit relative offset
	AM_C,							// C
	AM_C_IMM1,						// C,#value
	AM_C_REG_IMM4,					// C,reg,#value
	AM_C_REG_REG,					// C,reg,reg
	AM_REG_IMM4_IMM1,				// reg,#value,#value
	AM_REG_IMM4_C,					// reg,#value,C
	AM_REG_REG_IMM1,				// reg,reg,#value
	AM_REG_REG_C,					// reg,reg,C
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

#define	MODES(modeArray)	elementsof(modeArray),&modeArray[0]

static ADDRESSING_MODE
	M_ADD[]=
	{
		{AM_REG_REG,					0x0000},
		{AM_REG_IMM4,					0x2000},
	},
	M_ADDW[]=
	{
		{AM_REG_REG,					0x1000},
		{AM_REG_IMM4,					0x3000},
	},
	M_ADDC[]=
	{
		{AM_REG_REG,					0x0100},
		{AM_REG_IMM4,					0x2100},
	},
	M_ADDCW[]=
	{
		{AM_REG_REG,					0x1100},
		{AM_REG_IMM4,					0x3100},
	},
	M_SUB[]=
	{
		{AM_REG_REG,					0x0200},
		{AM_REG_IMM4,					0x2200},
	},
	M_SUBW[]=
	{
		{AM_REG_REG,					0x1200},
		{AM_REG_IMM4,					0x3200},
	},
	M_SUBC[]=
	{
		{AM_REG_REG,					0x0300},
		{AM_REG_IMM4,					0x2300},
	},
	M_SUBCW[]=
	{
		{AM_REG_REG,					0x1300},
		{AM_REG_IMM4,					0x3300},
	},
	M_CMPU[]=
	{
		{AM_REG_REG,					0x0400},
		{AM_REG_IMM4,					0x2400},
	},
	M_CMPUW[]=
	{
		{AM_REG_REG,					0x1400},
		{AM_REG_IMM4,					0x3400},
	},
	M_CMPS[]=
	{
		{AM_REG_REG,					0x0500},
		{AM_REG_IMM4,					0x2500},
	},
	M_CMPSW[]=
	{
		{AM_REG_REG,					0x1500},
		{AM_REG_IMM4,					0x3500},
	},
	M_CMPE[]=
	{
		{AM_REG_REG,					0x0600},
		{AM_REG_IMM4,					0x2600},
	},
	M_CMPEW[]=
	{
		{AM_REG_REG,					0x1600},
		{AM_REG_IMM4,					0x3600},
	},
	M_CMPP[]=
	{
		{AM_REG_REG,					0x0700},
		{AM_REG_IMM4,					0x2700},
	},
	M_CMPPW[]=
	{
		{AM_REG_REG,					0x1700},
		{AM_REG_IMM4,					0x3700},
	},
	M_ROL[]=
	{
		{AM_REG_REG,					0x0800},
		{AM_REG_IMM4,					0x2800},
	},
	M_ROLW[]=
	{
		{AM_REG_REG,					0x1800},
		{AM_REG_IMM4,					0x3800},
	},
	M_ROLC[]=
	{
		{AM_REG_REG,					0x0900},
		{AM_REG_IMM4,					0x2900},
	},
	M_ROLCW[]=
	{
		{AM_REG_REG,					0x1900},
		{AM_REG_IMM4,					0x3900},
	},
	M_ROL0[]=
	{
		{AM_REG_REG,					0x0A00},
		{AM_REG_IMM4,					0x2A00},
	},
	M_ROL0W[]=
	{
		{AM_REG_REG,					0x1A00},
		{AM_REG_IMM4,					0x3A00},
	},
	M_ROLS[]=
	{
		{AM_REG_REG,					0x0B00},
		{AM_REG_IMM4,					0x2B00},
	},
	M_ROLSW[]=
	{
		{AM_REG_REG,					0x1B00},
		{AM_REG_IMM4,					0x3B00},
	},
	M_ROR[]=
	{
		{AM_REG_REG,					0x0C00},
		{AM_REG_IMM4,					0x2C00},
	},
	M_RORW[]=
	{
		{AM_REG_REG,					0x1C00},
		{AM_REG_IMM4,					0x3C00},
	},
	M_RORC[]=
	{
		{AM_REG_REG,					0x0D00},
		{AM_REG_IMM4,					0x2D00},
	},
	M_RORCW[]=
	{
		{AM_REG_REG,					0x1D00},
		{AM_REG_IMM4,					0x3D00},
	},
	M_ROR0[]=
	{
		{AM_REG_REG,					0x0E00},
		{AM_REG_IMM4,					0x2E00},
	},
	M_ROR0W[]=
	{
		{AM_REG_REG,					0x1E00},
		{AM_REG_IMM4,					0x3E00},
	},
	M_RORS[]=
	{
		{AM_REG_REG,					0x0F00},
		{AM_REG_IMM4,					0x2F00},
	},
	M_RORSW[]=
	{
		{AM_REG_REG,					0x1F00},
		{AM_REG_IMM4,					0x3F00},
	},
	M_NOT[]=
	{
		{AM_REG_REG,					0x4000},
		{AM_REG_IMM4,					0x4400},
	},
	M_NOTW[]=
	{
		{AM_REG_REG,					0x5000},
		{AM_REG_IMM4,					0x5400},
	},
	M_AND[]=
	{
		{AM_REG_REG,					0x4100},
		{AM_REG_IMM4,					0x4500},
	},
	M_ANDW[]=
	{
		{AM_REG_REG,					0x5100},
		{AM_REG_IMM4,					0x5500},
	},
	M_OR[]=
	{
		{AM_REG_REG,					0x4200},
		{AM_REG_IMM4,					0x4600},
	},
	M_ORW[]=
	{
		{AM_REG_REG,					0x5200},
		{AM_REG_IMM4,					0x5600},
	},
	M_XOR[]=
	{
		{AM_REG_REG,					0x4300},
		{AM_REG_IMM4,					0x4700},
	},
	M_XORW[]=
	{
		{AM_REG_REG,					0x5300},
		{AM_REG_IMM4,					0x5700},
	},
	M_BLD[]=
	{
		{AM_C_IMM1,						0x6800},
		{AM_C_REG_IMM4,					0x7A00},
		{AM_C_REG_REG,					0x6A00},
		{AM_REG_IMM4_IMM1,				0x7000},
		{AM_REG_IMM4_C,					0x7200},
		{AM_REG_REG_IMM1,				0x6000},
		{AM_REG_REG_C,					0x6200},
	},
	M_BNOT[]=
	{
		{AM_C,							0x6B00},
		{AM_C_REG_IMM4,					0x7C00},
		{AM_C_REG_REG,					0x6C00},
		{AM_REG_IMM4,					0x7300},
		{AM_REG_REG,					0x6300},
		{AM_REG_IMM4_C,					0x7400},
		{AM_REG_REG_C,					0x6400},
	},
	M_BAND[]=
	{
		{AM_C_REG_IMM4,					0x7D00},
		{AM_C_REG_REG,					0x6D00},
		{AM_REG_IMM4_C,					0x7500},
		{AM_REG_REG_C,					0x6500},
	},
	M_BOR[]=
	{
		{AM_C_REG_IMM4,					0x7E00},
		{AM_C_REG_REG,					0x6E00},
		{AM_REG_IMM4_C,					0x7600},
		{AM_REG_REG_C,					0x6600},
	},
	M_BXOR[]=
	{
		{AM_C_REG_IMM4,					0x7F00},
		{AM_C_REG_REG,					0x6F00},
		{AM_REG_IMM4_C,					0x7700},
		{AM_REG_REG_C,					0x6700},
	},
	M_LD[]=
	{
		{AM_REG_REG,					0x4800},
		{AM_REG_IMM8,					0xE000},
	},
	M_LDW[]=
	{
		{AM_REG_REG,					0x5800},
	},
	M_LDP[]=
	{
		{AM_REG_IREG,					0xF000},
		{AM_IREG_REG,					0xF200},
		{AM_REG_ADDR8,					0x8000},
		{AM_ADDR8_REG,					0xA000},
	},
	M_LDPW[]=
	{
		{AM_REG_IREG,					0xF100},
		{AM_IREG_REG,					0xF300},
		{AM_REG_ADDR8,					0x9000},
		{AM_ADDR8_REG,					0xB000},
	},
	M_LDX[]=
	{
		{AM_REG_IREG,					0xF400},
		{AM_IREG_REG,					0xF600},
	},
	M_LDCW[]=
	{
		{AM_REG_IREG,					0xF500},
		{AM_IREG_REG,					0xF700},
	},
	M_JUMP[]=
	{
		{AM_REG_REG,					0xFC00},
		{AM_ADDR12,						0xC000},
	},
	M_CALL[]=
	{
		{AM_REG_REG,					0xFD00},
		{AM_ADDR12,						0xD000},
	},
	M_RET[]=
	{
		{AM_IMPLIED,					0xFFD0},
	},
	M_RETCC[]=
	{
		{AM_IMPLIED,					0xFFE0},
	},
	M_RETCS[]=
	{
		{AM_IMPLIED,					0xFFF0},
	},
	M_BCC[]=
	{
		{AM_REL8,						0xFA00},
	},
	M_BCS[]=
	{
		{AM_REL8,						0xFB00},
	},
	M_SKIP[]=
	{
		{AM_IMPLIED,					0xFFD1},
	},
	M_SKIPCC[]=
	{
		{AM_IMPLIED,					0xFFE1},
	},
	M_SKIPCS[]=
	{
		{AM_IMPLIED,					0xFFF1},
	},
	M_DISPATCH[]=
	{
		{AM_IMPLIED,					0xFFD2},
	},
	M_DISPATCHCC[]=
	{
		{AM_IMPLIED,					0xFFE2},
	},
	M_DISPATCHCS[]=
	{
		{AM_IMPLIED,					0xFFF2},
	},
	M_CONTROL[]=
	{
		{AM_REGB,						0xF900},
		{AM_IMM4,						0xF908},
	},
	M_STATUS[]=
	{
		{AM_REGA,						0xF800},
	},
	M_NOP[]=
	{
		{AM_IMPLIED,					0x4100},	// and r0,r0
	};

static bool HandleCTXP1DW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"ds",			HandleDS},
		{"dw",			HandleCTXP1DW},
	};

static OPCODE
	Opcodes[]=
	{
		{"add",			MODES(M_ADD)		},
		{"addw",		MODES(M_ADDW)		},
		{"addc",		MODES(M_ADDC)		},
		{"addcw",		MODES(M_ADDCW)		},
		{"sub",			MODES(M_SUB)		},
		{"subw",		MODES(M_SUBW)		},
		{"subc",		MODES(M_SUBC)		},
		{"subcw",		MODES(M_SUBCW)		},
		{"cmpu",		MODES(M_CMPU)		},
		{"cmpuw",		MODES(M_CMPUW)		},
		{"cmps",		MODES(M_CMPS)		},
		{"cmpsw",		MODES(M_CMPSW)		},
		{"cmpe",		MODES(M_CMPE)		},
		{"cmpew",		MODES(M_CMPEW)		},
		{"cmpp",		MODES(M_CMPP)		},
		{"cmppw",		MODES(M_CMPPW)		},
		{"rol",			MODES(M_ROL)		},
		{"rolw",		MODES(M_ROLW)		},
		{"rolc",		MODES(M_ROLC)		},
		{"rolcw",		MODES(M_ROLCW)		},
		{"rol0",		MODES(M_ROL0)		},
		{"rol0w",		MODES(M_ROL0W)		},
		{"rols",		MODES(M_ROLS)		},
		{"rolsw",		MODES(M_ROLSW)		},
		{"ror",			MODES(M_ROR)		},
		{"rorw",		MODES(M_RORW)		},
		{"rorc",		MODES(M_RORC)		},
		{"rorcw",		MODES(M_RORCW)		},
		{"ror0",		MODES(M_ROR0)		},
		{"ror0w",		MODES(M_ROR0W)		},
		{"rors",		MODES(M_RORS)		},
		{"rorsw",		MODES(M_RORSW)		},
		{"not",			MODES(M_NOT)		},
		{"notw",		MODES(M_NOTW)		},
		{"and",			MODES(M_AND)		},
		{"andw",		MODES(M_ANDW)		},
		{"or",			MODES(M_OR)			},
		{"orw",			MODES(M_ORW)		},
		{"xor",			MODES(M_XOR)		},
		{"xorw",		MODES(M_XORW)		},
		{"bld",			MODES(M_BLD)		},
		{"bnot",		MODES(M_BNOT)		},
		{"band",		MODES(M_BAND)		},
		{"bor",			MODES(M_BOR)		},
		{"bxor",		MODES(M_BXOR)		},
		{"ld",			MODES(M_LD)			},
		{"ldw",			MODES(M_LDW)		},
		{"ldp",			MODES(M_LDP)		},
		{"ldpw",		MODES(M_LDPW)		},
		{"ldx",			MODES(M_LDX)		},
		{"ldcw",		MODES(M_LDCW)		},
		{"jump",		MODES(M_JUMP)		},
		{"call",		MODES(M_CALL)		},
		{"ret",			MODES(M_RET)		},
		{"retcc",		MODES(M_RETCC)		},
		{"retcs",		MODES(M_RETCS)		},
		{"bcs",			MODES(M_BCS)		},
		{"bcc",			MODES(M_BCC)		},
		{"skip",		MODES(M_SKIP)		},
		{"skipcc",		MODES(M_SKIPCC)		},
		{"skipcs",		MODES(M_SKIPCS)		},
		{"dispatch",	MODES(M_DISPATCH)	},
		{"dispatchcc",	MODES(M_DISPATCHCC)	},
		{"dispatchcs",	MODES(M_DISPATCHCS)	},
		{"control",		MODES(M_CONTROL)	},
		{"status",		MODES(M_STATUS)		},
		{"nop",			MODES(M_NOP)		},
	};

static bool GenerateCTXP1Word(int wordValue,LISTING_RECORD *listingRecord)
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
		if(currentSegment->currentPC+currentSegment->codeGenOffset<0x1000)	// check to make sure it fits
		{
			if(!intermediatePass)				// only do the real work if necessary
			{
				length=strlen(listingRecord->listObjectString);
				if(length+6<MAX_STRING)
				{
					sprintf(&listingRecord->listObjectString[length],"%04X ",wordValue);		// create list file output
				}
				outputBytes[0]=wordValue>>8;		// data is written as big endian words
				outputBytes[1]=wordValue&0xFF;

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

static bool ParseRegister(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Try to parse out what looks like a register
// This will match:
// R0 through R15
// or
// r0 through r15
// NOTE: if what was located does not look like a register return false
{
	unsigned int
		inputIndex;
	unsigned int
		registerIndex;

	inputIndex=*lineIndex;
	if((line[inputIndex]=='R')||(line[inputIndex]=='r'))
	{
		inputIndex++;
		registerIndex=0;
		while(line[inputIndex]>='0'&&line[inputIndex]<='9')
		{
			registerIndex*=10;
			registerIndex+=line[inputIndex]-'0';
			inputIndex++;
		}
		if((inputIndex-*lineIndex>1)&&(registerIndex<=15))		// make we got something, and it is in range
		{
			if(!IsLabelChar(line[inputIndex]))	// make sure the character following this does not look like part of a label
			{
				operand->type=OT_REG;
				operand->value=registerIndex;
				operand->unresolved=false;
				*lineIndex=inputIndex;				// push past the register
				return(true);
			}
		}
	}
	return(false);
}

static bool ParseIndirectRegister(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Parse an operand which is an indirect register:
// [reg]
// NOTE: if what was located does not look like an indirect register return false
{
	unsigned int
		inputIndex;

	inputIndex=*lineIndex;
	if(line[inputIndex]=='[')	// make sure this looks indirect
	{
		inputIndex++;
		if(!ParseComment(line,&inputIndex))	// skip white space, make sure not at end of line
		{
			if(ParseRegister(line,&inputIndex,operand))	// grab a register
			{
				if(!ParseComment(line,&inputIndex))	// skip white space, make sure not at end of line
				{
					if(line[inputIndex]==']')
					{
						operand->type=OT_IREG;		// change type to indirect
						*lineIndex=inputIndex+1;
						return(true);
					}
				}
			}
		}
	}
	return(false);
}

static bool ParseCFlag(const char *line,unsigned int *lineIndex,OPERAND *operand)
// Try to parse out what looks like the C flag
// This will match:
// C through c
// NOTE: if what was located does not look like a C flag return false
{
	unsigned int
		inputIndex;

	inputIndex=*lineIndex;
	if((line[inputIndex]=='C')||(line[inputIndex]=='c'))
	{
		inputIndex++;
		if(!IsLabelChar(line[inputIndex]))	// make sure the character following this does not look like part of a label
		{
			operand->type=OT_C;
			operand->value=0;
			operand->unresolved=false;
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

	if(ParseRegister(line,lineIndex,operand))
	{
		return(true);
	}
	else if(ParseIndirectRegister(line,lineIndex,operand))
	{
		return(true);
	}
	else if(ParseCFlag(line,lineIndex,operand))
	{
		return(true);
	}
	else
	{
		inputIndex=*lineIndex;
		if(line[inputIndex]=='#')	// immediate value?
		{
			inputIndex++;
			if(ParseExpression(line,&inputIndex,&value,&unresolved))
			{
				operand->type=OT_IMM;
				operand->value=value;
				operand->unresolved=unresolved;
				*lineIndex=inputIndex;
				return(true);
			}
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

// opcode and processor-specific pseudo opcode handling

static bool HandleCTXP1DW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring words of data (making up instructions)
{
	int
		value;
	bool
		unresolved;
	bool
		done,
		fail;

	done=false;
	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	while(!done&&!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckWordRange(value,true,true);
			if(!GenerateCTXP1Word(value,listingRecord))
			{
				fail=true;
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

static bool CheckImmediate1(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 1 bit immediate
// complain if not
{
	if(value>=0&&value<2)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"1 bit immediate value (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckImmediate4(int value,bool generateMessage,bool isError)
// check value to see if it looks good as an unsigned 4 bit immediate
// complain if not
{
	if(value>=0&&value<16)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"4 bit immediate value (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress8(int value,bool generateMessage,bool isError)
// check value to see if it looks good as an unsigned 8 bit address
// complain if not
{
	if((value>=0)&&(value<0x100))
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"8 bit address (0x%X) out of range\n",value);
	}
	return(false);
}

static bool CheckAddress12(int value,bool generateMessage,bool isError)
// check value to see if it looks good as an unsigned 12 bit address
// complain if not
{
	if((value>=0)&&(value<0x1000))
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"12 bit address (0x%X) out of range\n",value);
	}
	return(false);
}

// opcode handling

static bool HandleAddressingMode(ADDRESSING_MODE *addressingMode,unsigned int numOperands,OPERAND *operand1,OPERAND *operand2,OPERAND *operand3,LISTING_RECORD *listingRecord)
// Given an addressing mode record, and a set of operands, generate code (or an error message if something is
// out of range)
// This will only return false on a 'hard' error
{
	bool
		fail;
	int
		value;

	fail=false;
	switch(addressingMode->mode)
	{
		case AM_IMPLIED:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_REG_REG:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand2->value<<4)|operand1->value,listingRecord);
			break;
		case AM_REG_IMM4:
			CheckImmediate4(operand2->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&0x0F)<<4)|operand1->value,listingRecord);
			break;
		case AM_REG_IMM8:
			CheckByteRange(operand2->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&0xFF)<<4)|operand1->value,listingRecord);
			break;
		case AM_ADDR12:
			CheckAddress12(operand1->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand1->value&0x0FFF),listingRecord);
			break;
		case AM_REG_ADDR8:
			CheckAddress8(operand2->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&0xFF)<<4)|operand1->value,listingRecord);
			break;
		case AM_ADDR8_REG:
			CheckAddress8(operand1->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand1->value&0xFF)<<4)|operand2->value,listingRecord);
			break;
		case AM_REG_IREG:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand2->value<<4)|operand1->value,listingRecord);
			break;
		case AM_IREG_REG:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand1->value<<4)|operand2->value,listingRecord);
			break;
		case AM_REGA:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|operand1->value,listingRecord);
			break;
		case AM_REGB:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand1->value<<4),listingRecord);
			break;
		case AM_IMM4:
			CheckImmediate4(operand1->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand1->value<<4),listingRecord);
			break;
		case AM_REL8:
			value=0;
			if(currentSegment)
			{
				value=operand1->value-((currentSegment->currentPC+currentSegment->codeGenOffset)+1);
				Check8RelativeRange(value,true,true);
			}
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(value&0xFF),listingRecord);
			break;
		case AM_C:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode,listingRecord);
			break;
		case AM_C_IMM1:
			CheckImmediate1(operand2->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&1)<<8),listingRecord);
			break;
		case AM_C_REG_IMM4:
			CheckImmediate4(operand3->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand3->value&0x0F)<<4)|operand2->value,listingRecord);
			break;
		case AM_C_REG_REG:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand3->value<<4)|operand2->value,listingRecord);
			break;
		case AM_REG_IMM4_IMM1:
			CheckImmediate4(operand2->value,true,true);
			CheckImmediate1(operand3->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&0x0F)<<4)|operand1->value|((operand3->value&1)<<8),listingRecord);
			break;
		case AM_REG_IMM4_C:
			CheckImmediate4(operand2->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|((operand2->value&0x0F)<<4)|operand1->value,listingRecord);
			break;
		case AM_REG_REG_IMM1:
			CheckImmediate1(operand3->value,true,true);
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand2->value<<4)|operand1->value|((operand3->value&1)<<8),listingRecord);
			break;
		case AM_REG_REG_C:
			fail=!GenerateCTXP1Word(addressingMode->baseOpcode|(operand2->value<<4)|operand1->value,listingRecord);
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
		case AM_REG_REG:
			return((numOperands==2)&&(operand1->type==OT_REG)&&(operand2->type==OT_REG));
			break;
		case AM_REG_IMM4:
			return((numOperands==2)&&(operand1->type==OT_REG)&&(operand2->type==OT_IMM));
			break;
		case AM_REG_IMM8:
			return((numOperands==2)&&(operand1->type==OT_REG)&&(operand2->type==OT_IMM));
			break;
		case AM_ADDR12:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_REG_ADDR8:
			return((numOperands==2)&&(operand1->type==OT_REG)&&(operand2->type==OT_VALUE));
			break;
		case AM_ADDR8_REG:
			return((numOperands==2)&&(operand1->type==OT_VALUE)&&(operand2->type==OT_REG));
			break;
		case AM_REG_IREG:
			return((numOperands==2)&&(operand1->type==OT_REG)&&(operand2->type==OT_IREG));
			break;
		case AM_IREG_REG:
			return((numOperands==2)&&(operand1->type==OT_IREG)&&(operand2->type==OT_REG));
			break;
		case AM_REGA:
			return((numOperands==1)&&(operand1->type==OT_REG));
			break;
		case AM_REGB:
			return((numOperands==1)&&(operand1->type==OT_REG));
			break;
		case AM_IMM4:
			return((numOperands==1)&&(operand1->type==OT_IMM));
			break;
		case AM_REL8:
			return((numOperands==1)&&(operand1->type==OT_VALUE));
			break;
		case AM_C:
			return((numOperands==1)&&(operand1->type==OT_C));
			break;
		case AM_C_IMM1:
			return((numOperands==2)&&(operand1->type==OT_C)&&(operand2->type==OT_IMM));
			break;
		case AM_C_REG_IMM4:
			return((numOperands==3)&&(operand1->type==OT_C)&&(operand2->type==OT_REG)&&(operand3->type==OT_IMM));
			break;
		case AM_C_REG_REG:
			return((numOperands==3)&&(operand1->type==OT_C)&&(operand2->type==OT_REG)&&(operand3->type==OT_REG));
			break;
		case AM_REG_IMM4_IMM1:
			return((numOperands==3)&&(operand1->type==OT_REG)&&(operand2->type==OT_IMM)&&(operand3->type==OT_IMM));
			break;
		case AM_REG_IMM4_C:
			return((numOperands==3)&&(operand1->type==OT_REG)&&(operand2->type==OT_IMM)&&(operand3->type==OT_C));
			break;
		case AM_REG_REG_IMM1:
			return((numOperands==3)&&(operand1->type==OT_REG)&&(operand2->type==OT_REG)&&(operand3->type==OT_IMM));
			break;
		case AM_REG_REG_C:
			return((numOperands==3)&&(operand1->type==OT_REG)&&(operand2->type==OT_REG)&&(operand3->type==OT_C));
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

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Core Technologies CTXP-1",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"ctxp1",NULL),
	};
