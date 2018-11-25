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


#define		VERSION					"1.11"

#define		DEFAULT_MAX_PASSES		32			// default maximum number of passes made by the assembler

#define		MAX_INCLUDE_DEPTH		256
#define		MAX_BLOCK_DEPTH			256			// number of levels of text substitution allowed
#define		MAX_STRING				4096		// maximum string length (including 0 termination)
#define		MAX_FILE_PATH			4096		// maximum length of a path (including 0 termination)

#define		Max(a,b) 	((a)>(b)?(a):(b))
#define		Min(a,b) 	((a)<(b)?(a):(b))

#define	elementsof(array) (sizeof(array)/sizeof(array[0]))

struct SYM_TABLE_NODE;							// opaque symbol table node
struct SYM_TABLE;								// opaque symbol table

struct WHERE_FROM
{
	SYM_TABLE_NODE
		*file;									// points at a node in the global file name symbol table which is the name of the file that something came from (NULL if none defined)
	unsigned int
		fileLineNumber;
};

struct TEXT_LINE
{
	TEXT_LINE
		*next;									// points to the next line of the text (NULL if this is the last line)
	WHERE_FROM
		whereFrom;								// tells where this line came from in the source data
	char
		line[1];								// stored line data, 0 terminated
};

struct TEXT_BLOCK
{
	TEXT_LINE
		*firstLine,								// pointers to lines of text (NULL if no lines)
		*lastLine;
};

// label flags (used in structure below)

enum
{
	LF_EXT_CONST,								// label was defined externally to source code (e.g. on the command line)
	LF_CONST,									// label is a constant, and cannot be redefined
	LF_SET,										// label is a set constant, and can be redefined only by another set
	LF_LABEL,									// label is a label for code (and cannot be redefined, or equated, or set)
};

struct PARSED_LABEL
{
	char
		name[MAX_STRING];
	bool
		isLocal;
};

struct LABEL_RECORD
{
	int
		value;									// current value for label
	unsigned int
		type;									// flags what type of label this is
	unsigned int
		passCount;								// passcount when last defined
	bool
		resolved;								// tells if this label has been resolved or not
	unsigned int
		refCount;								// tells how many places in the source this label was referenced
	LABEL_RECORD
		*previous,
		*next;
	WHERE_FROM
		whereFrom;								// tells where this label was defined (first)
	SYM_TABLE_NODE
		*symbol;								// label name is stored here
};

struct LISTING_RECORD							// used to create and control the listing
{
	unsigned int
		lineNumber;
	unsigned int
		listPC;
	char
		sourceType;
	bool
		wantList;
	char
		listObjectString[MAX_STRING];			// object code string printed out
};

enum
{
	CT_ROOT,									// bottom entry on the context stack
	CT_IF,
	CT_SWITCH,
	CT_MACRO,
	CT_REPEAT,
};

typedef void CONTEXT_FLUSH(struct CONTEXT_RECORD *record);

struct CONTEXT_RECORD
{
	unsigned int
		contextType;							// type of context this is
	bool
		active;									// true if making code, false if just holding context for parsing
	CONTEXT_RECORD
		*next;									// points to the next record on the stack, or NULL if no more
	WHERE_FROM
		whereFrom;								// tells where the context was started
	CONTEXT_FLUSH
		*flush;									// routine used to flush this record before it is deleted (NULL if no routine)
	unsigned char
		contextData[1];							// variable length array of context data
};

struct IF_CONTEXT
{
	bool
		sawElse,								// true if the else has been seen 
		wantElse;								// true if the else will make the assembly proceed 
};

struct SWITCH_CONTEXT
{
	bool
		wantCase;								// true if a case statement can make us active
	bool
		unresolved;								// true if expression at switch statement was unresolved
	int
		value;									// value of expression at switch statement
};

struct REPEAT_CONTEXT
{
	unsigned int
		numTimes;								// tells how many more times to repeat this
	TEXT_BLOCK
		textBlock;								// block of text containing the data to be repeated
};

struct MACRO_RECORD
{
	TEXT_BLOCK
		parameters;								// block of text containing the macro parameters
	TEXT_BLOCK
		contents;								// block of text containing the macro contents
	MACRO_RECORD
		*previous,
		*next;									// next macro record in the global list
	SYM_TABLE_NODE
		*symbol;								// macro name is stored here
	WHERE_FROM
		whereFrom;								// tells where the macro was defined
};

struct ALIAS_RECORD
{
	ALIAS_RECORD
		*previous,								// globally linked list of aliases
		*next;
	SYM_TABLE_NODE
		*symbol;								// alias name stored here
	WHERE_FROM
		whereFrom;								// tells where the alias was defined
	unsigned int
		contentLength;							// number of bytes of content
	char
		contents[1];							// variable length array of contents data
};

struct CODE_PAGE
{
	unsigned int
		address;								// page address (low 8 bits are always cleared)
	unsigned char
		usageMap[32];							// bit array which tells the bytes in use by this page (0th bit of 0th byte indicates the 0th byte of the page is occupied)
	unsigned char
		pageData[256];							// data for the page
	CODE_PAGE
		*previous,								// linkage to the next page in the global list
		*next;
};

struct SEGMENT_RECORD
{
	bool
		generateOutput;							// true if this segment should generate output, false otherwise
	CODE_PAGE
		*pageCache,								// points to the last used page to make things more efficient
		*firstPage;								// points to the first page of the segment (pages are linked in order here)
	unsigned int
		currentPC;								// keeps track of the running byte location within the segment
	unsigned int
		codeGenOffset;							// code is generated to run at currentPC+codeGenOffset (this allows code to be assembled to run in a different location that it is being written to)
	bool
		sawDS;									// true if define space command (non-allocating PC movement) was issued for this segment
	unsigned int
		minDS,									// keeps track of the lowest PC we ever saw a define space for
		maxDS;									// keeps track of the highest PC we ever saw a define space for
	SEGMENT_RECORD
		*previous,								// linkage to the next segment in the global list
		*next;
	SYM_TABLE_NODE
		*symbol;								// segment name is stored here
};

