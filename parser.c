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

// Parse a line in various ways

// NOTE: All of the "Parse..." functions here will skip whitespace before
// they start looking for whatever it is they are meant to find. The
// caller can rely on this behavior. Even if a Parse function returns
// false, it is will have modified the line index to point past any
// whitespace that was at the start.

#include	"include.h"

static inline bool IsWhiteChar(char character)
// return true if character is white-space, false if not
{
	return(character==' '||character=='\t');
}

static inline bool IsCommaSeparator(char character)
// see if character is a list separator
{
	return(character==',');
}

bool IsStartLabelChar(char character)
// return true if character is a character that can be used at the start of a label
{
	return((character>='A'&&character<='Z')||(character>='a'&&character<='z')||(character=='_'));
}

bool IsLabelChar(char character)
// return true if character is a character that can be used anywhwere in a label
{
	return(IsStartLabelChar(character)||(character>='0'&&character<='9'));
}

static inline bool IsNameChar(char character)
// return true if character is alpha, numeric, dot, or underscore), false if not
{
	return((character>='A'&&character<='Z')||(character>='a'&&character<='z')||(character>='0'&&character<='9')||character=='.'||character=='_');
}

const TOKEN *MatchBuriedToken(const char *string,unsigned int *index,const TOKEN *list)
// Try to match characters of string starting at index against the list of tokens,
// if a match is made, move index to the end of the match in string,
// and return the matched token.
// if nothing is matched, return NULL
{
	unsigned int
		i,j;
	const char
		*stringA,
		*stringB;
	const TOKEN
		*match;

	match=NULL;
	i=0;
	while(!match&&(list[i].token[0]!='\0'))	// if nothing found yet, and not at end of list
	{
		stringA=&string[*index];
		stringB=&list[i].token[0];
		j=0;
		while((*stringA)&&((*stringA)==(*stringB)))
		{
			stringA++;
			stringB++;
			j++;
		}
		if(!(*stringB))			// got to end of token?
		{
			(*index)+=j;		// push forward in the string past the matched data
			match=&list[i];
		}
		i++;
	}
	return(match);
}

bool SkipWhiteSpace(const char *line,unsigned int *lineIndex)
// push lineIndex past any white space in line
// If there was no white space, return false
{
	if(IsWhiteChar(line[*lineIndex]))
	{
		while(IsWhiteChar(line[++(*lineIndex)]))
			;
		return(true);
	}
	return(false);
}

static inline bool IsCommentStart(const char *line,unsigned int *lineIndex)
// see if a comment is starting at the given location, or if the line is ending
{
	return(line[*lineIndex]=='\0'||line[*lineIndex]==';'||(line[*lineIndex]=='/'&&line[(*lineIndex)+1]=='/'));	// comment starting, or end of line?
}

bool ParseComment(const char *line,unsigned int *lineIndex)
// Return true if line is empty, or contains a comment.
// Do not push lineIndex past the start of the comment.
{
	SkipWhiteSpace(line,lineIndex);
	return(IsCommentStart(line,lineIndex));
}

bool ParseNonWhiteSpace(const char *line,unsigned int *lineIndex,char *string)
// Extract the next non-white, non-comment characters from the line and return them in string.
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	localIndex=*lineIndex;					// keep index here for a while
	outputIndex=0;
	while(!IsWhiteChar(line[localIndex])&&!IsCommentStart(line,&localIndex))	// get everything non-white and non-comment
	{
		string[outputIndex++]=line[localIndex++];
	}
	if(outputIndex)
	{
		string[outputIndex]='\0';
		*lineIndex=localIndex;
		return(true);
	}
	return(false);								// nothing
}

bool ParseName(const char *line,unsigned int *lineIndex,char *string)
// Parse sequences of name characters
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	localIndex=*lineIndex;					// keep index here for a while
	outputIndex=0;
	while(IsNameChar(line[localIndex])&&!IsCommentStart(line,&localIndex))
	{
		string[outputIndex++]=line[localIndex++];
	}
	if(outputIndex)
	{
		string[outputIndex]='\0';
		*lineIndex=localIndex;
		return(true);
	}
	return(false);								// nothing found
}

