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


// Generate code for all pic families: 16C5X, 16CXX, and 17CXX

#include	"include.h"

static SYM_TABLE
	*pseudoOpcodeSymbols,			// pseudo ops shared between all processors
	*opcodeSymbols16C5X,			// symbols for the various PIC processor families
	*opcodeSymbols16CXX,
	*opcodeSymbols17CXX;

enum
{
	PIC_16C5X,						// families
	PIC_16CXX,
	PIC_17CXX
};

struct PIC_PROCESSOR
{
	unsigned int
		family;						// tells which family this processor belongs to
	unsigned int
		coreMask;					// tells how many bits of each core location are available
	unsigned int
		bankSelBits;				// tells how many bits of bank select are needed by this processor
	unsigned int
		bankISelBits;				// tells how many bits of indirect bank select to use
	unsigned int
		ROMSize;					// tells how many locations are in ROM
	unsigned int
		configAddress;				// tells where configuration information is to be written
	unsigned int
		IDLocAddress;				// address in which to store ID data for this processor (0 if invalid)
};

static PIC_PROCESSOR
	*currentProcessor;				// points at the record for the currently selected processor

#define	MAX_BAD_RAM	0x1000			// MAXRAM pseudo-op must have argument less than this

static bool
	testBadRAM;						// tells if bad ram should be tested for
static int
	maxRAM;							// the value of maxRAM that was given
static unsigned char
	badRAMMap[MAX_BAD_RAM>>3];		// bitmap of bad RAM locations (tested when maxram is specified)

enum
{
	OT_IMPLICIT,					// no operands
	OT_LITERAL8,					// one byte literal operand
	OT_CALL8,						// 8 bit literal operand (errors generated if out of range)
	OT_CALL11,						// 11 bit operand
	OT_GOTO9,						// 9 bit operand
	OT_GOTO11,						// 11 bit operand
	OT_GOTO13,						// 13 bit operand
	OT_REGISTER3,					// 3 bits of register
	OT_REGISTER5,					// 5 bits of register
	OT_REGISTER5_DEST,				// 5 bits of register and a destination
	OT_REGISTER5_BIT,				// 5 bits of register and bit index
	OT_REGISTER7,					// 7 bits of register
	OT_REGISTER7_DEST,				// 7 bits of register and a destination
	OT_REGISTER7_BIT,				// 7 bits of register and 3 bit index
	OT_REGISTER8,					// 8 bits of register
	OT_REGISTER8_DEST,				// 8 bits of register and a destination
	OT_REGISTER8_PORT,				// 8 bits of register and 5 bits of port
	OT_PORT_REGISTER8,				// 5 bits of port and 8 bits of register
	OT_HIGHLOW_INCREMENT_REGISTER8,	// 1 bit of high/low byte, 1 bit of increment and 8 bits of register
	OT_HIGHLOW_REGISTER8,			// 1 bit of high/low byte and 8 bits of register
	OT_REGISTER8_BIT,				// 8 bits of register and 3 bit bit index
};

struct OPCODE
{
	const char
		*name;
	unsigned int
		type;
	unsigned int
		baseOpcode;
};

// prototypes for the handler functions which need to be declared before we can build
// the opcode tables

