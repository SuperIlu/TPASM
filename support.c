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


// Support routines used by numerous processors to help keep the
// amount of duplicated code to a minimum

#include	"include.h"

void ReportDisallowedLabel(const PARSED_LABEL *lineLabel)
// complain that a label is not allowed on this op
{
	AssemblyComplaint(NULL,false,"Label '%s' not allowed here\n",lineLabel->name);
}

void ReportBadOperands()
// complain that the operands are bad
{
	AssemblyComplaint(NULL,true,"Invalid operands\n");
}

bool CheckSignedByteRange(int value,bool generateMessage,bool isError)
// see if value looks like a signed byte
// if not, return false, and generate a warning or error if told to
{
	if(value>=-128&&value<128)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Signed byte value (0x%X) out of range\n",value);
	}
	return(false);
}

bool CheckUnsignedByteRange(int value,bool generateMessage,bool isError)
// see if value looks like an unsigned byte
// if not, return false, and generate a warning or error if told to
{
	if(value>=0&&value<256)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Unsigned byte value (0x%X) out of range\n",value);
	}
	return(false);
}

bool CheckByteRange(int value,bool generateMessage,bool isError)
// see if value looks like a signed byte, or an unsigned byte
// if not, return false, and generate a warning or error if told to
{
	if(value>=-128&&value<256)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Byte value (0x%X) out of range\n",value);
	}
	return(false);
}

bool CheckSignedWordRange(int value,bool generateMessage,bool isError)
// see if value looks like a signed word
// if not, return false, and generate a warning or error if told to
{
	if(value>=-32768&&value<32768)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Signed word value (0x%X) out of range\n",value);
	}
	return(false);
}

bool CheckUnsignedWordRange(int value,bool generateMessage,bool isError)
// see if value looks like an unsigned word
// if not, return false, and generate a warning or error if told to
{
	if(value>=0&&value<65536)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Unsigned word value (0x%X) out of range\n",value);
	}
	return(false);
}

bool CheckWordRange(int value,bool generateMessage,bool isError)
// see if value looks like a signed word, or an unsigned word
// if not, return false, and generate a warning or error if told to
{
	if(value>=-32768&&value<65536)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Word value (0x%X) out of range\n",value);
	}
	return(false);
}

bool Check8BitIndexRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as 8 bit index
// complain if not
{
	if(value>=0&&value<8)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Bit index (0x%X) out of range\n",value);
	}
	return(false);
}

bool Check16BitIndexRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as 16 bit index
// complain if not
{
	if(value>=0&&value<16)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Bit index (0x%X) out of range\n",value);
	}
	return(false);
}

bool Check32BitIndexRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as 32 bit index
// complain if not
{
	if(value>=0&&value<32)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Bit index (0x%X) out of range\n",value);
	}
	return(false);
}

bool Check8RelativeRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as 8 bit relative offset
// complain if not
{
	if(value>=-128&&value<128)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Relative offset (0x%X) out of range\n",value);
	}
	return(false);
}

bool Check16RelativeRange(int value,bool generateMessage,bool isError)
// check value to see if it looks good as 16 bit relative offset
// complain if not
{
	if(value>=-32768&&value<32768)
	{
		return(true);
	}
	if(generateMessage)
	{
		AssemblyComplaint(NULL,isError,"Relative offset (0x%X) out of range\n",value);
	}
	return(false);
}

bool GenerateByte(unsigned char value,LISTING_RECORD *listingRecord)
// output the value to the current segment
// This will return false only if a "hard" error occurs
{
	bool
		fail;
	unsigned int
		length;

	fail=false;
	if(currentSegment)
	{
		if(!intermediatePass)				// only do the real work if necessary
		{
			length=strlen(listingRecord->listObjectString);
			if(length+4<MAX_STRING)
			{
				sprintf(&listingRecord->listObjectString[length],"%02X ",value);		// create list file output
			}
			fail=!AddBytesToSegment(currentSegment,currentSegment->currentPC,&value,1);
		}
		currentSegment->currentPC++;
	}
	else
	{
		AssemblyComplaint(NULL,true,"Code cannot occur outside of a segment\n");
	}
	return(!fail);
}

bool GenerateWord(unsigned int value,LISTING_RECORD *listingRecord,bool bigEndian)
// output the value to the current segment as a word in the given endian.
// This will return false only if a "hard" error occurs
{
	if(bigEndian)
	{
		if(GenerateByte(value>>8,listingRecord))
		{
			return(GenerateByte(value&0xFF,listingRecord));
		}
	}
	else
	{
		if(GenerateByte(value&0xFF,listingRecord))
		{
			return(GenerateByte(value>>8,listingRecord));
		}
	}
	return(false);
}

