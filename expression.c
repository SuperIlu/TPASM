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

// Expression handling
//
// @@@ this needs help (make functions take parameter lists, clean up
// expression handling to allow for hard errors, write various functions (substr, expr, etc..))
// ALSO, look more carefully at various "unresolved" cases as they may not be
// handled correctly.
// Allow equ and set to work with strings...
// Clean up "processor" pseudo-op to only take a string parameter
// Do same with "include" and others.... Make them all parse expressions.

#include	"include.h"

struct OPERATOR_DESCRIPTION				// tells how each operator is defined
{
	unsigned int
		precedence;						// keeps relative precedence of operators
	bool
		binaryOperator;					// set true if operator can be used as a binary operator
};

enum
{
	ET_NUM,								// immediate number
	ET_OPERATOR,						// expression operator
	ET_FUNCTION,						// expression function call
	ET_LABEL,							// label
	ET_LOCAL_LABEL,						// local label
	ET_STRING,							// immediate string
};

struct EXPRESSION_ELEMENT
{
	unsigned int
		type;
	int
		value;							// if element is numeric, the value is placed here. If element is operator, the token number is here
	char
		string[MAX_STRING];				// parsed string (function name, label name, or string)
};

enum
{
	OP_LOGICAL_NOT,
	OP_NOT,
	OP_LEFT_PAREN,
	OP_RIGHT_PAREN,
	OP_LOGICAL_OR,
	OP_LOGICAL_AND,
	OP_OR,
	OP_XOR,
	OP_AND,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_LESS_THAN,
	OP_LESS_OR_EQUAL,
	OP_GREATER_THAN,
	OP_GREATER_OR_EQUAL,
	OP_SHIFT_LEFT,
	OP_SHIFT_RIGHT,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_MOD,
};

// These descriptions must be aligned with the token values enumerated above
static const OPERATOR_DESCRIPTION
	opDescriptions[]=
	{
		{0,false},		// OP_LOGICAL_NOT
		{0,false},		// OP_NOT
		{0,false},		// OP_LEFT_PAREN
		{0,false},		// OP_RIGHT_PAREN
		{1,true},		// OP_LOGICAL_OR
		{2,true},		// OP_LOGICAL_AND
		{3,true},		// OP_OR
		{4,true},		// OP_XOR
		{5,true},		// OP_AND
		{6,true},		// OP_EQUAL
		{6,true},		// OP_NOT_EQUAL
		{7,true},		// OP_LESS_THAN
		{7,true},		// OP_LESS_OR_EQUAL
		{7,true},		// OP_GREATER_THAN
		{7,true},		// OP_GREATER_OR_EQUAL
		{8,true},		// OP_SHIFT_LEFT
		{8,true},		// OP_SHIFT_RIGHT
		{9,true},		// OP_ADD
		{9,true}, 		// OP_SUBTRACT - can be both binary and unary operator, but since it _can_ be used as binary, set binaryOperator true
		{10,true},		// OP_MULTIPLY
		{10,true},		// OP_DIVIDE
		{10,true},		// OP_MOD
	};

// NOTE: the tokens below need to be placed in order from
// longest to shortest to guarantee that the parser will extract
// the longest valid operator which is available
// NOTE: the precedence value tells which operators take precedence
// over others. All unary operators are assumed by the code to have
// the highest precedence. 
static const TOKEN
	opTokenList[] =
	{
		{"<<",OP_SHIFT_LEFT},
		{">>",OP_SHIFT_RIGHT},
		{"<=",OP_LESS_OR_EQUAL},
		{">=",OP_GREATER_OR_EQUAL},
		{"==",OP_EQUAL},
		{"!=",OP_NOT_EQUAL},
		{"&&",OP_LOGICAL_AND},
		{"||",OP_LOGICAL_OR},
		{"*",OP_MULTIPLY},
		{"/",OP_DIVIDE},
		{"%",OP_MOD},
		{"+",OP_ADD},
		{"-",OP_SUBTRACT},
		{"<",OP_LESS_THAN},
		{">",OP_GREATER_THAN},
		{"&",OP_AND},
		{"^",OP_XOR},
		{"|",OP_OR},
		{"!",OP_LOGICAL_NOT},
		{"~",OP_NOT},
		{"(",OP_LEFT_PAREN},
		{")",OP_RIGHT_PAREN},
		{"",0}
	};