bool ParseNonName(const char *line,unsigned int *lineIndex,char *string)
// Extract the next non-white, non-comment, non-name sequence of characters.
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	localIndex=*lineIndex;					// keep index here for a while
	outputIndex=0;
	while(!IsWhiteChar(line[localIndex])&&!IsNameChar(line[localIndex])&&!IsCommentStart(line,&localIndex))
	{
		string[outputIndex++]=line[localIndex++];
	}
	if(outputIndex)
	{
		string[outputIndex]='\0';
		*lineIndex=localIndex;
		return(true);
	}
	return(false);								// nothing found
}

bool ParseFunction(const char *line,unsigned int *lineIndex,char *functionName)
// Attempt to parse a something which looks like a function
// call (label characters followed by an '(')
// leave lineIndex pushed past the last thing successfully parsed.
// If this finds something, it pushes lineIndex past the open paren,
// and collects the function name in functionName.
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	localIndex=*lineIndex;						// keep index here for a while
	outputIndex=0;
	if(IsStartLabelChar(line[localIndex]))		// verify that the first character looks like a label
	{
		do
		{
			functionName[outputIndex++]=line[localIndex++];
		}
		while(IsLabelChar(line[localIndex]));
		functionName[outputIndex]='\0';			// terminate the function name just collected
		SkipWhiteSpace(line,&localIndex);		// move past any white space
		if(line[localIndex]=='(')				// open paren (function starting?)
		{
			localIndex++;
			*lineIndex=localIndex;				// update output index
			return(true);						// return the label
		}
	}
	return(false);
}

bool ParseLabelString(const char *line,unsigned int *lineIndex,char *label,bool *isLocal)
// Attempt to parse a label from line at lineIndex.
// Leave lineIndex pushed past the last thing successfully parsed
// If the label is recognized as local (because it begins with a '.' or an '@'),
// this will adjust it for the current scope.
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	localIndex=*lineIndex;					// keep index here for a while
	outputIndex=0;
	*isLocal=false;
	if(line[localIndex]=='.')				// see if local label
	{
		sprintf(label,"%s@",scope);
		outputIndex=strlen(label);			// move to the end
		localIndex++;
		*isLocal=true;
	}
	else if(line[localIndex]=='@')			// see if macro local label
	{
		sprintf(label,"%s@%d@",scope,scopeValue);
		outputIndex=strlen(label);			// move to the end
		localIndex++;
		*isLocal=true;
	}
	if(IsStartLabelChar(line[localIndex]))	// verify that the first character looks like a label
	{
		do
		{
			label[outputIndex++]=line[localIndex++];
		}
		while(IsLabelChar(line[localIndex]));
		label[outputIndex]='\0';			// terminate the label we just collected
		*lineIndex=localIndex;				// update output index
		return(true);						// return the label
	}
	return(false);							// no label found
}

bool ParseLabel(const char *line,unsigned int *lineIndex,PARSED_LABEL *parsedLabel)
// Attempt to parse a label from line at lineIndex, filling in parsedLabel.
// Leave lineIndex pushed past the last thing successfully parsed
// If the label is recognized as local (because it begins with a '.' or an '@'),
// this will adjust it for the current scope.
{
	return(ParseLabelString(line,lineIndex,&parsedLabel->name[0],&parsedLabel->isLocal));
}

static bool GetRadixDigitValue(char digit,unsigned int radix,int *value)
// Return value as a binary representation of the value of digit given in
// radix
// NOTE: if digit is not valid for radix, return false
{
	if(digit>='a')
	{
		digit-='a'-'A';		// pull back to upper case
	}
	if((digit>='0'&&digit<='9')||(digit>='A'&&digit<='Z'))
	{
		digit-='0';			// make binary
		if(digit>9)
		{
			digit-='A'-'0'-10;
		}
		if(digit<(int)radix)
		{
			*value=digit;
			return(true);
		}
	}
	return(false);
}