bool HandleDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring bytes of data
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
				fail=!GenerateByte(outputString[i],listingRecord);
				i++;
			}
		}
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckByteRange(value,true,true);
			fail=!GenerateByte(value&0xFF,listingRecord);
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

static bool HandleDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord,bool bigEndian)
// Declare words of data in the given endian
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
				fail=!GenerateWord(outputString[i],listingRecord,bigEndian);
				i++;
			}
		}
		else if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			CheckWordRange(value,true,true);
			fail=!GenerateWord(value,listingRecord,bigEndian);
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

bool HandleLEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring words of data in little endian
{
	return(HandleDW(opcodeName,line,lineIndex,lineLabel,listingRecord,false));
}

bool HandleBEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// declaring words of data in big endian
{
	return(HandleDW(opcodeName,line,lineIndex,lineLabel,listingRecord,true));
}

bool HandleDS(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// move the PC forward by the given amount
{
	bool
		fail;
	int
		value;
	bool
		unresolved;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				if(currentSegment)
				{
					if(!unresolved)
					{
						if(!intermediatePass)							// only do the real work if necessary
						{
							fail=!AddSpaceToSegment(currentSegment,currentSegment->currentPC,value);
						}
						currentSegment->currentPC+=value;
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' outside of segment\n",opcodeName);
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

bool HandleDSW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// move the PC forward by the given amount times two (do as if skipping this many words)
{
	bool
		fail;
	int
		value;
	bool
		unresolved;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseExpression(line,lineIndex,&value,&unresolved))
		{
			if(ParseComment(line,lineIndex))
			{
				value*=2;
				if(currentSegment)
				{
					if(!unresolved)
					{
						if(!intermediatePass)							// only do the real work if necessary
						{
							fail=!AddSpaceToSegment(currentSegment,currentSegment->currentPC,value);
						}
						currentSegment->currentPC+=value;
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"'%s' outside of segment\n",opcodeName);
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

bool HandleIncbin(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord)
// include the data of the given file into the input stream
// If there is a problem, report it and return false
// NOTE: on _large_ incbins, the assembly can be slowed by
// trying to read through all the bytes each pass of the assembly.
// So, on all passes but the last, this simply gets the length of the
// file and pushes the PC forward.
{
	bool
		fail,
		done;
	char
		fileName[MAX_STRING];
	unsigned int
		stringLength;
	FILE
		*file;
	unsigned char
		inputDataBuffer[1024];
	int
		bytesRead;

	fail=!ProcessLineLocationLabel(lineLabel);		// deal with any label on the line
	if(!fail)
	{
		if(ParseQuotedString(line,lineIndex,'"','"',fileName,&stringLength))
		{
			if(ParseComment(line,lineIndex))							// make sure there's nothing else on the line
			{
				if(currentSegment)
				{
					if((file=fopen(fileName,"rb")))
					{
						if(intermediatePass)							// only do the real work if necessary
						{
							if(fseek(file,0,SEEK_END)==0)
							{
								currentSegment->currentPC+=ftell(file);
							}
							else
							{
								AssemblyComplaint(NULL,true,"Failed seeking '%s' to the end. %s\n",fileName,strerror(errno));
								fail=true;
							}
						}
						else
						{
							done=false;
							while(!fail&&!done)
							{
								bytesRead=fread(inputDataBuffer,1,1024,file);
								if(bytesRead>0)
								{
									fail=!AddBytesToSegment(currentSegment,currentSegment->currentPC,inputDataBuffer,bytesRead);
									currentSegment->currentPC+=bytesRead;
								}
								else if(bytesRead==0)
								{
									done=true;
								}
								else
								{
									AssemblyComplaint(NULL,true,"Failed reading file '%s'. %s\n",fileName,strerror(errno));
									fail=true;
								}
							}
						}
						fclose(file);
					}
					else
					{
						AssemblyComplaint(NULL,true,"Failed to open file '%s'. %s\n",fileName,strerror(errno));
					}
				}
				else
				{
					AssemblyComplaint(NULL,true,"Code cannot occur outside of a segment\n");
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Missing or incorrectly formatted file name\n");
			}
		}
		else
		{
			AssemblyComplaint(NULL,true,"Missing or incorrectly formatted file name\n");
		}
	}
	return(!fail);
}