static bool HandlePICDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICDT(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICConfig(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICIDLocs(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICMaxRAM(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICBadRAM(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICBankSel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICBankISel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
static bool HandlePICPageSel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);

static PSEUDO_OPCODE
	pseudoOpcodes[]=
	{
		{"db",		HandlePICDB			},
		{"dw",		HandlePICDW			},
		{"data",	HandlePICDW			},		// same as dw
		{"dt",		HandlePICDT			},
		{"ds",		HandleDS			},
		{"__config",HandlePICConfig		},
		{"__idlocs",HandlePICIDLocs		},
		{"__maxram",HandlePICMaxRAM		},
		{"__badram",HandlePICBadRAM		},
		{"banksel",	HandlePICBankSel	},
		{"bankisel",HandlePICBankISel	},
		{"pagesel",	HandlePICPageSel	},
	};

static OPCODE
	Opcodes16C5X[]=
	{
		{"andlw",	OT_LITERAL8,					0x0E00},
		{"call",	OT_CALL8,						0x0900},
		{"clrwdt",	OT_IMPLICIT,					0x0004},
		{"goto",	OT_GOTO9,						0x0A00},
		{"iorlw",	OT_LITERAL8,					0x0D00},
		{"movlw",	OT_LITERAL8,					0x0C00},
		{"option",	OT_IMPLICIT,					0x0002},
		{"retlw",	OT_LITERAL8,					0x0800},
		{"sleep",	OT_IMPLICIT,					0x0003},
		{"tris",	OT_REGISTER3,					0x0000},
		{"xorlw",	OT_LITERAL8,					0x0F00},
		{"addwf",	OT_REGISTER5_DEST,				0x01C0},
		{"andwf",	OT_REGISTER5_DEST,				0x0140},
		{"clrf",	OT_REGISTER5,					0x0060},
		{"clrw",	OT_IMPLICIT,					0x0040},
		{"comf",	OT_REGISTER5_DEST,				0x0240},
		{"decf",	OT_REGISTER5_DEST,				0x00C0},
		{"decfsz",	OT_REGISTER5_DEST,				0x02C0},
		{"incf",	OT_REGISTER5_DEST,				0x0280},
		{"incfsz",	OT_REGISTER5_DEST,				0x03C0},
		{"iorwf",	OT_REGISTER5_DEST,				0x0100},
		{"movf",	OT_REGISTER5_DEST,				0x0200},
		{"movwf",	OT_REGISTER5,					0x0020},
		{"nop",		OT_IMPLICIT,					0x0000},
		{"rlf",		OT_REGISTER5_DEST,				0x0340},
		{"rrf",		OT_REGISTER5_DEST,				0x0300},
		{"subwf",	OT_REGISTER5_DEST,				0x0080},
		{"swapf",	OT_REGISTER5_DEST,				0x0380},
		{"xorwf",	OT_REGISTER5_DEST,				0x0180},
		{"bcf",		OT_REGISTER5_BIT,				0x0400},
		{"bsf",		OT_REGISTER5_BIT,				0x0500},
		{"btfsc",	OT_REGISTER5_BIT,				0x0600},
		{"btfss",	OT_REGISTER5_BIT,				0x0700},
	};

static OPCODE
	Opcodes16CXX[]=
	{
		{"addlw",	OT_LITERAL8,					0x3E00},
		{"andlw",	OT_LITERAL8,					0x3900},
		{"call",	OT_CALL11,						0x2000},
		{"clrwdt",	OT_IMPLICIT,					0x0064},
		{"goto",	OT_GOTO11,						0x2800},
		{"iorlw",	OT_LITERAL8,					0x3800},
		{"movlw",	OT_LITERAL8,					0x3000},
		{"option",	OT_IMPLICIT,					0x0062},
		{"retfie",	OT_IMPLICIT,					0x0009},
		{"retlw",	OT_LITERAL8,					0x3400},
		{"return",	OT_IMPLICIT,					0x0008},
		{"sleep",	OT_IMPLICIT,					0x0063},
		{"tris",	OT_REGISTER3,					0x0060},
		{"xorlw",	OT_LITERAL8,					0x3A00},
		{"addwf",	OT_REGISTER7_DEST,				0x0700},
		{"andwf",	OT_REGISTER7_DEST,				0x0500},
		{"clrf",	OT_REGISTER7,					0x0180},
		{"clrw",	OT_IMPLICIT,					0x0100},
		{"comf",	OT_REGISTER7_DEST,				0x0900},
		{"decf",	OT_REGISTER7_DEST,				0x0300},
		{"decfsz",	OT_REGISTER7_DEST,				0x0B00},
		{"incf",	OT_REGISTER7_DEST,				0x0A00},
		{"incfsz",	OT_REGISTER7_DEST,				0x0F00},
		{"iorwf",	OT_REGISTER7_DEST,				0x0400},
		{"movf",	OT_REGISTER7_DEST,				0x0800},
		{"movwf",	OT_REGISTER7,					0x0080},
		{"nop",		OT_IMPLICIT,					0x0000},
		{"rlf",		OT_REGISTER7_DEST,				0x0D00},
		{"rrf",		OT_REGISTER7_DEST,				0x0C00},
		{"sublw",	OT_LITERAL8,					0x3C00},
		{"subwf",	OT_REGISTER7_DEST,				0x0200},
		{"swapf",	OT_REGISTER7_DEST,				0x0E00},
		{"xorwf",	OT_REGISTER7_DEST,				0x0600},
		{"bcf",		OT_REGISTER7_BIT,				0x1000},
		{"bsf",		OT_REGISTER7_BIT,				0x1400},
		{"btfsc",	OT_REGISTER7_BIT,				0x1800},
		{"btfss",	OT_REGISTER7_BIT,				0x1C00},
	};

static OPCODE
	Opcodes17CXX[]=
	{
		{"addlw",	OT_LITERAL8,					0xB100},
		{"addwf",	OT_REGISTER8_DEST,				0x0E00},
		{"addwfc",	OT_REGISTER8_DEST,				0x1000},
		{"andlw",	OT_LITERAL8,					0xB500},
		{"andwf",	OT_REGISTER8_DEST,				0x0A00},
		{"bcf",		OT_REGISTER8_BIT,				0x8800},
		{"bsf",		OT_REGISTER8_BIT,				0x8000},
		{"btfsc",	OT_REGISTER8_BIT,				0x9800},
		{"btfss",	OT_REGISTER8_BIT,				0x9000},
		{"btg",		OT_REGISTER8_BIT,				0x3800},
		{"call",	OT_GOTO13,						0xE000},
		{"clrf",	OT_REGISTER8_DEST,				0x2800},
		{"clrwdt",	OT_IMPLICIT,					0x0004},
		{"comf",	OT_REGISTER8_DEST,				0x1200},
		{"cpfseq",	OT_REGISTER8,					0x3100},
		{"cpfsgt",	OT_REGISTER8,					0x3200},
		{"cpfslt",	OT_REGISTER8,					0x3000},
		{"daw",		OT_REGISTER8_DEST,				0x2E00},
		{"dcfsnz",	OT_REGISTER8_DEST,				0x2600},
		{"decf",	OT_REGISTER8_DEST,				0x0600},
		{"decfsz",	OT_REGISTER8_DEST,				0x1600},
		{"goto",	OT_GOTO13,						0xC000},
		{"incf",	OT_REGISTER8_DEST,				0x1400},
		{"incfsz",	OT_REGISTER8_DEST,				0x1E00},
		{"infsnz",	OT_REGISTER8_DEST,				0x2400},
		{"iorlw",	OT_LITERAL8,					0xB300},
		{"iorwf",	OT_REGISTER8_DEST,				0x0800},
		{"lcall",	OT_LITERAL8,					0xB700},
		{"movfp",	OT_REGISTER8_PORT,				0x6000},
		{"movlb",	OT_LITERAL8,					0xB800},
		{"movlr",	OT_LITERAL8,					0xBA00},
		{"movlw",	OT_LITERAL8,					0xB000},
		{"movpf",	OT_PORT_REGISTER8,				0x4000},
		{"movwf",	OT_REGISTER8,					0x0100},
		{"mullw",	OT_LITERAL8,					0xBC00},
		{"mulwf",	OT_REGISTER8,					0x3400},
		{"negw",	OT_REGISTER8_DEST,				0x2C00},
		{"nop",		OT_IMPLICIT,					0x0000},
		{"ret",		OT_IMPLICIT,					0x0002},
		{"retfie",	OT_IMPLICIT,					0x0005},
		{"retlw",	OT_LITERAL8,					0xB600},
		{"return",	OT_IMPLICIT,					0x0002},
		{"rlcf",	OT_REGISTER8_DEST,				0x1A00},
		{"rlncf",	OT_REGISTER8_DEST,				0x2200},
		{"rrcf",	OT_REGISTER8_DEST,				0x1800},
		{"rrncf",	OT_REGISTER8_DEST,				0x2000},
		{"setf",	OT_REGISTER8_DEST,				0x2A00},
		{"sleep",	OT_IMPLICIT,					0x0003},
		{"sublw",	OT_LITERAL8,					0xB200},
		{"subwf",	OT_REGISTER8_DEST,				0x0400},
		{"subwfb",	OT_REGISTER8_DEST,				0x0200},
		{"swapf",	OT_REGISTER8_DEST,				0x1C00},
		{"tablrd",	OT_HIGHLOW_INCREMENT_REGISTER8,	0xA800},
		{"tablwt",	OT_HIGHLOW_INCREMENT_REGISTER8,	0xAC00},
		{"tlrd",	OT_HIGHLOW_REGISTER8,			0xA000},
		{"tlwt",	OT_HIGHLOW_REGISTER8,			0xA400},
		{"tstfsz",	OT_REGISTER8,					0x3300},
		{"xorlw",	OT_LITERAL8,					0xB400},
		{"xorwf",	OT_REGISTER8_DEST,				0x0C00},
	};

static bool GeneratePICWordsAtAddress(unsigned int address,unsigned int numWords,int *wordValues,LISTING_RECORD *listingRecord)
// force word values out to memory starting at the given address
{
	unsigned char
		outputBytes[2];
	int
		temp;
	unsigned int
		length;
	unsigned int
		i;
	bool
		fail;

	fail=false;
	if(currentSegment)
	{
		if(!intermediatePass)				// only do the real work if necessary
		{
			listingRecord->listPC=address;				// change list output to show this address

			for(i=0;!fail&&i<numWords;i++)
			{
				temp=wordValues[i];
				if(temp&~currentProcessor->coreMask)	// see if word uses more bits than it should (if so, kill upper bits and generate warning)
				{
					AssemblyComplaint(NULL,false,"Program word too large. Truncated to core size. (%04X)\n",temp);
					temp&=currentProcessor->coreMask;
				}
				length=strlen(listingRecord->listObjectString);
				if(length+6<MAX_STRING)
				{
					sprintf(&listingRecord->listObjectString[length],"%04X ",temp);		// create list file output
				}
				outputBytes[0]=temp&0xFF;				// data is written as little endian words
				outputBytes[1]=temp>>8;

				fail=!AddBytesToSegment(currentSegment,address*2,outputBytes,2);	// each memory location is 16 bits wide
				address++;
			}
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Attempt to write with no current segment\n");
	}
	return(!fail);
}

static bool GeneratePICWord(int wordValue,LISTING_RECORD *listingRecord)
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
		if(currentSegment->currentPC+currentSegment->codeGenOffset<currentProcessor->ROMSize)
		{
			if(!intermediatePass)				// only do the real work if necessary
			{
				if(wordValue&~currentProcessor->coreMask)	// see if word uses more bits than it should (if so, kill upper bits and generate warning)
				{
					AssemblyComplaint(NULL,false,"Program word too large. Truncated to core size. (%04X)\n",wordValue);
					wordValue&=currentProcessor->coreMask;
				}
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
static int
	byteGenCounter;

static void StartByteGeneration()
// some opcodes generate bytes at a time which need to be packed into words
// this makes that easier
{
	byteGenCounter=0;
}

static bool GeneratePICByte(unsigned char byteValue,LISTING_RECORD *listingRecord)
// output the byteValue to the current segment
{
	bool
		fail;

	fail=false;
	if(byteGenCounter&1)
	{
		fail=!GeneratePICWord((heldByte<<8)+byteValue,listingRecord);
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
		return(GeneratePICByte(0,listingRecord));
	}
	return(true);
}

static bool ParseOperandSeparator(const char *line,unsigned int *lineIndex)
// Expect a comma, step over one if found
{
	SkipWhiteSpace(line,lineIndex);		// move past any white space
	if(line[*lineIndex]==',')			// does this look like an opcode separator?
	{
		(*lineIndex)++;						// step over it
		return(true);
	}
	return(false);
}

static bool ParseRangeSeparator(const char *line,unsigned int *lineIndex)
// Expect a dash, step over one if found
{
	SkipWhiteSpace(line,lineIndex);		// move past any white space
	if(line[*lineIndex]=='-')			// does this look like an opcode separator?
	{
		(*lineIndex)++;						// step over it
		return(true);
	}
	return(false);
}

static bool ParseDestination(const char *line,unsigned int *lineIndex,int *destination,bool *unresolved,bool *unspecified)
// try to find a destination register
{
	*destination=1;							// default if unresolved, or unspecified
	*unspecified=true;
	if(ParseOperandSeparator(line,lineIndex))
	{
		*unspecified=false;
		return(ParseExpression(line,lineIndex,destination,unresolved));
	}
	return(true);
}

static bool ParseNextExpression(const char *line,unsigned int *lineIndex,int *value,bool *unresolved)
// look for a separator, and another expression
{
	if(ParseOperandSeparator(line,lineIndex))
	{
		return(ParseExpression(line,lineIndex,value,unresolved));
	}
	return(false);
}

static void ReportUnspecifiedDestination()
// complain that the operands are bad for a given opcode
{
	AssemblyComplaint(NULL,false,"Unspecified destination -- defaults to register file\n");
}

static bool CheckRAMLocation(int value,bool generateMessage,bool isError)
// NOTE: because the PICS use a banked memory scheme, and because
// we want to allow programmers to be able to specify memory locations
// as absolutes, we only test to see if value is out of the range
// specified by MAXRAM, and BADRAM.
// This means that the value may be larger than will fit in the field,
// but we assume the programmer has done the correct bank selecting
// to make sure he is referencing the place he thinks he is.
{
	if((value>=0)&&(value<=maxRAM)&&(!(badRAMMap[value>>3]&(1<<(value&7)))))
	{
		return(true);
	}

	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Invalid RAM location specified\n");
	}
	return(false);
}

static bool CheckRegister3Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 3 bit register
// complain if not
{
	if(testBadRAM)
	{
		return(CheckRAMLocation(value,generateMessage,isError));
	}
	return(true);
}

static bool CheckRegister5Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 5 bit register
// complain if not
{
	if(testBadRAM)
	{
		return(CheckRAMLocation(value,generateMessage,isError));
	}
	return(true);
}

static bool CheckRegister7Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 7 bit register
// complain if not
{
	if(testBadRAM)
	{
		return(CheckRAMLocation(value,generateMessage,isError));
	}
	return(true);
}

static bool CheckRegister8Range(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 8 bit register
// complain if not
// NOTE: because the PICS use a banked memory scheme, and because
// we want to allow programmers to be able to specify memory locations
// as absolutes, we only test to see if value is out of the range
// specified by MAXRAM, and BADRAM.
// This means that the value may be larger than will fit in the field,
// but we assume the programmer has done the correct bank selecting
// to make sure he is referencing the place he thinks he is.
{
	if(testBadRAM)
	{
		return(CheckRAMLocation(value,generateMessage,isError));
	}
	return(true);
}

static bool CheckPortRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 5 bit port
// complain if not
{
	if(value>=0&&value<32)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Port value out of range\n");
	}
	return(false);
}

static bool CheckDestinationRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 1 bit destination
// complain if not
{
	if(value>=0&&value<2)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Destination value out of range\n");
	}
	return(false);
}

static bool CheckHighLowRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 1 bit highlow flag
// complain if not
{
	if(value>=0&&value<2)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"High/Low value out of range\n");
	}
	return(false);
}

static bool CheckIncrementRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as a 1 bit increment flag
// complain if not
{
	if(value>=0&&value<2)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Increment value out of range\n");
	}
	return(false);
}

// pseudo op handling

static bool HandlePICDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
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
				fail=!GeneratePICByte(outputString[i],listingRecord);
				i++;
			}
		}	
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckByteRange(value,true,true);
			fail=!GeneratePICByte(value&0xFF,listingRecord);
		}
		else
		{
			ReportBadOperands();
			done=true;
		}
		if(!done&&!fail)				// look for separator, or comment
		{
			if(ParseOperandSeparator(line,lineIndex))
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

static bool HandlePICDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
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
	StartByteGeneration();
	while(!done&&!fail)
	{
		if(ParseQuotedString(line,lineIndex,'"','"',outputString,&stringLength))
		{
			i=0;
			while(!fail&&i<stringLength)
			{
				fail=!GeneratePICByte(outputString[i],listingRecord);
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
			if(GeneratePICByte(value>>8,listingRecord))
			{
				fail=!GeneratePICByte(value&0xFF,listingRecord);
			}
			else
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
			if(ParseOperandSeparator(line,lineIndex))
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

static bool HandlePICDT(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// make a table of retlw instructions with associated data
{
	int
		value;
	bool
		unresolved;
	unsigned char
		insnByte;
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
	switch(currentProcessor->family)
	{
		case PIC_16C5X:
			insnByte=0x08;
			break;
		case PIC_16CXX:
			insnByte=0x34;
			break;
		case PIC_17CXX:
		default:
			insnByte=0xB6;
			break;
	}

	StartByteGeneration();
	while(!done&&!fail)
	{
		if(ParseQuotedString(line,lineIndex,'"','"',outputString,&stringLength))
		{
			i=0;
			while(!fail&&i<stringLength)
			{
				if(GeneratePICByte(insnByte,listingRecord))		// output the retlw instruction
				{
					fail=!GeneratePICByte(outputString[i],listingRecord);
				}
				else
				{
					fail=true;
				}
				i++;
			}
		}	
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckByteRange(value,true,true);
			if(GeneratePICByte(insnByte,listingRecord))			// output the retlw instruction
			{
				fail=!GeneratePICByte(value&0xFF,listingRecord);
			}
			else
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
			if(ParseOperandSeparator(line,lineIndex))
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

static bool HandlePICConfig(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// write the given configuration bits out
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	if(lineLabel)
	{
		ReportDisallowedLabel(lineLabel);
	}
	fail=false;
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				fail=!GeneratePICWordsAtAddress(currentProcessor->configAddress,1,&value,listingRecord);
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
	return(!fail);
}

static bool HandlePICIDLocs(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// write the ID locations for the given processor
{
	int
		value;
	bool
		unresolved;
	int
		IDValues[4];
	bool
		fail;

	if(lineLabel)
	{
		ReportDisallowedLabel(lineLabel);
	}
	fail=false;
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				if(currentProcessor->IDLocAddress)	// does this processor support ID?
				{
					IDValues[0]=(value>>12)&0x0F;
					IDValues[1]=(value>>8)&0x0F;
					IDValues[2]=(value>>4)&0x0F;
					IDValues[3]=(value)&0x0F;
					fail=!GeneratePICWordsAtAddress(currentProcessor->IDLocAddress,4,IDValues,listingRecord);
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' invalid for this processor\n",opcodeName);
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
	return(!fail);
}

static bool HandlePICMaxRAM(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// maximum RAM location is being specified
{
	unsigned int
		i;
	int
		value;
	bool
		fail,
		unresolved;

	if(lineLabel)
	{
		ReportDisallowedLabel(lineLabel);
	}
	fail=false;
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				if(!unresolved)
				{
					if((value>=0)&&(value<MAX_BAD_RAM))
					{
						maxRAM=value;
						testBadRAM=true;
						for(i=0;i<(MAX_BAD_RAM>>3);i++)
						{
							badRAMMap[i]=0;		// mark all of RAM as being valid
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
	return(!fail);
}

static bool HandlePICBadRAM(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// unusable RAM locations are being specified
// NOTE: because some bozo working on mpasm decided that the ranges should be separated with
// -'s, it is not possible to parse an expression for the range values (since
// expressions can and often do contain -'s)
{
	int
		startValue,
		endValue;
	unsigned int
		i;
	bool
		fail,
		done,
		parseFail;

	parseFail=false;
	done=false;
	if(lineLabel)
	{
		ReportDisallowedLabel(lineLabel);
	}
	fail=false;
	while(!fail&&!parseFail&&!done)
	{
		if(ParseNumber(line,lineIndex,&startValue))
		{
			if(ParseRangeSeparator(line,lineIndex))
			{
				if(!ParseNumber(line,lineIndex,&endValue))
				{
					ReportBadOperands();
					parseFail=true;
				}
			}
			else
			{
				endValue=startValue;
			}
			if(!parseFail)	// have start and end of range
			{
				if(testBadRAM)
				{
					if((startValue>=0)&&(endValue>=startValue)&&(endValue<=maxRAM))
					{
						for(i=(unsigned int)startValue;i<=(unsigned int)endValue;i++)
						{
							badRAMMap[i>>3]|=(1<<(i&7));		// mark bad ram locations into bit table
						}
					}
					else
					{
						ReportBadOperands();
						parseFail=true;
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' before __MAXRAM\n",opcodeName);
					parseFail=true;
				}
			}
			if(!parseFail&&!ParseOperandSeparator(line,lineIndex))	// need to see separator to continue
			{
				done=true;
			}
		}
		else		// failed to get a number when one was expected
		{
			ReportBadOperands();
			parseFail=true;
		}
	}
	if(!parseFail)
	{
		if(!ParseComment(line,lineIndex))				// make sure we got to the end
		{
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandlePICBankSel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Generate bank selection code
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				switch(currentProcessor->family)
				{
					case PIC_16C5X:
						if(currentProcessor->bankSelBits)
						{
							if(GeneratePICWord(0x04A4|((value&0x20)<<3),listingRecord))		// generate BSF, or BCF on bit 5 of FSR
							{
								if(currentProcessor->bankSelBits>1)
								{
									fail=!GeneratePICWord(0x04C4|((value&0x40)<<2),listingRecord);	// generate BSF, or BCF on bit 6 of FSR
								}
							}
							else
							{
								fail=true;
							}
						}
						break;
					case PIC_16CXX:
						if(currentProcessor->bankSelBits)
						{
							if(GeneratePICWord(0x1283|((value&0x80)<<3),listingRecord))			// generate BSF, or BCF on bit 5 of STATUS
							{
								if(currentProcessor->bankSelBits>1)
								{
									fail=!GeneratePICWord(0x1303|((value&0x100)<<2),listingRecord);	// generate BSF, or BCF on bit 6 of STATUS
								}
							}
							else
							{
								fail=true;
							}
						}
						break;
					case PIC_17CXX:
						if((currentProcessor->bankSelBits)&&(value&0xFF)>=0x20)
						{
							fail=!GeneratePICWord(0xBA00|((value>>4)&0xF0),listingRecord);		// generate MOVLR
						}
						else
						{
							fail=!GeneratePICWord(0xB800|((value>>8)&0x0F),listingRecord);		// generate MOVLB
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
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandlePICBankISel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Generate indirect bank selection code
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				switch(currentProcessor->family)
				{
					case PIC_16C5X:
						break;
					case PIC_16CXX:
						if(currentProcessor->bankISelBits)
						{
							fail=!GeneratePICWord(0x1383|((value&0x100)<<2),listingRecord);		// generate BSF, or BCF on bit 7 of STATUS
						}
						break;
					case PIC_17CXX:
						if((currentProcessor->bankISelBits)&&(value&0xFF)>=0x20)
						{
							fail=!GeneratePICWord(0xBA00|((value>>4)&0xF0),listingRecord);		// generate MOVLR
						}
						else
						{
							fail=!GeneratePICWord(0xB800|((value>>8)&0x0F),listingRecord);		// generate MOVLB
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
			ReportBadOperands();
		}
	}
	return(!fail);
}

static bool HandlePICPageSel(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// Generate page selection code
{
	int
		value;
	bool
		unresolved;
	bool
		fail;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				switch(currentProcessor->family)
				{
					case PIC_16C5X:
						if(currentProcessor->ROMSize>0x200)
						{
							if(GeneratePICWord(0x04A3|((value&0x200)>>1),listingRecord))				// generate BSF, or BCF on bit 5 of STATUS
							{
								if(currentProcessor->ROMSize>0x400)
								{
									fail=!GeneratePICWord(0x04C3|((value&0x400)>>2),listingRecord);	// generate BSF, or BCF on bit 6 of STATUS
								}
							}
							else
							{
								fail=true;
							}
						}
						break;
					case PIC_16CXX:
						if(currentProcessor->ROMSize>0x800)
						{
							if(GeneratePICWord(0x3000|((value>>8)&0xFF),listingRecord))		// generate MOVLW (high byte of address)
							{
								fail=!GeneratePICWord(0x008A,listingRecord);					// generate MOVWF PCLATH
							}
							else
							{
								fail=true;
							}
						}
						break;
					case PIC_17CXX:
						if(GeneratePICWord(0xB000|((value>>8)&0xFF),listingRecord))		// generate MOVLW (high byte of address)
						{
							fail=!GeneratePICWord(0x0103,listingRecord);					// generate MOVWF PCLATH
						}
						else
						{
							fail=true;
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
			ReportBadOperands();
		}
	}
	return(!fail);
}

// opcode handling

static bool HandleIMPLICIT(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle implicit opcodes
{
	bool
		fail;

	fail=false;
	if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
	{
		fail=!GeneratePICWord(opcode->baseOpcode,listingRecord);
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleLITERAL8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle literal 8 bit opcodes
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			CheckByteRange(immediate,true,true);
			immediate&=0xFF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleCALL8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle call opcodes (8 bits of operand, the 9th bit must be clear or an error is generated)
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			if(immediate<0||immediate>=1024||(immediate&(1<<8)))
			{
				AssemblyComplaint(NULL,false,"Address (0x%04X) is out of range, it will be truncated\n",immediate);
			}
			immediate&=0xFF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleCALL11(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle call opcodes (11 bits of operand)
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			if(immediate<0||immediate>=2048)
			{
				AssemblyComplaint(NULL,false,"Address (0x%04X) is out of range, it will be truncated\n",immediate);
			}
			immediate&=0x7FF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleGOTO9(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle goto opcodes (9 bit operand)
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			if(immediate<0||immediate>=1024)
			{
				AssemblyComplaint(NULL,false,"Address (0x%04X) is out of range, it will be truncated\n",immediate);
			}
			immediate&=0x1FF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleGOTO11(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle goto opcodes (11 bit operand)
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			if(immediate<0||immediate>=2048)
			{
				AssemblyComplaint(NULL,false,"Address (0x%04X) is out of range, it will be truncated\n",immediate);
			}
			immediate&=0x7FF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleGOTO13(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle goto/call opcodes (13 bit operand)
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			if(immediate<0||immediate>=8192)
			{
				AssemblyComplaint(NULL,false,"Address (0x%04X) is out of range, it will be truncated\n",immediate);
			}
			immediate&=0x1FFF;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleREGISTER3(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 3 bit register
{
	int
		immediate;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&immediate,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			CheckRegister3Range(immediate,true,false);
			immediate&=0x07;
			fail=!GeneratePICWord(opcode->baseOpcode|immediate,listingRecord);
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

static bool HandleREGISTER5(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 5 bit register
{
	int
		reg;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			CheckRegister5Range(reg,true,false);
			reg&=0x1F;
			fail=!GeneratePICWord(opcode->baseOpcode|reg,listingRecord);
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

static bool HandleREGISTER5_DEST(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 5 bit register and a destination
{
	int
		reg;
	bool
		registerUnresolved;
	int
		destination;
	bool
		destUnresolved,
		destUnspecified;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))		// get the register
	{
		if(ParseDestination(line,lineIndex,&destination,&destUnresolved,&destUnspecified))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister5Range(reg,true,false);
				CheckDestinationRange(destination,true,true);
				reg&=0x1F;
				destination&=0x01;
				fail=!GeneratePICWord(opcode->baseOpcode|(destination<<5)|reg,listingRecord);
				if(destUnspecified)
				{
					ReportUnspecifiedDestination();
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER5_BIT(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 5 bit register and a 3 bit bit index
{
	int
		reg;
	bool
		registerUnresolved;
	int
		bit;
	bool
		bitUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&bit,&bitUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister5Range(reg,true,false);
				Check8BitIndexRange(bit,true,true);
				reg&=0x1F;
				bit&=0x07;
				fail=!GeneratePICWord(opcode->baseOpcode|(bit<<5)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER7(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 7 bit register
{
	int
		reg;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			CheckRegister7Range(reg,true,false);
			reg&=0x7F;
			fail=!GeneratePICWord(opcode->baseOpcode|reg,listingRecord);
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

static bool HandleREGISTER7_DEST(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 7 bit register and a destination
{
	int
		reg;
	bool
		registerUnresolved;
	int
		destination;
	bool
		destUnresolved,
		destUnspecified;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))
	{
		if(ParseDestination(line,lineIndex,&destination,&destUnresolved,&destUnspecified))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister7Range(reg,true,false);
				CheckDestinationRange(destination,true,true);
				reg&=0x7F;
				destination&=0x01;
				fail=!GeneratePICWord(opcode->baseOpcode|(destination<<7)|reg,listingRecord);
				if(destUnspecified)
				{
					ReportUnspecifiedDestination();
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER7_BIT(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 7 bit register and a 3 bit bit index
{
	int
		reg;
	bool
		registerUnresolved;
	int
		bit;
	bool
		bitUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&bit,&bitUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister7Range(reg,true,false);
				Check8BitIndexRange(bit,true,true);
				reg&=0x7F;
				bit&=0x07;
				fail=!GeneratePICWord(opcode->baseOpcode|(bit<<7)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with an 8 bit register
{
	int
		reg;
	bool
		unresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&unresolved))
	{
		if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
		{
			CheckRegister8Range(reg,true,false);
			reg&=0xFF;
			fail=!GeneratePICWord(opcode->baseOpcode|reg,listingRecord);
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

static bool HandleREGISTER8_DEST(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with an 8 bit register and 1 bit destination
{
	int
		reg;
	bool
		registerUnresolved;
	int
		destination;
	bool
		destUnresolved,
		destUnspecified;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))		// get the register
	{
		if(ParseDestination(line,lineIndex,&destination,&destUnresolved,&destUnspecified))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister8Range(reg,true,false);
				CheckDestinationRange(destination,true,true);
				reg&=0xFF;
				destination&=0x01;
				fail=!GeneratePICWord(opcode->baseOpcode|(destination<<8)|reg,listingRecord);
				if(destUnspecified)
				{
					ReportUnspecifiedDestination();
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER8_PORT(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with an 8 bit register and 5 bit port
{
	int
		reg;
	bool
		registerUnresolved;
	int
		port;
	bool
		portUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))		// get the register
	{
		if(ParseNextExpression(line,lineIndex,&port,&portUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister8Range(reg,true,false);
				CheckPortRange(port,true,true);
				reg&=0xFF;
				port&=0x1F;
				fail=!GeneratePICWord(opcode->baseOpcode|(port<<8)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandlePORT_REGISTER8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with a 5 bit port and 8 bit register
{
	int
		reg;
	bool
		registerUnresolved;
	int
		port;
	bool
		portUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&port,&portUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&reg,&registerUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckPortRange(port,true,true);
				CheckRegister8Range(reg,true,false);
				reg&=0xFF;
				port&=0x1F;
				fail=!GeneratePICWord(opcode->baseOpcode|(port<<8)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleHIGHLOW_INCREMENT_REGISTER8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with high/low bit, an increment bit, and an 8 bit register
{
	int
		reg;
	bool
		registerUnresolved;
	int
		highLow;
	bool
		highLowUnresolved;
	int
		increment;
	bool
		incrementUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&highLow,&highLowUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&increment,&incrementUnresolved))
		{
			if(ParseNextExpression(line,lineIndex,&reg,&registerUnresolved))
			{
				if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
				{
					CheckHighLowRange(highLow,true,true);
					CheckIncrementRange(increment,true,true);
					CheckRegister8Range(reg,true,false);
					reg&=0xFF;
					highLow&=0x01;
					increment&=0x01;
					fail=!GeneratePICWord(opcode->baseOpcode|(highLow<<9)|(increment<<8)|reg,listingRecord);
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
			ReportBadOperands();
		}
	}
	else
	{
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleHIGHLOW_REGISTER8(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with high/low bit, and an 8 bit register
{
	int
		reg;
	bool
		registerUnresolved;
	int
		highLow;
	bool
		highLowUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&highLow,&highLowUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&reg,&registerUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckHighLowRange(highLow,true,true);
				CheckRegister8Range(reg,true,false);
				reg&=0xFF;
				highLow&=0x01;
				fail=!GeneratePICWord(opcode->baseOpcode|(highLow<<9)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static bool HandleREGISTER8_BIT(OPCODE *opcode,const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord)
// handle opcodes with an 8 bit register and a 3 bit bit index
{
	int
		reg;
	bool
		registerUnresolved;
	int
		bit;
	bool
		bitUnresolved;
	bool
		fail;

	fail=false;
	if(ParseExpression(line,lineIndex,&reg,&registerUnresolved))
	{
		if(ParseNextExpression(line,lineIndex,&bit,&bitUnresolved))
		{
			if(ParseComment(line,lineIndex))				// make sure there's nothing else on the line
			{
				CheckRegister8Range(reg,true,false);
				Check8BitIndexRange(bit,true,true);
				reg&=0xFF;
				bit&=0x07;
				fail=!GeneratePICWord(opcode->baseOpcode|(bit<<8)|reg,listingRecord);
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
		ReportBadOperands();
	}
	return(!fail);
}

static OPCODE *MatchOpcode(const char *string)
// match opcodes for the current processor, return NULL if none matched
{
	OPCODE
		*result;

	result=NULL;
	switch(currentProcessor->family)
	{
		case PIC_16C5X:
			result=(OPCODE *)STFindDataForNameNoCase(opcodeSymbols16C5X,string);
			break;
		case PIC_16CXX:
			result=(OPCODE *)STFindDataForNameNoCase(opcodeSymbols16CXX,string);
			break;
		case PIC_17CXX:
			result=(OPCODE *)STFindDataForNameNoCase(opcodeSymbols17CXX,string);
			break;
	}
	return(result);
}

static bool AttemptOpcode(const char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord,bool *success)
// handle opcodes for this processor
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

	result=true;					// no hard failure yet
	*success=false;					// no match yet
	tempIndex=*lineIndex;
	if(ParseName(line,&tempIndex,string))						// something that looks like an opcode?
	{
		if((opcode=MatchOpcode(string)))						// found opcode?
		{
			*lineIndex=tempIndex;								// actually push forward on the line
			*success=true;
			switch(opcode->type)
			{
				case OT_IMPLICIT:
					 result=HandleIMPLICIT(opcode,line,lineIndex,listingRecord);
					break;
				case OT_LITERAL8:
					 result=HandleLITERAL8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_CALL8:
					 result=HandleCALL8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_CALL11:
					 result=HandleCALL11(opcode,line,lineIndex,listingRecord);
					break;
				case OT_GOTO9:
					 result=HandleGOTO9(opcode,line,lineIndex,listingRecord);
					break;
				case OT_GOTO11:
					 result=HandleGOTO11(opcode,line,lineIndex,listingRecord);
					break;
				case OT_GOTO13:
					 result=HandleGOTO13(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER3:
					 result=HandleREGISTER3(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER5:
					 result=HandleREGISTER5(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER5_DEST:
					 result=HandleREGISTER5_DEST(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER5_BIT:
					 result=HandleREGISTER5_BIT(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER7:
					 result=HandleREGISTER7(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER7_DEST:
					 result=HandleREGISTER7_DEST(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER7_BIT:
					 result=HandleREGISTER7_BIT(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER8:
					 result=HandleREGISTER8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER8_DEST:
					 result=HandleREGISTER8_DEST(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER8_PORT:
					 result=HandleREGISTER8_PORT(opcode,line,lineIndex,listingRecord);
					break;
				case OT_PORT_REGISTER8:
					 result=HandlePORT_REGISTER8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_HIGHLOW_INCREMENT_REGISTER8:
					 result=HandleHIGHLOW_INCREMENT_REGISTER8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_HIGHLOW_REGISTER8:
					 result=HandleHIGHLOW_REGISTER8(opcode,line,lineIndex,listingRecord);
					break;
				case OT_REGISTER8_BIT:
					 result=HandleREGISTER8_BIT(opcode,line,lineIndex,listingRecord);
					break;
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
// NOTE: the PICs use a word based PC, so alter the PC to have word meaning
// when the processor is selected
{
	currentProcessor=(PIC_PROCESSOR *)processor->processorData;	// remember which processor has been selected
	testBadRAM=false;												// selecting a processor kills the bad ram test
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
	STDisposeSymbolTable(opcodeSymbols17CXX);
	STDisposeSymbolTable(opcodeSymbols16CXX);
	STDisposeSymbolTable(opcodeSymbols16C5X);
	STDisposeSymbolTable(pseudoOpcodeSymbols);
}

static bool InitFamily()
// initialize symbol table
{
	unsigned int
		i;
	bool
		fail;

	testBadRAM=false;			// by default, do not test for bad RAM
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
			if((opcodeSymbols16C5X=STNewSymbolTable(elementsof(Opcodes16C5X))))
			{
				for(i=0;!fail&&(i<elementsof(Opcodes16C5X));i++)
				{
					if(!STAddEntryAtEnd(opcodeSymbols16C5X,Opcodes16C5X[i].name,&Opcodes16C5X[i]))
					{
						fail=true;
					}
				}
				if(!fail)
				{
					if((opcodeSymbols16CXX=STNewSymbolTable(elementsof(Opcodes16CXX))))
					{
						for(i=0;!fail&&(i<elementsof(Opcodes16CXX));i++)
						{
							if(!STAddEntryAtEnd(opcodeSymbols16CXX,Opcodes16CXX[i].name,&Opcodes16CXX[i]))
							{
								fail=true;
							}
						}
						if(!fail)
						{
							if((opcodeSymbols17CXX=STNewSymbolTable(elementsof(Opcodes17CXX))))
							{
								for(i=0;!fail&&(i<elementsof(Opcodes17CXX));i++)
								{
									if(!STAddEntryAtEnd(opcodeSymbols17CXX,Opcodes17CXX[i].name,&Opcodes17CXX[i]))
									{
										fail=true;
									}
								}
								if(!fail)
								{
									return(true);
								}
								STDisposeSymbolTable(opcodeSymbols17CXX);
							}
						}
						STDisposeSymbolTable(opcodeSymbols16CXX);
					}
				}
				STDisposeSymbolTable(opcodeSymbols16C5X);
			}
		}
		STDisposeSymbolTable(pseudoOpcodeSymbols);
	}
	return(false);
}

// NOTE: there is some uncertainty in these values as I do not have
// the spec page for every device here.
// If you find a problem, email the author with upated values

static const PIC_PROCESSOR
// family,coreMask,bankSelBits,bankISelBits,ROMSize,configAddress,IDLocAddress

	pic12c508	={PIC_16C5X,0x0FFF,0,0,0x0200,0x0FFF,0x0200},
	pic12c509	={PIC_16C5X,0x0FFF,2,0,0x0400,0x0FFF,0x0400},
	pic12ce518	={PIC_16C5X,0x0FFF,0,0,0x0200,0x0FFF,0x0200},
	pic12ce519	={PIC_16C5X,0x0FFF,2,0,0x0400,0x0FFF,0x0400},
	pic12c671	={PIC_16CXX,0x3FFF,2,1,0x0400,0x2007,0x2000},
	pic12c672	={PIC_16CXX,0x3FFF,2,1,0x0800,0x2007,0x2000},
	pic16c505	={PIC_16C5X,0x0FFF,2,0,0x0400,0x0FFF,0x0400},
	pic16c52	={PIC_16C5X,0x0FFF,0,0,0x0180,0x0FFF,0x0180},
	pic16c54	={PIC_16C5X,0x0FFF,0,0,0x0200,0x0FFF,0x0200},
	pic16c55	={PIC_16C5X,0x0FFF,0,0,0x0200,0x0FFF,0x0200},
	pic16c56	={PIC_16C5X,0x0FFF,0,0,0x0400,0x0FFF,0x0400},
	pic16c57	={PIC_16C5X,0x0FFF,2,0,0x0800,0x0FFF,0x0800},
	pic16c58	={PIC_16C5X,0x0FFF,2,0,0x0800,0x0FFF,0x0800},

	pic14000	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c554	={PIC_16CXX,0x3FFF,1,0,0x0200,0x2007,0x2000},
	pic16c556	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16c558	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c61	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16c62	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c620	={PIC_16CXX,0x3FFF,1,0,0x0200,0x2007,0x2000},
	pic16c621	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16c622	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c63	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c64	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c642	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c65	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c66	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16c662	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c67	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16c71	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16c710	={PIC_16CXX,0x3FFF,1,0,0x0200,0x2007,0x2000},
	pic16c711	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16c715	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c72	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},
	pic16c73	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c74	={PIC_16CXX,0x3FFF,1,0,0x1000,0x2007,0x2000},
	pic16c76	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16c77	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16f627	={PIC_16CXX,0x3FFF,2,1,0x0400,0x2007,0x2000},
	pic16f628	={PIC_16CXX,0x3FFF,2,1,0x0800,0x2007,0x2000},
	pic16f648	={PIC_16CXX,0x3FFF,2,1,0x1000,0x2007,0x2000},
	pic16f818	={PIC_16CXX,0x3FFF,2,1,0x0400,0x2007,0x2000},
	pic16f819	={PIC_16CXX,0x3FFF,2,1,0x0800,0x2007,0x2000},
	pic16f83	={PIC_16CXX,0x3FFF,1,0,0x0200,0x2007,0x2000},
	pic16c84	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16f84	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic16f873	={PIC_16CXX,0x3FFF,2,1,0x1000,0x2007,0x2000},
	pic16f874	={PIC_16CXX,0x3FFF,2,1,0x1000,0x2007,0x2000},
	pic16f876	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16f877	={PIC_16CXX,0x3FFF,2,1,0x2000,0x2007,0x2000},
	pic16c923	={PIC_16CXX,0x3FFF,2,1,0x1000,0x2007,0x2000},
	pic16c924	={PIC_16CXX,0x3FFF,2,1,0x1000,0x2007,0x2000},
	pic12f629	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic12f675	={PIC_16CXX,0x3FFF,1,0,0x0400,0x2007,0x2000},
	pic12f683	={PIC_16CXX,0x3FFF,1,0,0x0800,0x2007,0x2000},

	pic17c42	={PIC_17CXX,0xFFFF,0,0,0x0800,0xFE00,0x0000},
	pic17c43	={PIC_17CXX,0xFFFF,4,4,0x1000,0xFE00,0x0000},
	pic17c44	={PIC_17CXX,0xFFFF,4,4,0x2000,0xFE00,0x0000},
	pic17c52	={PIC_17CXX,0xFFFF,4,4,0x2000,0xFE00,0x0000},
	pic17c56	={PIC_17CXX,0xFFFF,4,4,0x4000,0xFE00,0x0000};

// processors handled here (the constuctors for these variables link them to the global
// list of processors that the assembler knows how to handle)

static PROCESSOR_FAMILY
	processorFamily("Microchip PIC",InitFamily,UnInitFamily,SelectProcessor,DeselectProcessor,AttemptPseudoOpcode,AttemptOpcode);


static PROCESSOR
	processors[]=
	{
// 16C5x
		PROCESSOR(&processorFamily,	"12c508"	,&pic12c508),
		PROCESSOR(&processorFamily,	"12c508a"	,&pic12c508),
		PROCESSOR(&processorFamily,	"12c509"	,&pic12c509),
		PROCESSOR(&processorFamily,	"12c509a"	,&pic12c509),
		PROCESSOR(&processorFamily,	"12ce518"	,&pic12ce518),
		PROCESSOR(&processorFamily,	"12ce519"	,&pic12ce519),
		PROCESSOR(&processorFamily,	"12c671"	,&pic12c671),
		PROCESSOR(&processorFamily,	"12c672"	,&pic12c672),
		PROCESSOR(&processorFamily,	"16c505"	,&pic16c505),
		PROCESSOR(&processorFamily,	"16c52"		,&pic16c52),
		PROCESSOR(&processorFamily,	"16c54"		,&pic16c54),
		PROCESSOR(&processorFamily,	"16c55"		,&pic16c55),
		PROCESSOR(&processorFamily,	"16c56"		,&pic16c56),
		PROCESSOR(&processorFamily,	"16c57"		,&pic16c57),
		PROCESSOR(&processorFamily,	"16c58"		,&pic16c58),
// 16Cxx
		PROCESSOR(&processorFamily,	"14000"		,&pic14000),
		PROCESSOR(&processorFamily,	"16c554"	,&pic16c554),
		PROCESSOR(&processorFamily,	"16c556"	,&pic16c556),
		PROCESSOR(&processorFamily,	"16c558"	,&pic16c558),
		PROCESSOR(&processorFamily,	"16c61"		,&pic16c61),
		PROCESSOR(&processorFamily,	"16c62"		,&pic16c62),
		PROCESSOR(&processorFamily,	"16c62a"	,&pic16c62),
		PROCESSOR(&processorFamily,	"16c62b"	,&pic16c62),
		PROCESSOR(&processorFamily,	"16c620"	,&pic16c620),
		PROCESSOR(&processorFamily,	"16c620a"	,&pic16c620),
		PROCESSOR(&processorFamily,	"16c621"	,&pic16c621),
		PROCESSOR(&processorFamily,	"16c621a"	,&pic16c621),
		PROCESSOR(&processorFamily,	"16c622"	,&pic16c622),
		PROCESSOR(&processorFamily,	"16c622a"	,&pic16c622),
		PROCESSOR(&processorFamily,	"16c63"		,&pic16c63),
		PROCESSOR(&processorFamily,	"16c63a"	,&pic16c63),
		PROCESSOR(&processorFamily,	"16c64"		,&pic16c64),
		PROCESSOR(&processorFamily,	"16c64a"	,&pic16c64),
		PROCESSOR(&processorFamily,	"16c642"	,&pic16c642),
		PROCESSOR(&processorFamily,	"16c65"		,&pic16c65),
		PROCESSOR(&processorFamily,	"16c65a"	,&pic16c65),
		PROCESSOR(&processorFamily,	"16c65b"	,&pic16c65),
		PROCESSOR(&processorFamily,	"16c66"		,&pic16c66),
		PROCESSOR(&processorFamily,	"16c662"	,&pic16c662),
		PROCESSOR(&processorFamily,	"16c67"		,&pic16c67),
		PROCESSOR(&processorFamily,	"16c71"		,&pic16c71),
		PROCESSOR(&processorFamily,	"16c71a"	,&pic16c71),
		PROCESSOR(&processorFamily,	"16c710"	,&pic16c710),
		PROCESSOR(&processorFamily,	"16c711"	,&pic16c711),
		PROCESSOR(&processorFamily,	"16c715"	,&pic16c715),
		PROCESSOR(&processorFamily,	"16c72"		,&pic16c72),
		PROCESSOR(&processorFamily,	"16c72a"	,&pic16c72),
		PROCESSOR(&processorFamily,	"16c73"		,&pic16c73),
		PROCESSOR(&processorFamily,	"16c73a"	,&pic16c73),
		PROCESSOR(&processorFamily,	"16c73b"	,&pic16c73),
		PROCESSOR(&processorFamily,	"16c74"		,&pic16c74),
		PROCESSOR(&processorFamily,	"16c74a"	,&pic16c74),
		PROCESSOR(&processorFamily,	"16c74b"	,&pic16c74),
		PROCESSOR(&processorFamily,	"16c76"		,&pic16c76),
		PROCESSOR(&processorFamily,	"16c77"		,&pic16c77),
		PROCESSOR(&processorFamily,	"16f627"	,&pic16f627),
		PROCESSOR(&processorFamily,	"16f627a"	,&pic16f627),
		PROCESSOR(&processorFamily,	"16f628"	,&pic16f628),
		PROCESSOR(&processorFamily,	"16f628a"	,&pic16f628),
		PROCESSOR(&processorFamily,	"16f648"	,&pic16f648),
		PROCESSOR(&processorFamily,	"16f648a"	,&pic16f648),
		PROCESSOR(&processorFamily,	"16f818"	,&pic16f818),
		PROCESSOR(&processorFamily,	"16f819"	,&pic16f819),
		PROCESSOR(&processorFamily,	"16f83"		,&pic16f83),
		PROCESSOR(&processorFamily,	"16c84"		,&pic16c84),
		PROCESSOR(&processorFamily,	"16f84"		,&pic16f84),
		PROCESSOR(&processorFamily,	"16f84a"	,&pic16f84),
		PROCESSOR(&processorFamily,	"16f873"	,&pic16f873),
		PROCESSOR(&processorFamily,	"16f874"	,&pic16f874),
		PROCESSOR(&processorFamily,	"16f876"	,&pic16f876),
		PROCESSOR(&processorFamily,	"16f877"	,&pic16f877),
		PROCESSOR(&processorFamily,	"16c923"	,&pic16c923),
		PROCESSOR(&processorFamily,	"16c924"	,&pic16c924),
		PROCESSOR(&processorFamily,	"12f629"	,&pic12f629),
		PROCESSOR(&processorFamily,	"12f675"	,&pic12f675),
		PROCESSOR(&processorFamily,	"12f683"	,&pic12f683),
// 17CXX
		PROCESSOR(&processorFamily,	"17c42"		,&pic17c42),
		PROCESSOR(&processorFamily,	"17c43"		,&pic17c43),
		PROCESSOR(&processorFamily,	"17c44"		,&pic17c44),
		PROCESSOR(&processorFamily,	"17c52"		,&pic17c52),
		PROCESSOR(&processorFamily,	"17c56"		,&pic17c56),
};
