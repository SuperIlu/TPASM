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


void ReportDisallowedLabel(const PARSED_LABEL *lineLabel);
void ReportBadOperands();
bool CheckSignedByteRange(int value,bool generateMessage,bool isError);
bool CheckUnsignedByteRange(int value,bool generateMessage,bool isError);
bool CheckByteRange(int value,bool generateMessage,bool isError);
bool CheckSignedWordRange(int value,bool generateMessage,bool isError);
bool CheckUnsignedWordRange(int value,bool generateMessage,bool isError);
bool CheckWordRange(int value,bool generateMessage,bool isError);
bool Check8BitIndexRange(int value,bool generateMessage,bool isError);
bool Check16BitIndexRange(int value,bool generateMessage,bool isError);
bool Check32BitIndexRange(int value,bool generateMessage,bool isError);
bool Check8RelativeRange(int value,bool generateMessage,bool isError);
bool Check16RelativeRange(int value,bool generateMessage,bool isError);
bool GenerateByte(unsigned char value,LISTING_RECORD *listingRecord);
bool GenerateWord(unsigned int value,LISTING_RECORD *listingRecord,bool bigEndian);
bool HandleDB(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
bool HandleLEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
bool HandleBEDW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
bool HandleDS(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
bool HandleDSW(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
bool HandleIncbin(const char *opcodeName,const char *line,unsigned int *lineIndex,const PARSED_LABEL *lineLabel,LISTING_RECORD *listingRecord);