static bool ParseRadixNumber(const char *line,unsigned int *lineIndex,unsigned int radix,int *value)
// read at least one digit in the given radix, assemble it into outValue
// if no digits are located, return false
{
	unsigned int
		numDigits;
	int
		outDigit;
	char
		inChar;

	SkipWhiteSpace(line,lineIndex);
	*value=0;
	numDigits=0;
	while((inChar=line[*lineIndex])&&GetRadixDigitValue(inChar,radix,&outDigit))
	{
		(*lineIndex)++;
		(*value)=((*value)*radix)+outDigit;
		numDigits++;
	}
	if(numDigits)
	{
		return(true);
	}
	return(false);
}

static bool ParseQuotedNumber(const char *line,unsigned int *lineIndex,unsigned int radix,int *value)
// a quoted number with a given radix is arriving, so parse it into element
// if there is a problem, return false
{
	unsigned int
		inputIndex;

	SkipWhiteSpace(line,lineIndex);
	inputIndex=*lineIndex;
	if(line[inputIndex]=='\'')
	{
		inputIndex++;
		if(ParseRadixNumber(line,&inputIndex,radix,value))
		{
			if(line[inputIndex]=='\'')
			{
				*lineIndex=inputIndex+1;
				return(true);
			}
		}
	}
	return(false);
}

static bool ParseDecimalOrPostfixNumber(const char *line,unsigned int *lineIndex,int *value)
// The next thing on the line should be interpreted as a decimal number,
// or a number with a postfix radix specifier.
// parse it into value
// if there is a problem (does not look like a proper decimal or postfix number), return false
{
	unsigned int
		inputIndex;
	bool
		done;
	unsigned int
		i;
	unsigned int
		radix;

	SkipWhiteSpace(line,lineIndex);
	inputIndex=*lineIndex;
	// first run to the end of the string of digits and letters, attempting to locate
	// a radix specifier
	i=*lineIndex;
	done=false;
	while(!done)
	{
		if((line[i]>='0'&&line[i]<='9')||											// decimal digit
			((line[i]>='A'&&line[i]<='F')||(line[i]>='a'&&line[i]<='f'))||	// hex digit, or binary or decimal specifier
			(line[i]=='H'||line[i]=='h'||line[i]=='O'||line[i]=='o'))		// hex specifier, or octal specifier
		{
			i++;		// step over it
		}
		else
		{
			done=true;	// something which is none of the above
		}
	}

	if(line[i-1]>='0'&&line[i-1]<='9')	// raw decimal number
	{
		if(ParseRadixNumber(line,&inputIndex,10,value))	// fetch in the decimal number
		{
			if(inputIndex==i)					// verify we have read all the digits (nothing funky in-between)
			{
				*lineIndex=inputIndex;			// push to the end of the number
				return(true);
			}
		}
	}
	else
	{
		switch(line[i-1])	// look at the specifier
		{
			case 'b':
			case 'B':
				radix=2;
				break;
			case 'o':
			case 'O':
				radix=8;
				break;
			case 'd':
			case 'D':
				radix=10;
				break;
			case 'h':
			case 'H':
				radix=16;
				break;
			default:
				return(false);	// something other than a digit or specifier at the end of the string
				break;
		}
		if(ParseRadixNumber(line,&inputIndex,radix,value))	// fetch in the number
		{
			if(inputIndex==(i-1))			// verify we have read up until the radix specifier (nothing funky in between)
			{
				*lineIndex=inputIndex+1;	// push past the specifier
				return(true);
			}
		}
	}
	return(false);
}

