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


// Generate code for Sunplus SPCxxxx (crippled 6502 core)

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,
	*opcodeSymbols;

// enumerated addressing modes

#define	OT_IMPLIED				0								// no operands
#define	OT_IMMEDIATE			1								// #xx
#define	OT_ZEROPAGE				2								// xx
#define	OT_ZEROPAGE_OFF_X		3								// xx,X
#define	OT_EXTENDED				4								// xxxx
#define	OT_EXTENDED_OFF_X		5								// xxxx,X
#define	OT_INDIRECT_OFF_X		6								// (xx,X)
#define	OT_INDIRECT_WORD		7								// (xxxx)
#define	OT_RELATIVE				8								// one byte relative offset

#define	OT_NUM					OT_RELATIVE+1					// number of addressing modes

// masks for the various addressing modes

#define	M_IMPLIED				(1<<OT_IMPLIED)
#define	M_IMMEDIATE				(1<<OT_IMMEDIATE)
#define	M_ZEROPAGE				(1<<OT_ZEROPAGE)
#define	M_ZEROPAGE_OFF_X		(1<<OT_ZEROPAGE_OFF_X)
#define	M_EXTENDED				(1<<OT_EXTENDED)
#define	M_EXTENDED_OFF_X		(1<<OT_EXTENDED_OFF_X)
#define	M_INDIRECT_OFF_X		(1<<OT_INDIRECT_OFF_X)
#define	M_INDIRECT_WORD			(1<<OT_INDIRECT_WORD)
#define	M_RELATIVE				(1<<OT_RELATIVE)

struct OPCODE
{
	const char
		*name;
	unsigned int
		typeMask;
	unsigned char
		baseOpcode[OT_NUM];
};

// prototypes for the handler functions which need to be declared before we can build
// the opcode table

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

#define	____		0					// unused opcode

// differences between this and a 6502:
//	no Y register
//	no asl/lsr instructions
//	no decimal flag (so no cld/sed instructions)
//	many instructions have fewer supported address modes
//	opcodes are all different (except for brk = 0x00)
//

static OPCODE
	Opcodes[]=
	{
//																											imp   imm  zp   zpx  ext  extx indx indw rel
		{"adc",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x56,0x17,____,____,____,____,____,____}},
		{"and",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x54,0x15,____,____,____,____,____,____}},
		{"bcc",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x28}},
		{"bcs",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x38}},
		{"beq",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x3a}},
		{"bit",		M_ZEROPAGE|M_EXTENDED,																	{____,____,0x11,____,0x51,____,____,____,____}},
		{"bmi",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x18}},
		{"bne",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x2a}},
		{"bpl",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x08}},
		{"brk",		M_IMPLIED,																				{0x00,____,____,____,____,____,____,____,____}},
		{"bvc",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x0a}},
		{"bvs",		M_RELATIVE,																				{____,____,____,____,____,____,____,____,0x1a}},
		{"clc",		M_IMPLIED,																				{0x48,____,____,____,____,____,____,____,____}},
		{"cli",		M_IMPLIED,																				{0x4a,____,____,____,____,____,____,____,____}},
		{"clv",		M_IMPLIED,																				{0x78,____,____,____,____,____,____,____,____}},
		{"cmp",		M_IMMEDIATE|M_ZEROPAGE|M_ZEROPAGE_OFF_X,												{____,0x66,0x27,0x2f,____,____,____,____,____}},
		{"cpx",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x32,0x33,____,____,____,____,____,____}},
		{"dec",		M_ZEROPAGE|M_ZEROPAGE_OFF_X,															{____,____,0xa3,0xab,____,____,____,____,____}},
		{"dex",		M_IMPLIED,																				{0xe2,____,____,____,____,____,____,____,____}},
		{"eor",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x46,0x07,____,____,____,____,____,____}},
		{"inc",		M_ZEROPAGE,																				{____,____,0xb3,____,____,____,____,____,____}},
		{"inx",		M_IMPLIED,																				{0x72,____,____,____,____,____,____,____,____}},
		{"jmp",		M_EXTENDED|M_INDIRECT_WORD,																{____,____,____,____,0x43,____,____,0x53,____}},
		{"jsr",		M_EXTENDED,																				{____,____,____,____,0x10,____,____,____,____}},
		{"lda",		M_IMMEDIATE|M_ZEROPAGE|M_ZEROPAGE_OFF_X|M_EXTENDED|M_EXTENDED_OFF_X|M_INDIRECT_OFF_X,	{____,0x74,0x35,0x3d,0x75,0x7d,0x34,____,____}},
		{"ldx",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0xb0,0xb1,____,____,____,____,____,____}},
		{"nop",		M_IMPLIED,																				{0xf2,____,____,____,____,____,____,____,____}},
		{"ora",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x44,0x05,____,____,____,____,____,____}},
		{"pha",		M_IMPLIED,																				{0x42,____,____,____,____,____,____,____,____}},
		{"php",		M_IMPLIED,																				{0x40,____,____,____,____,____,____,____,____}},
		{"pla",		M_IMPLIED,																				{0x52,____,____,____,____,____,____,____,____}},
		{"plp",		M_IMPLIED,																				{0x50,____,____,____,____,____,____,____,____}},
		{"rol",		M_IMPLIED|M_ZEROPAGE,																	{0xd0,____,0x91,____,____,____,____,____,____}},
		{"ror",		M_IMPLIED|M_ZEROPAGE,																	{0xd2,____,0x93,____,____,____,____,____,____}},
		{"rti",		M_IMPLIED,																				{0x02,____,____,____,____,____,____,____,____}},
		{"rts",		M_IMPLIED,																				{0x12,____,____,____,____,____,____,____,____}},
		{"sbc",		M_IMMEDIATE|M_ZEROPAGE,																	{____,0x76,0x37,____,____,____,____,____,____}},
		{"sec",		M_IMPLIED,																				{0x58,____,____,____,____,____,____,____,____}},
		{"sei",		M_IMPLIED,																				{0x5a,____,____,____,____,____,____,____,____}},
		{"sta",		M_ZEROPAGE|M_ZEROPAGE_OFF_X|M_INDIRECT_OFF_X,											{____,____,0x25,0x2d,____,____,0x24,____,____}},
		{"stx",		M_ZEROPAGE|M_EXTENDED,																	{____,____,0xa1,____,0xe1,____,____,____,____}},
		{"tax",		M_IMPLIED,																				{0xf0,____,____,____,____,____,____,____,____}},
		{"tsx",		M_IMPLIED,																				{0xf8,____,____,____,____,____,____,____,____}},
		{"txa",		M_IMPLIED,																				{0xe0,____,____,____,____,____,____,____,____}},
		{"txs",		M_IMPLIED,																				{0xe8,____,____,____,____,____,____,____,____}},
	};