static bool ParseExpressionNumber(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Attempt to read a number as the next element of an expression
{
	if(ParseNumber(line,lineIndex,&element->value))
	{
		element->type=ET_NUM;
		return(true);
	}
	return(false);
}

static bool ParseExpressionOperator(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Attempt to parse an expression operator from line at lineIndex.
// Return the operator found with lineIndex updated, or false if none located.
{
	const TOKEN
		*match;

	SkipWhiteSpace(line,lineIndex);
	if((match=MatchBuriedToken(line,lineIndex,opTokenList)))
	{
		element->type=ET_OPERATOR;
		element->value=match->value;
		return(true);
	}
	return(false);
}

static bool ParseExpressionConstant(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// See if the thing at lineIndex represents a constant we know about
{
	SkipWhiteSpace(line,lineIndex);
	switch(line[*lineIndex])
	{
		case '$':						// this means the current PC (one $ is current, $$ is current ignoring any relative origin)
			(*lineIndex)++;
			element->type=ET_NUM;
			if(line[*lineIndex]=='$')
			{
				(*lineIndex)++;
				if(currentSegment)
				{
					element->value=currentSegment->currentPC;
				}
				else
				{
					element->value=0;	// outside of segments, this is 0
				}
			}
			else
			{
				if(currentSegment)
				{
					element->value=currentSegment->currentPC+currentSegment->codeGenOffset;
				}
				else
				{
					element->value=0;	// outside of segments, this is 0
				}
			}
			return(true);
			break;
	}
	return(false);
}

static bool ParseExpressionFunction(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Attempt to parse a function call from the given line
{
	if(ParseFunction(line,lineIndex,&element->string[0]))
	{
		element->type=ET_FUNCTION;
		return(true);
	}
	return(false);
}

static bool ParseExpressionLabel(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Attempt to parse an expression element which is a label from the
// given line
{
	bool
		isLocal;

	if(ParseLabelString(line,lineIndex,&element->string[0],&isLocal))
	{
		if(isLocal)
		{
			element->type=ET_LOCAL_LABEL;
		}
		else
		{
			element->type=ET_LABEL;
		}
		return(true);
	}
	return(false);
}

static bool ParseExpressionString(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Attempt to parse an expression element which must be a quoted string
{
	unsigned int
		stringLength;

	if(ParseQuotedString(line,lineIndex,'"','"',&element->string[0],&stringLength))
	{
		element->type=ET_STRING;
		return(true);
	}
	return(false);
}

static bool ParseExpressionElementOperator(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Parse an element of an expression but only look for operators
// If no operator can be located (or a comment is found), then return false
// NOTE: this is useful to avoid certain ambiguities when parsing expressions:
// namely, the expression 1%1 will not be parsed correctly without it
{
	if(!ParseComment(line,lineIndex))		// if comment is hit, then done -- no more elements
	{
		if(ParseExpressionOperator(line,lineIndex,element))
		{
			return(true);
		}
	}
	return(false);								// none found
}

static bool ParseExpressionElement(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element)
// Parse an element of an expression (a label, number, or operator)
// If no element can be located (or a comment is found), then return false
{
	if(!ParseComment(line,lineIndex))		// if comment is hit, then done -- no more elements
	{
		if(ParseExpressionNumber(line,lineIndex,element))		// try to get a numeric value
		{
			return(true);
		}
		if(ParseExpressionOperator(line,lineIndex,element))	// see if we can locate an expression operator
		{
			return(true);
		}
		if(ParseExpressionConstant(line,lineIndex,element))	// try to get a constant we know about
		{
			return(true);
		}
		if(ParseExpressionFunction(line,lineIndex,element))	// look for a function call
		{
			return(true);
		}
		if(ParseExpressionLabel(line,lineIndex,element))		// try to get a label (convert local label if possible)
		{
			return(true);
		}
		if(ParseExpressionString(line,lineIndex,element))		// try to get a string
		{
			return(true);
		}
	}
	return(false);								// none found
}

static bool ProcessFunction(char *functionName,EXPRESSION_LIST_ITEM *expressionListItem,EXPRESSION_LIST_ITEM *parameterExpressionListItem)
// Process the function
// @@@ lazy -- make this better
{
	bool
		fail;

	fail=false;
	if(strcmp(functionName,"strlen")==0)
	{
		expressionListItem->itemType=ELI_INTEGER;
		expressionListItem->itemValue=0;
		expressionListItem->valueResolved=false;
		if(parameterExpressionListItem->itemType!=ELI_UNRESOLVED)
		{
			if(parameterExpressionListItem->itemType==ELI_STRING)
			{
				expressionListItem->itemValue=strlen(parameterExpressionListItem->itemString);
				expressionListItem->valueResolved=true;
			}
			else
			{
				AssemblyComplaint(NULL,true,"%s: only works on strings\n",functionName);
				fail=true;
			}
		}
	}
	else if(strcmp(functionName,"high")==0)
	{
		expressionListItem->itemType=ELI_INTEGER;
		expressionListItem->itemValue=0;
		expressionListItem->valueResolved=false;
		if(parameterExpressionListItem->itemType!=ELI_UNRESOLVED)
		{
			if(parameterExpressionListItem->itemType==ELI_INTEGER)
			{
				expressionListItem->itemValue=(parameterExpressionListItem->itemValue>>8)&0xFF;
				expressionListItem->valueResolved=true;
			}
			else
			{
				AssemblyComplaint(NULL,true,"%s: only works on integers\n",functionName);
				fail=true;
			}
		}
	}
	else if(strcmp(functionName,"low")==0)
	{
		expressionListItem->itemType=ELI_INTEGER;
		expressionListItem->itemValue=0;
		expressionListItem->valueResolved=false;
		if(parameterExpressionListItem->itemType!=ELI_UNRESOLVED)
		{
			if(parameterExpressionListItem->itemType==ELI_INTEGER)
			{
				expressionListItem->itemValue=(parameterExpressionListItem->itemValue)&0xFF;
				expressionListItem->valueResolved=true;
			}
			else
			{
				AssemblyComplaint(NULL,true,"%s: only works on integers\n",functionName);
				fail=true;
			}
		}
	}
	else if(strcmp(functionName,"msw")==0)
	{
		expressionListItem->itemType=ELI_INTEGER;
		expressionListItem->itemValue=0;
		expressionListItem->valueResolved=false;
		if(parameterExpressionListItem->itemType!=ELI_UNRESOLVED)
		{
			if(parameterExpressionListItem->itemType==ELI_INTEGER)
			{
				expressionListItem->itemValue=(parameterExpressionListItem->itemValue>>16)&0xFFFF;
				expressionListItem->valueResolved=true;
			}
			else
			{
				AssemblyComplaint(NULL,true,"%s: only works on integers\n",functionName);
				fail=true;
			}
		}
	}
	else if(strcmp(functionName,"lsw")==0)
	{
		expressionListItem->itemType=ELI_INTEGER;
		expressionListItem->itemValue=0;
		expressionListItem->valueResolved=false;
		if(parameterExpressionListItem->itemType!=ELI_UNRESOLVED)
		{
			if(parameterExpressionListItem->itemType==ELI_INTEGER)
			{
				expressionListItem->itemValue=(parameterExpressionListItem->itemValue)&0xFFFF;
				expressionListItem->valueResolved=true;
			}
			else
			{
				AssemblyComplaint(NULL,true,"%s: only works on integers\n",functionName);
				fail=true;
			}
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Unrecognized function name: %s\n",functionName);
		fail=true;
	}
	return(!fail);
}

// prototype recursive function
static bool ParentheticExpression(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element,bool *haveElement,EXPRESSION_LIST_ITEM *expressionListItem,unsigned int precedence);

static bool Factor(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element,bool *haveElement,EXPRESSION_LIST_ITEM *expressionListItem)
// Handle factors in expressions
{
	EXPRESSION_ELEMENT
		tmpElement;
	EXPRESSION_LIST_ITEM
		tmpExpressionListItem;
	bool
		fail;
	LABEL_RECORD
		*oldLabel;

	fail=false;
	if(*haveElement)
	{
		if(element->type==ET_OPERATOR)	// handle unary operators
		{
			switch(element->value)
			{
				case OP_SUBTRACT:
					*haveElement=ParseExpressionElement(line,lineIndex,element);
					if(Factor(line,lineIndex,element,haveElement,expressionListItem))	// get the next factor
					{
						if(expressionListItem->valueResolved)
						{
							expressionListItem->itemValue=-expressionListItem->itemValue;	// negate it
						}
					}
					else
					{
						fail=true;
					}
					break;
				case OP_LOGICAL_NOT:
					*haveElement=ParseExpressionElement(line,lineIndex,element);
					if(Factor(line,lineIndex,element,haveElement,expressionListItem))	// get the next factor
					{
						if(expressionListItem->valueResolved)
						{
							expressionListItem->itemValue=!expressionListItem->itemValue;	// get logical not
						}
					}
					else
					{
						fail=true;
					}
					break;
				case OP_NOT:
					*haveElement=ParseExpressionElement(line,lineIndex,element);
					if(Factor(line,lineIndex,element,haveElement,expressionListItem))	// get the next factor
					{
						if(expressionListItem->valueResolved)
						{
							expressionListItem->itemValue=~expressionListItem->itemValue;	// get not
						}
					}
					else
					{
						fail=true;
					}
					break;
				case OP_LEFT_PAREN:
					*haveElement=ParseExpressionElement(line,lineIndex,element);
					if(ParentheticExpression(line,lineIndex,element,haveElement,expressionListItem,0))	// get expression within the parens
					{
						if(*haveElement&&(element->type==ET_OPERATOR)&&element->value==OP_RIGHT_PAREN)
						{
							*haveElement=ParseExpressionElementOperator(line,lineIndex,element);
						}
						else
						{
							AssemblyComplaint(NULL,true,"Missing ')' in expression\n");
							fail=true;
						}
					}
					else
					{
						fail=true;
					}
					break;
				default:
					AssemblyComplaint(NULL,true,"Unexpected operator in expression\n");
					fail=true;
					break;
			}
		}
		else
		{
			if(element->type==ET_FUNCTION)	// handle functions
			{
				*haveElement=ParseExpressionElement(line,lineIndex,&tmpElement);
				if(ParentheticExpression(line,lineIndex,&tmpElement,haveElement,&tmpExpressionListItem,0))	// get expression within the parens
				{
					if(*haveElement&&(tmpElement.type==ET_OPERATOR)&&tmpElement.value==OP_RIGHT_PAREN)
					{
						fail=!ProcessFunction(element->string,expressionListItem,&tmpExpressionListItem);
					}
					else
					{
						AssemblyComplaint(NULL,true,"Missing ')'\n");
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
			}
			else if(element->type==ET_NUM)	// handle numbers
			{
				expressionListItem->itemType=ELI_INTEGER;
				expressionListItem->itemValue=element->value;
				expressionListItem->valueResolved=true;
			}
			else if(element->type==ET_STRING)	// handle strings
			{
				expressionListItem->itemType=ELI_STRING;
				strcpy(expressionListItem->itemString,element->string);
				expressionListItem->valueResolved=true;
			}
			else										// must be a label, or local label
			{
				if((oldLabel=LocateLabel(&element->string[0],!intermediatePass))&&oldLabel->resolved)
				{
					// @@@ handle "string" labels
					expressionListItem->itemType=ELI_INTEGER;
					expressionListItem->itemValue=oldLabel->value;
					expressionListItem->valueResolved=true;
				}
				else
				{
					expressionListItem->itemType=ELI_UNRESOLVED;
					expressionListItem->itemValue=0;
					expressionListItem->valueResolved=false;
					numUnresolvedLabels++;
					AssemblyComplaint(NULL,true,"Failed to resolve '%s'\n",element->string);
					ReportDiagnostic("Unresolved label: '%s'\n",element->string);
				}
			}
			*haveElement=ParseExpressionElementOperator(line,lineIndex,element);
		}
	}
	else
	{
		AssemblyComplaint(NULL,true,"Missing factor in expression\n");
		fail=true;
	}
	return(!fail);
}

static bool ProcessBinaryOperator(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element,bool *haveElement,EXPRESSION_LIST_ITEM *expressionListItem)
// A binary operator has been encountered during expression parsing
// Get the next term for the operator, operate on it, and return the result
{
	EXPRESSION_LIST_ITEM
		tmpExpressionListItem;
	int
		token;
	bool
		fail;

	fail=false;

	token=element->value;	// remember this as we may write over it

	*haveElement=ParseExpressionElement(line,lineIndex,element);
	if(ParentheticExpression(line,lineIndex,element,haveElement,&tmpExpressionListItem,opDescriptions[token].precedence))
	{
		if((tmpExpressionListItem.itemType!=ELI_UNRESOLVED)&&(expressionListItem->itemType!=ELI_UNRESOLVED))	// make sure both types are known
		{
			if((tmpExpressionListItem.itemType==ELI_INTEGER)&&(expressionListItem->itemType==ELI_INTEGER))		// for the moment, they both must be integers, later, we'll allow some math on strings
			{
				if(tmpExpressionListItem.valueResolved&&expressionListItem->valueResolved)	// make sure both types are resolved
				{
					switch(token)
					{
						case OP_MULTIPLY:
							expressionListItem->itemValue*=tmpExpressionListItem.itemValue;
							break;
						case OP_DIVIDE:
							if(tmpExpressionListItem.itemValue!=0)
							{
								expressionListItem->itemValue/=tmpExpressionListItem.itemValue;
							}
							else
							{
								AssemblyComplaint(NULL,true,"Attempt to divide by 0\n");
								fail=true;
							}
							break;
						case OP_MOD:
							if(tmpExpressionListItem.itemValue!=0)
							{
								expressionListItem->itemValue%=tmpExpressionListItem.itemValue;
							}
							else
							{
								AssemblyComplaint(NULL,true,"Attempt to mod by 0\n");
								fail=true;
							}
							break;

						case OP_ADD:
							expressionListItem->itemValue+=tmpExpressionListItem.itemValue;
							break;
						case OP_SUBTRACT:
							expressionListItem->itemValue-=tmpExpressionListItem.itemValue;
							break;

						case OP_SHIFT_LEFT:
							expressionListItem->itemValue<<=tmpExpressionListItem.itemValue;
							if(tmpExpressionListItem.itemValue<0)
							{
								AssemblyComplaint(NULL,false,"Shift by negative value\n");
							}
							break;
						case OP_SHIFT_RIGHT:
							expressionListItem->itemValue>>=tmpExpressionListItem.itemValue;
							if(tmpExpressionListItem.itemValue<0)
							{
								AssemblyComplaint(NULL,false,"Shift by negative value\n");
							}
							break;

						case OP_GREATER_OR_EQUAL:
							expressionListItem->itemValue=((expressionListItem->itemValue)>=tmpExpressionListItem.itemValue);
							break;
						case OP_LESS_OR_EQUAL:
							expressionListItem->itemValue=((expressionListItem->itemValue)<=tmpExpressionListItem.itemValue);
							break;
						case OP_GREATER_THAN:
							expressionListItem->itemValue=((expressionListItem->itemValue)>tmpExpressionListItem.itemValue);
							break;
						case OP_LESS_THAN:
							expressionListItem->itemValue=((expressionListItem->itemValue)<tmpExpressionListItem.itemValue);
							break;

						case OP_EQUAL:
							expressionListItem->itemValue=((expressionListItem->itemValue)==tmpExpressionListItem.itemValue);
							break;
						case OP_NOT_EQUAL:
							expressionListItem->itemValue=((expressionListItem->itemValue)!=tmpExpressionListItem.itemValue);
							break;

						case OP_LOGICAL_AND:
							expressionListItem->itemValue=((expressionListItem->itemValue)&&tmpExpressionListItem.itemValue);
							break;

						case OP_LOGICAL_OR:
							expressionListItem->itemValue=((expressionListItem->itemValue)||tmpExpressionListItem.itemValue);
							break;

						case OP_AND:
							expressionListItem->itemValue&=tmpExpressionListItem.itemValue;
							break;

						case OP_XOR:
							expressionListItem->itemValue^=tmpExpressionListItem.itemValue;
							break;

						case OP_OR:
							expressionListItem->itemValue|=tmpExpressionListItem.itemValue;
							break;
					}
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Cannot do math on strings\n");
				fail=true;
			}
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool ParentheticExpression(const char *line,unsigned int *lineIndex,EXPRESSION_ELEMENT *element,bool *haveElement,EXPRESSION_LIST_ITEM *expressionListItem,unsigned int precedence)
// Handle evaluating an expression until a right parenthesis, or the end of the expression
{
	bool
		fail,
		done;

	fail=false;
	if(Factor(line,lineIndex,element,haveElement,expressionListItem))
	{
		done=false;
		while(!done&&!fail&&(*haveElement))
		{
			if(element->type==ET_OPERATOR)
			{
				if(element->value==OP_RIGHT_PAREN)
				{
					done=true;
				}
				else
				{
					if(opDescriptions[element->value].binaryOperator)
					{
						if(opDescriptions[element->value].precedence>precedence)
						{
							fail=!ProcessBinaryOperator(line,lineIndex,element,haveElement,expressionListItem);
						}
						else
						{
							done=true;	// operator is of lower precedence, so save it until next time
						}
					}
					else
					{
						AssemblyComplaint(NULL,true,"Unexpected operator in expression\n");
						fail=true;
					}
				}
			}
			else
			{
				AssemblyComplaint(NULL,true,"Expected operator, none found\n");
				fail=true;
			}
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

bool ParseTypedExpression(const char *line,unsigned int *lineIndex,EXPRESSION_LIST_ITEM *expressionListItem)
// Parse an expression from line, return the resulting expressionList
// item.
// Leave lineIndex pushed past the last thing successfully parsed.
// NOTE: this is responsible for updating numUnresolvedLabels every time
// it encounters a label it cannot resolve
// NOTE: this prints its own error messages
// NOTE: because this is fairly high level, it should only be called
// when you _expect_ the next thing on the line to be an expression.
// In other words, this function should not be called to test to see
// if the next thing is an expression, since it may output spurious
// error/warning messages.
{
	bool
		haveElement;
	EXPRESSION_ELEMENT
		element;
	bool
		fail;

	fail=false;
	expressionListItem->itemType=ELI_UNRESOLVED;		// don't know what this is yet
	expressionListItem->valueResolved=false;			// not resolved either

	if((haveElement=ParseExpressionElement(line,lineIndex,&element)))	// get the first element of the expression (if there is one)
	{
		if(ParentheticExpression(line,lineIndex,&element,&haveElement,expressionListItem,0))
		{
			if(haveElement)
			{
				switch(element.type)
				{
					case ET_NUM:
						AssemblyComplaint(NULL,true,"Unexpected number in expression\n");
						break;
					case ET_OPERATOR:
						AssemblyComplaint(NULL,true,"Unexpected operator in expression\n");
						break;
					case ET_FUNCTION:
						AssemblyComplaint(NULL,true,"Unexpected function in expression\n");
						break;
					case ET_LABEL:
					case ET_LOCAL_LABEL:
						AssemblyComplaint(NULL,true,"Unexpected label in expression\n");
						break;
					case ET_STRING:
						AssemblyComplaint(NULL,true,"Unexpected string in expression\n");
						break;
				}
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
	return(!fail);
}

bool ParseExpression(const char *line,unsigned int *lineIndex,int *value,bool *unresolved)
// Parse a numeric expression from line, return the result in value
// NOTE: if unresolved is returned true, the expression could not be evaluated
// because 1 or more labels it contained were not defined. In this case,
// value will be returned as 0.
// Leave lineIndex pushed past the last thing successfully parsed.
// NOTE: this is responsible for updating numUnresolvedLabels every time
// it encounters a label it cannot resolve
// NOTE: this prints its own error messages
// NOTE: because this is fairly high level, it should only be called
// when you _expect_ the next thing on the line to be an expression.
// In other words, this function should not be called to test to see
// if the next thing is an expression, since it may output spurious
// error/warning messages.
{
	bool
		fail;
	EXPRESSION_LIST_ITEM
		expressionListItem;

	fail=false;
	if(ParseTypedExpression(line,lineIndex,&expressionListItem))
	{
		switch(expressionListItem.itemType)		// see what we got back
		{
			case ELI_UNRESOLVED:
				*unresolved=true;					// if unknown, hold out hope that it will be integer
				*value=0;
				break;
			case ELI_INTEGER:
				if(expressionListItem.valueResolved)
				{
					*unresolved=false;
					*value=expressionListItem.itemValue;
				}
				else
				{
					*unresolved=true;
					*value=0;
				}
				break;
			case ELI_STRING:
				AssemblyComplaint(NULL,true,"Expression must be numeric\n");
				fail=true;
				break;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}