bool ParseQuotedString(const char *line,unsigned int *lineIndex,char startQuote,char endQuote,char *string,unsigned int *stringLength)
// Parse a quoted string (if none can be found, return false)
// NOTE: if a quoted string contains \'s, they will quote the next character, and possibly interpret it
// \a, \b, \f, \n, \r, \t, \v. \x## and \0## are interpreted
// NOTE: the outer quotes are not returned in string
// NOTE: since the string could contain NULs, the length is returned
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;
	int
		digitValue,
		value;

	SkipWhiteSpace(line,lineIndex);
	outputIndex=0;
	if(line[*lineIndex]==startQuote)
	{
		localIndex=(*lineIndex)+1;				// skip over the start quote
		while((line[localIndex]!=endQuote)&&line[localIndex])
		{
			if(line[localIndex]!='\\')
			{
				string[outputIndex++]=line[localIndex++];
			}
			else
			{
				localIndex++;					// skip over the '\'
				if(line[localIndex])
				{
					switch(line[localIndex])
					{
						case 'a':							// alert
							string[outputIndex++]='\a';
							localIndex++;
							break;
						case 'b':							// backspace
							string[outputIndex++]='\b';
							localIndex++;
							break;
						case 'f':							// form feed
							string[outputIndex++]='\f';
							localIndex++;
							break;
						case 'n':							// newline
							string[outputIndex++]='\n';
							localIndex++;
							break;
						case 'r':							// return
							string[outputIndex++]='\r';
							localIndex++;
							break;
						case 't':							// tab
							string[outputIndex++]='\t';
							localIndex++;
							break;
						case 'v':							// vertical tab
							string[outputIndex++]='\v';
							localIndex++;
							break;
						case 'x':							// hex number
							localIndex++;					// skip the introduction
							value=0;
							while(line[localIndex]&&GetRadixDigitValue(line[localIndex],16,&digitValue))
							{
								localIndex++;
								value=value*16+digitValue;
							}
							string[outputIndex++]=value&0xFF;
							break;
						case '0':							// octal number
							localIndex++;					// skip the introduction
							value=0;
							while(line[localIndex]&&GetRadixDigitValue(line[localIndex],8,&digitValue))
							{
								localIndex++;
								value=value*8+digitValue;
							}
							string[outputIndex++]=value&0xFF;
							break;
						default:
							string[outputIndex++]=line[localIndex++];
							break;
					}
				}
			}
		}
		if(line[localIndex]==endQuote)		// make sure it ended
		{
			string[outputIndex]='\0';		// terminate the string
			*stringLength=outputIndex;			// return the length (excluding the termination we added)
			*lineIndex=localIndex+1;			// skip over the terminating quote
			return(true);
		}
	}
	return(false);								// none found
}

bool ParseQuotedStringVerbatim(const char *line,unsigned int *lineIndex,char startQuote,char endQuote,char *string)
// Parse a quoted string (if none can be found, return false)
// NOTE: if a quoted string contains \'s, they will quote the next character, but will be left in
// the output, along with an unmodified next character
// NOTE: the outer quotes are not returned in string
{
	unsigned int
		outputIndex;
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	outputIndex=0;
	if(line[*lineIndex]==startQuote)
	{
		localIndex=(*lineIndex)+1;				// skip over the start quote
		while((line[localIndex]!=endQuote)&&line[localIndex])
		{
			string[outputIndex++]=line[localIndex++];
			if(line[localIndex-1]=='\\')
			{
				if(line[localIndex])
				{
					string[outputIndex++]=line[localIndex++];
				}
			}
		}
		if(line[localIndex]==endQuote)		// make sure it ended
		{
			string[outputIndex]='\0';		// terminate the string
			*lineIndex=localIndex+1;			// skip over the terminating quote
			return(true);
		}
	}
	return(false);								// none found
}

static bool ParseASCIIConstant(const char *line,unsigned int *lineIndex,int *value)
// parse ascii data into a number as an ascii constant
// if there is a problem, return false
{
	unsigned int
		inputIndex;
	unsigned int
		i;
	char
		string[MAX_STRING];
	unsigned int
		stringLength;

	inputIndex=*lineIndex;
	if(ParseQuotedString(line,&inputIndex,'\'','\'',string,&stringLength))
	{
		(*value)=0;
		i=0;
		while(i<stringLength)
		{
			(*value)<<=8;
			(*value)|=string[i];
			i++;
		}
		*lineIndex=inputIndex;
		return(true);
	}
	return(false);
}

