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

// File handling

#include	"include.h"

#define		PATH_SEP	'/'		// path separation character (change this for your OS if needed)

struct PATH_HEADER
{
	PATH_HEADER
		*nextPath;
	char
		pathName[1];			// variable length path name
};

static PATH_HEADER
	*topPath,
	*bottomPath;

void CloseTextOutputFile(FILE *file)
// close the text output file
{
	fclose(file);
}

FILE *OpenTextOutputFile(const char *name)
// Open a file for writing text output into
// If there is a problem, complain and return NULL
{
	FILE
		*file;

	if(!(file=fopen(name,"w")))
	{
		ReportComplaint(true,"Could not open file '%s'\nOS Reports: %s\n",name,strerror(errno));
	}
	return(file);
}

void CloseBinaryOutputFile(FILE *file)
// Close the binary output file
{
	fclose(file);
}

FILE *OpenBinaryOutputFile(const char *name)
// Open a file for writing binary output into
// If there is a problem, complain and return NULL
{
	FILE
		*file;

	if(!(file=fopen(name,"wb")))
	{
		ReportComplaint(true,"Could not open file '%s'\nOS Reports: %s\n",name,strerror(errno));
	}
	return(file);
}


static SYM_TABLE_NODE *CreateFileNameSymbol(const char *name)
// Create (or locate) an entry in the global file name symbol table for name
// NOTE: this table is used to keep track of all files accessed during assembly (across
// all passes).
// If there is a problem, return NULL
{
	SYM_TABLE_NODE
		*node;

	node=STFindNode(fileNameSymbols,name);
	if(!node)	// if node could not be located, try to create one
	{
		node=STAddEntryAtEnd(fileNameSymbols,name,NULL);
	}
	return(node);
}

void CloseSourceFile(FILE *file)
// Close the source file (leave symbol table entry around because
// things created when parsing the file are still referencing it)
{
	fclose(file);
}

FILE *OpenSourceFile(const char *name,bool huntForIt,SYM_TABLE_NODE **fileNameSymbol)
// Open a source/include file
// If there is a problem, complain and return NULL
// if huntForIt is true, then look through the include paths trying to locate
// it.
// If a file is successfully opened, a pointer to it is returned, along with
// a pointer to a symbol table entry for its name
{
	FILE
		*file;
	char
		newPath[MAX_FILE_PATH];
	unsigned int
		nameLength;
	PATH_HEADER
		*path;

	strcpy(newPath,name);
	if(!(file=fopen(newPath,"rb")))				// try to open the passed file
	{
		if(huntForIt)							// if we failed, see if we need to hunt for it
		{
			nameLength=strlen(name);
			path=topPath;
			while(path&&!file)
			{
				if(nameLength+strlen(path->pathName)<sizeof(newPath))		// make sure what we are about to do will not overflow
				{
					sprintf(newPath,"%s%s",path->pathName,name);
					file=fopen(newPath,"rb");
				}
				path=path->nextPath;
			}
		}
	}

	if(file)
	{
		if(((*fileNameSymbol)=CreateFileNameSymbol(newPath)))
		{
			return(file);
		}
		else
		{
			AssemblyComplaint(NULL,true,"Failed to create file name symbol table entry\n");
		}
		fclose(file);
	}
	else
	{
		AssemblyComplaint(NULL,true,"Could not open source file '%s': %s\n",name,strerror(errno));
	}
	return(NULL);
}

bool AddIncludePath(const char *pathName)
// Add path to the list of paths to be searched for include files
// If there is a problem, return false
{
	unsigned int
		length;
	PATH_HEADER
		*path;
	bool
		addPathSep;

	length=strlen(pathName);
	addPathSep=false;
	if(length&&pathName[length-1]!=PATH_SEP)	// see if we need to add a path separator to the end of the path
	{
		addPathSep=true;
		length++;								// make room for it
	}
	if((path=(PATH_HEADER *)NewPtr(sizeof(PATH_HEADER)+length)))
	{
		strcpy(&path->pathName[0],pathName);	// copy over the path name
		if(addPathSep)							// see if need to add seperator
		{
			path->pathName[length-1]=PATH_SEP;
			path->pathName[length]='\0';
		}
		// link this to the list at the end (so they are searched in the order given)
		path->nextPath=NULL;
		if(bottomPath)
		{
			bottomPath->nextPath=path;
			bottomPath=path;
		}
		else
		{
			topPath=bottomPath=path;
		}
	}
	return(path!=NULL);
}

void UnInitFiles()
// undo what InitFiles did
{
	PATH_HEADER
		*nextPath;

	while(topPath)
	{
		nextPath=topPath->nextPath;
		DisposePtr(topPath);
		topPath=nextPath;
	}
	STDisposeSymbolTable(fileNameSymbols);		// get rid of the file names symbol table
}

bool InitFiles()
// Initialize file handling
{
	if((fileNameSymbols=STNewSymbolTable(100)))
	{
		topPath=bottomPath=NULL;
		return(true);
	}
	return(false);
}
