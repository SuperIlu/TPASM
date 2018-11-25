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

struct TOKEN						// token definition
{
	const char
		*token;
	unsigned int
		value;						// ID associated with token
};

bool IsStartLabelChar(char character);
bool IsLabelChar(char character);
const TOKEN *MatchBuriedToken(const char *string,unsigned int *index,const TOKEN *list);
bool SkipWhiteSpace(const char *line,unsigned int *lineIndex);
bool ParseComment(const char *line,unsigned int *lineIndex);
bool ParseNonWhiteSpace(const char *line,unsigned int *lineIndex,char *string);
bool ParseName(const char *line,unsigned int *lineIndex,char *string);
bool ParseNonName(const char *line,unsigned int *lineIndex,char *string);
bool ParseFunction(const char *line,unsigned int *lineIndex,char *functionName);
bool ParseLabelString(const char *line,unsigned int *lineIndex,char *label,bool *isLocal);
bool ParseLabel(const char *line,unsigned int *lineIndex,PARSED_LABEL *parsedLabel);
bool ParseQuotedString(const char *line,unsigned int *lineIndex,char startQuote,char endQuote,char *string,unsigned int *stringLength);
bool ParseQuotedStringVerbatim(const char *line,unsigned int *lineIndex,char startQuote,char endQuote,char *string);
bool ParseNumber(const char *line,unsigned int *lineIndex,int *value);
bool ParseParentheticString(const char *line,unsigned int *lineIndex,char *string);
bool ParseLabelDefinition(const char *line,unsigned int *lineIndex,PARSED_LABEL *parsedLabel);
bool ParseCommaSeparator(const char *line,unsigned int *lineIndex);
bool ParseFirstListElement(const char *line,unsigned int *lineIndex,char *element);
bool ParseNextListElement(const char *line,unsigned int *lineIndex,char *element);
bool ParseFirstNameElement(const char *line,unsigned int *lineIndex,char *element);
bool ParseNextNameElement(const char *line,unsigned int *lineIndex,char *element);
bool ProcessTextSubsitutions(char *outputLine,const char *inputLine,TEXT_BLOCK *labels,TEXT_BLOCK *substitutionText,bool *overflow);