bool ParseNumber(const char *line,unsigned int *lineIndex,int *value)
// Skip white space and attempt to parse a number from the line
// The following forms are supported:
//
// ### decimal
// 'cccc' for ascii
//
// C style
// 0b### binary
// 0o### octal
// 0d### decimal
// 0x### hex
//
// Microchip style
// A'###' ascii
// B'###' binary
// O'###' octal
// D'###' decimal
// H'###' hex
//
// Intel style
// ###b binary
// ###B binary
// ###o octal
// ###O octal
// ###d decimal
// ###D decimal
// ###h hex (first digit must be 0-9)
// ###H hex (first digit must be 0-9)
//
// Motorola style
// %### binary
// .### decimal
// $### hex
//
// return the number found with lineIndex updated, or false if none located
{
	unsigned int
		localIndex;

	SkipWhiteSpace(line,lineIndex);
	switch(line[*lineIndex])
	{
		case '0':				// leading 0?
			switch(line[(*lineIndex)+1])
			{
				case 'b':	// could be 0b (0 in binary), OR 0b1101 (binary 1101) OR 0b3cFh (0b3cf in hex)
					if(ParseDecimalOrPostfixNumber(line,lineIndex,value))
					{
						return(true);
					}
					else
					{
						localIndex=(*lineIndex)+2;	// skip over the prefix
						if(ParseRadixNumber(line,&localIndex,2,value))			// try it as binary
						{
							(*lineIndex)=localIndex;
							return(true);
						}
						else
						{
							return(false);
						}
					}
					break;
				case 'o':	// could be 0o (0 in octal), OR 0o777 (octal 777)
					if(ParseDecimalOrPostfixNumber(line,lineIndex,value))
					{
						return(true);
					}
					else
					{
						localIndex=(*lineIndex)+2;	// skip over the prefix
						if(ParseRadixNumber(line,&localIndex,8,value))		// try it as octal
						{
							(*lineIndex)=localIndex;
							return(true);
						}
						else
						{
							return(false);
						}
					}
					break;
				case 'd':	// could be 0d (0 in decimal), OR 0d1234 (decimal 1234) OR 0d45h (0d45 in hex)
					if(ParseDecimalOrPostfixNumber(line,lineIndex,value))
					{
						return(true);
					}
					else
					{
						localIndex=(*lineIndex)+2;	// skip over the prefix
						if(ParseRadixNumber(line,&localIndex,10,value))		// try it as decimal
						{
							(*lineIndex)=localIndex;
							return(true);
						}
						else
						{
							return(false);
						}
					}
					break;
				case 'x':
					localIndex=(*lineIndex)+2;		// skip over the prefix
					if(ParseRadixNumber(line,&localIndex,16,value))		// try it as hex
					{
						(*lineIndex)=localIndex;
						return(true);
					}
					else
					{
						return(false);
					}
					break;
				default:
					return(ParseDecimalOrPostfixNumber(line,lineIndex,value));
					break;
			}
			break;
		case 'A':
			if(line[(*lineIndex)+1]=='\'')
			{
				localIndex=(*lineIndex)+1;		// skip over the prefix
				if(ParseASCIIConstant(line,&localIndex,value))	// see if we actually got a constant
				{
					*lineIndex=localIndex;
					return(true);
				}
				return(false);
			}
			break;
		case 'B':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseQuotedNumber(line,&localIndex,2,value))
			{
				*lineIndex=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case 'O':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseQuotedNumber(line,&localIndex,8,value))
			{
				*lineIndex=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case 'D':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseQuotedNumber(line,&localIndex,10,value))
			{
				*lineIndex=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case 'H':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseQuotedNumber(line,&localIndex,16,value))
			{
				*lineIndex=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case '$':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseRadixNumber(line,&localIndex,16,value))		// try it as hex
			{
				(*lineIndex)=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case '.':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseRadixNumber(line,&localIndex,10,value))		// try it as decimal
			{
				(*lineIndex)=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case '%':
			localIndex=(*lineIndex)+1;		// skip over the prefix
			if(ParseRadixNumber(line,&localIndex,2,value))		// try it as binary
			{
				(*lineIndex)=localIndex;
				return(true);
			}
			else
			{
				return(false);
			}
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return(ParseDecimalOrPostfixNumber(line,lineIndex,value));
			break;
		case '\'':
			return(ParseASCIIConstant(line,lineIndex,value));
			break;
	}
	return(false);
}