static bool ParseImmediatePreamble(const char *line,unsigned int *lineIndex)
// Expect a pound sign, step over one if found
{
	SkipWhiteSpace(line,lineIndex);
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
	SkipWhiteSpace(line,lineIndex);
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

// opcode handling for Sunplus

enum
{
	POT_IMMEDIATE,
	POT_VALUE,					// extended, zero page, or relative	xxxx or xx
	POT_VALUE_OFF_X,			// extended, or zero page			xxxx,X or xx,X
	POT_INDIRECT,				// extended or zero page			(xxxx) or (xx)
	POT_INDIRECT_OFF_X,			// zero page						(xx,X)
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
							*type=POT_INDIRECT_OFF_X;
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
			else
			{
				// DEBUG something may need to happen around here so lda (ptr),x won't work but lda (this + that)*offset,x will
				//  (post-indexed indirect is not supported)
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
	if(opcode->typeMask&M_ZEROPAGE)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZEROPAGE],listingRecord))
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
	if(opcode->typeMask&M_ZEROPAGE)
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
	int
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
	if(opcode->typeMask&M_ZEROPAGE_OFF_X)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_ZEROPAGE_OFF_X],listingRecord))
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
	if(opcode->typeMask&M_ZEROPAGE_OFF_X)
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


static bool HandleIndirect(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a indirect value has been parsed, it must be indirect word
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_INDIRECT_WORD)
	{
		CheckUnsignedWordRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_INDIRECT_WORD],listingRecord))
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

static bool HandleIndirectOffX(OPCODE *opcode,int value,bool unresolved,LISTING_RECORD *listingRecord)
// a indirect value offset by X has been parsed, it must be zero page
{
	bool
		fail;

	fail=false;
	if(opcode->typeMask&M_INDIRECT_OFF_X)
	{
		CheckUnsignedByteRange(value,true,true);
		if(GenerateByte(opcode->baseOpcode[OT_INDIRECT_OFF_X],listingRecord))
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

	result=(OPCODE *)STFindDataForNameNoCase(opcodeSymbols,string);
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
						case POT_INDIRECT:
							result=HandleIndirect(opcode,value,unresolved,listingRecord);
							break;
						case POT_INDIRECT_OFF_X:
							result=HandleIndirectOffX(opcode,value,unresolved,listingRecord);
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
	processorFamily("Sunplus SPCxxx",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);

static PROCESSOR
	processors[]=
	{
		PROCESSOR(&processorFamily,"spc08a",NULL),
		PROCESSOR(&processorFamily,"spc21a",NULL),
		PROCESSOR(&processorFamily,"spc21a1",NULL),
		PROCESSOR(&processorFamily,"spc41a",NULL),
		PROCESSOR(&processorFamily,"spc41b",NULL),
		PROCESSOR(&processorFamily,"spc41b1",NULL),
		PROCESSOR(&processorFamily,"spc41c",NULL),
		PROCESSOR(&processorFamily,"spc81a",NULL),
		PROCESSOR(&processorFamily,"spc121a",NULL),
		PROCESSOR(&processorFamily,"spc251a",NULL),
		PROCESSOR(&processorFamily,"spc512a",NULL),
		PROCESSOR(&processorFamily,"spc500a1",NULL),
		PROCESSOR(&processorFamily,"spc1000a",NULL),
		PROCESSOR(&processorFamily,"spc2000a",NULL),
	};