bool ParseParentheticString(const char *line,unsigned int *lineIndex,char *string)
// Parse out a string which is enclosed in parenthesis
// line must begin with an open paren, and will be
// placed into string until a matching close paren is seen
// if a comment, this will stop parsing, and
// return false
// NOTE: this understands that there may be quoted material between the
// parenthesis, and abides by the quotes
// NOTE: the parenthesis that enclose the string are not included in
// the parsed output
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
	if(line[*lineIndex]=='(')				// open paren?
	{
		parenDepth++;
		initialIndex=workingIndex=(*lineIndex)+1;
		while(parenDepth&&!ParseComment(line,&workingIndex))
		{
			switch(line[workingIndex])
			{
				case '(':
					parenDepth++;
					break;
				case ')':
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

bool ParseLabelDefinition(const char *line,unsigned int *lineIndex,PARSED_LABEL *parsedLabel)
// Attempt to parse a label from line at lineIndex, filling in parsedLabel.
// leave lineIndex pushed past the last thing successfully parsed
// If the label is recognized as local, this will adjust it for the
// current scope.
{
	if(ParseLabel(line,lineIndex,parsedLabel))
	{
		if(line[*lineIndex]==':')			// if there is a colon after the label, move past it
		{
			(*lineIndex)++;
		}
		return(true);							// return the label
	}
	return(false);								// no label found
}

bool ParseCommaSeparator(const char *line,unsigned int *lineIndex)
// Expect a comma, step over one if found
{
	SkipWhiteSpace(line,lineIndex);
	if(IsCommaSeparator(line[*lineIndex]))	// does this look like a separator?
	{
		(*lineIndex)++;							// step over it
		return(true);
	}
	return(false);
}

bool ParseFirstListElement(const char *line,unsigned int *lineIndex,char *element)
// Parse the first element from a comma separated list
// NOTE: a list element is anything which is non-white at the start,
// non-white at the end, and which is not a comma, or a comment character
// NOTE: this will pay attention to quotes in the list element, and WILL
// allow comment characters or comma's to be embedded in elements
// as long as they are quoted
// false is returned if no element could be located
// NOTE: if the list separator is encountered before any list
// element characters are seen, an empty element is returned
// NOTE: the list separator which marks the end of the element is NOT used up by
// this routine, and is left around for calls to ParseNextListElement
{
	bool
		done;
	char
		character;
	unsigned int
		outputIndex;

	outputIndex=0;
	if(!ParseComment(line,lineIndex))
	{
		done=false;
		while(!done&&!IsCommentStart(line,lineIndex))
		{
			character=line[*lineIndex];
			if(!IsCommaSeparator(character))
			{
				switch(character)
				{
					case '"':						// start of a double quoted string
					case '\'':						// start of a single quoted string
						element[outputIndex++]=character;
						if(ParseQuotedStringVerbatim(line,lineIndex,character,character,&element[outputIndex]))
						{
							outputIndex+=strlen(&element[outputIndex]);	// push forward
							element[outputIndex++]=character;
						}
						else
						{
							// no final matching quote, so just get it all
							(*lineIndex)++;
							while(line[*lineIndex])
							{
								element[outputIndex++]=line[(*lineIndex)++];
							}
						}
						break;

					case '\\':						// quoting the next single character?
						(*lineIndex)++;
						if(line[*lineIndex])		// if not end of line, then pull into the element
						{
							element[outputIndex++]=line[*lineIndex];
							(*lineIndex)++;
						}
						break;
					default:
						element[outputIndex++]=character;
						(*lineIndex)++;
						break;
				}
			}
			else
			{
				done=true;
			}
		}
		// remove white space from the end
		while(outputIndex&&IsWhiteChar(element[outputIndex-1]))
		{
			outputIndex--;
		}
		element[outputIndex]='\0';
		return(true);
	}
	return(false);
}

bool ParseNextListElement(const char *line,unsigned int *lineIndex,char *element)
// Expect a comma, and a list element
{
	if(!ParseComment(line,lineIndex))		// move past any white space, make sure we're not at the end
	{
		if(ParseCommaSeparator(line,lineIndex))	// make sure that a separator is the next thing we see
		{
			if(!ParseComment(line,lineIndex))	// see if at the end
			{
				return(ParseFirstListElement(line,lineIndex,element));
			}
			else
			{
				element[0]='\0';
				return(true);					// empty element at end of list
			}
		}
	}
	return(false);
}

bool ParseFirstNameElement(const char *line,unsigned int *lineIndex,char *element)
// Parse the first name element from a comma separated list
// NOTE: if the list separator is encountered before any name
// characters are seen, an empty element is returned
// NOTE: the list separator which marks the end of the element is NOT used up by
// this routine, and is left around for calls to ParseNextNameElement
{
	char
		character;
	unsigned int
		outputIndex;

	outputIndex=0;
	if(!ParseComment(line,lineIndex))
	{
		while(IsNameChar((character=line[*lineIndex])))
		{
			(*lineIndex)++;
			element[outputIndex++]=character;
		}
		element[outputIndex]='\0';
		return(outputIndex!=0);
	}
	return(false);
}

bool ParseNextNameElement(const char *line,unsigned int *lineIndex,char *element)
// Expect a comma, and a name element
{
	unsigned int
		currentIndex;

	if(!ParseComment(line,lineIndex))		// move past any white space, make sure we're not at the end
	{
		if(IsCommaSeparator(line[*lineIndex]))	// make sure that a separator is the next thing we see
		{
			currentIndex=(*lineIndex)+1;			// hold but do not step over the separator (just in case there is one at the end, do not move past it)
			if(ParseFirstNameElement(line,&currentIndex,element))
			{
				(*lineIndex)=currentIndex;
				return(true);
			}
		}
	}
	return(false);
}

// --------------------------------------------------------------------------------------------
// Text substitution
//

static inline int StringMatch(const char *lineString,const char *labelString)
// return the last non-zero matching index of lineString against labelString
{
	unsigned int
		index;

	index=0;
	while(lineString[index]&&labelString[index]&&(lineString[index]==labelString[index]))
	{
		index++;
	}
	return(index);
}

static inline void AddCharToOutput(char *outputLine,unsigned int *outputIndex,char character,bool *overflow)
// add a character to outputLine at outputIndex
// Check the index and keep it from overflowing
{
	if(*outputIndex<MAX_STRING)
	{
		outputLine[(*outputIndex)++]=character;
	}
	else
	{
		*overflow=true;
	}
}

static inline void AddStringToOutput(char *outputLine,unsigned int *outputIndex,const char *string,bool *overflow)
// copy string to outputLine at outputIndex
// update outputIndex
// Check the index and keep it from overflowing
{
	unsigned int
		length;

	length=strlen(string);
	if(length>(MAX_STRING-*outputIndex))
	{
		length=MAX_STRING-*outputIndex;
		*overflow=true;
	}
	strncpy(&outputLine[*outputIndex],string,length);
	(*outputIndex)+=length;
}

static inline void TerminateOutput(char *outputLine,unsigned int outputIndex,bool *overflow)
// drop a terminator at the end of the line
{
	if(outputIndex<MAX_STRING)
	{
		outputLine[outputIndex]='\0';
	}
	else
	{
		outputLine[MAX_STRING-1]='\0';
		*overflow=true;
	}
}

static bool AttemptSubstitution(char *outputLine,unsigned int *outputIndex,const char *inputLine,unsigned int *inputIndex,TEXT_BLOCK *labels,TEXT_BLOCK *substitutionText,bool *overflow)
// try to substitute text at the current indices
// if successful, then update the indicies, and return true
// otherwise, return false
{
	TEXT_LINE
		*currentLabel,
		*currentSubText;
	unsigned int
		matchIndex;

	currentLabel=labels->firstLine;
	currentSubText=substitutionText->firstLine;
	while(currentLabel)
	{
		matchIndex=StringMatch(&inputLine[*inputIndex],&currentLabel->line[0]);
		if(!currentLabel->line[matchIndex]&&!IsLabelChar(inputLine[(*inputIndex)+matchIndex]))		// found match?
		{
			if(currentSubText)
			{
				AddStringToOutput(outputLine,outputIndex,&currentSubText->line[0],overflow);	// blast in the substitution text
			}
			(*inputIndex)+=matchIndex;
			return(true);
		}
		else
		{
			currentLabel=currentLabel->next;
			if(currentSubText)
			{
				currentSubText=currentSubText->next;
			}
		}
	}
	return(false);
}

bool ProcessTextSubsitutions(char *outputLine,const char *inputLine,TEXT_BLOCK *labels,TEXT_BLOCK *substitutionText,bool *overflow)
// read inputLine, perform substitutions on it, create substituted text in
// outputLine which is at least MAX_STRING bytes long
// If there is a hard error, report it, and return false
// NOTE: the substitutions which are made here are very rudimentary:
// text which is not contained in quotes is searched to find matches within the
// label character space to the labels which are specified in "labels"
// if a match occurs, the label is replaced with the characters which are
// specified in the same relative position of substitutionText.
// The results are placed into output buffer.
// NOTE: if there is no matching substitution text for a given label,
// the label is removed from the input
// NOTE: this is very inefficient
{
	unsigned int
		inputIndex,
		outputIndex;
	char
		character;
	bool
		wasLabelChar;
	char
		parsedString[MAX_STRING];

	inputIndex=0;
	outputIndex=0;
	wasLabelChar=false;
	*overflow=false;
	while((character=inputLine[inputIndex]))
	{
		switch(character)
		{
			case '\\':			// single character quote? -- if so, the next character goes out without change
				inputIndex++;
				if((character=inputLine[inputIndex]))
				{
					AddCharToOutput(outputLine,&outputIndex,character,overflow);
					inputIndex++;
				}
				wasLabelChar=false;
				break;
			case '"':			// start of a double quoted string
			case '\'':			// start of a single quoted string
				AddCharToOutput(outputLine,&outputIndex,character,overflow);
				if(ParseQuotedStringVerbatim(inputLine,&inputIndex,character,character,&parsedString[0]))
				{
					AddStringToOutput(outputLine,&outputIndex,&parsedString[0],overflow);
					AddCharToOutput(outputLine,&outputIndex,character,overflow);
					wasLabelChar=false;
				}
				else
				{
					// failed to parse a string, so just copy the rest of the line and leave
					AddStringToOutput(outputLine,&outputIndex,&inputLine[inputIndex+1],overflow);
					TerminateOutput(outputLine,outputIndex,overflow);
					return(true);
				}
				break;

			default:
				if(!IsLabelChar(character))
				{
					AddCharToOutput(outputLine,&outputIndex,character,overflow);
					inputIndex++;
					wasLabelChar=false;
				}
				else
				{
					if(!wasLabelChar)
					{
						if(!AttemptSubstitution(outputLine,&outputIndex,inputLine,&inputIndex,labels,substitutionText,overflow))
						{
							AddCharToOutput(outputLine,&outputIndex,character,overflow);
							inputIndex++;
							wasLabelChar=true;
						}
					}
					else
					{
						AddCharToOutput(outputLine,&outputIndex,character,overflow);
						inputIndex++;
					}
				}
				break;
		}
	}
	TerminateOutput(outputLine,outputIndex,overflow);				// substitution complete
	return(true);
}
