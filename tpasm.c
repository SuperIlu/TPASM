//	Copyright (C) 1999-2018 Core Technologies.
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


// Cross Assembler

#include	"include.h"

struct TOKEN_LIST
{
	const char
		*token;
	unsigned int
		value;
};

enum
{
	T_HELP,
	T_OUTPUT,
	T_DIRECTORY,
	T_DEFINE,
	T_PROCESSOR,
	T_PASSES,
	T_LIST_NAME,
	T_STRICT_PSEUDO,
	T_WARNINGS,
	T_DEBUG,
	T_SHOW_PROCESSORS,
	T_SHOW_OUTPUT_TYPES,
};

static const TOKEN_LIST
	cmdTokens[]=
	{
		{"-h",T_HELP},
		{"--help",T_HELP},
		{"-o",T_OUTPUT},
		{"-I",T_DIRECTORY},
		{"-d",T_DEFINE},
		{"-P",T_PROCESSOR},
		{"-n",T_PASSES},
		{"-l",T_LIST_NAME},
		{"-s",T_STRICT_PSEUDO},
		{"-w",T_WARNINGS},
		{"-p",T_DEBUG},
		{"-show_procs",T_SHOW_PROCESSORS},
		{"-show_types",T_SHOW_OUTPUT_TYPES},
		{"",0}
	};

static bool MatchToken(char *string,const TOKEN_LIST *list,unsigned int *token)
// given a string, see if it matches any of the tokens in the token list, if so, return
// its token number in token, otherwise, return false
{
	unsigned int
		i;
	bool
		found,
		reachedEnd;

	found=reachedEnd=false;
	i=0;
	while(!found&&!reachedEnd)
	{
		if(list[i].token[0]!='\0')
		{
			if(strcmp(string,list[i].token)==0)
			{
				found=true;
				*token=list[i].value;
			}
			i++;
		}
		else
		{
			reachedEnd=true;
		}
	}
	return(found);
}

bool ProcessLineLocationLabel(const PARSED_LABEL *parsedLabel)
// When the label on a line is meant to be interpreted as labelling the current
// location, call this to handle it. NOTE: parsedLabel is passed in as NULL, it
// indicates there was no label on the line.
{
	bool
		fail;

	fail=false;
	if(parsedLabel)
	{
		if(currentSegment)
		{
			fail=!AssignLabel(parsedLabel,currentSegment->currentPC+currentSegment->codeGenOffset);	// assign this label now, since no assembler pseudo-op was located which may have changed its meaning
		}
		else
		{
			AssemblyComplaint(NULL,true,"Label '%s' defined outside of segment\n",parsedLabel->name);
		}
	}
	return(!fail);
}

static bool ParseLine(char *line,LISTING_RECORD *listingRecord)
// Handle parsing and assembling.
// Return true if there was no "hard" failure.
{
	bool
		result;
	bool
		hadMatch;
	PARSED_LABEL
		parsedLabel,
		*lineLabel;
	unsigned int
		lineIndex;
	char
		string[MAX_STRING];

	result=true;		// assume no hard failure
	lineIndex=0;
	lineLabel=NULL;
	hadMatch=false;		// nothing has matched this line yet
	if(SkipWhiteSpace(line,&lineIndex)||(lineLabel=(ParseLabelDefinition(line,&lineIndex,&parsedLabel)?&parsedLabel:NULL))||ParseComment(line,&lineIndex))	// does the line begin correctly?
	{
		if((result=HandleAliasMatches(line,&lineIndex,listingRecord)))	// substitute opcode/operand aliases
		{
			if((result=AttemptGlobalPseudoOpcode(line,&lineIndex,lineLabel,listingRecord,&hadMatch)))
			{
				if(!hadMatch)
				{
					if(contextStack->active)							// if not active, only a global pseudo-op can solve that, so ignore these lines
					{
						if((result=AttemptProcessorPseudoOpcode(line,&lineIndex,lineLabel,listingRecord,&hadMatch)))
						{
							if(!hadMatch)
							{
								if((result=ProcessLineLocationLabel(lineLabel)))		// label must be for the current location, so process it
								{
									if((result=AttemptProcessorOpcode(line,&lineIndex,listingRecord,&hadMatch)))
									{
										if(!hadMatch)
										{
											if((result=AttemptMacro(line,&lineIndex,listingRecord,&hadMatch)))
											{
												if(!hadMatch)
												{
													if(ParseNonWhiteSpace(line,&lineIndex,string))	// something still remains?
													{
														AssemblyComplaint(NULL,true,"Unrecognized opcode '%s'\n",string);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if(contextStack->active)
		{
			AssemblyComplaint(NULL,true,"Garbage at start of line\n");
		}
	}
	return(result);
}

static bool AssembleLine(char *line,char sourceType,bool wantList)
// Assemble the current source line
// If there is a problem, (hard error), complain and return false
// If there is a syntax error, or some sort of assembly problem
// Do not complain about it until the last pass (assembly errors
// are not considered failure for this routine).
{
	bool
		fail;
	LISTING_RECORD
		listingRecord;
	TEXT_BLOCK
		*wasCollecting;

	fail=false;

	listingRecord.lineNumber=currentVirtualFileLine;
	listingRecord.listPC=0;
	if(currentSegment)
	{
		listingRecord.listPC=currentSegment->currentPC;
	}
	listingRecord.listObjectString[0]='\0';
	listingRecord.wantList=wantList;
	listingRecord.sourceType=sourceType;

	// ### this is a little ugly.
	// We want to be able to collect the text BETWEEN the start and end pseudo-ops
	// this solves the problem 
	wasCollecting=collectingBlock;

	if(ParseLine(line,&listingRecord))
	{
		// generate list output
		OutputListFileLine(&listingRecord,line);			// dump the generated bytes to the list file

		if(wasCollecting&&collectingBlock)					// if nothing done to change block collection status, then collect the text line
		{
			if(!AddLineToTextBlock(collectingBlock,line))
			{
				ReportComplaint(true,"Failed to create text line during text block collection\n");
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

static bool GetLine(FILE *file,char *line,unsigned int lineLength,bool *atEOF,bool *overflow)
// read a line from the given file into the array line
// if there is a read error, return false
// lineLength is considered to be the maximum number of characters
// to read into the line (including the 0 terminator)
// if the line fills before a CR is hit, overflow will be set true, and the rest of the
// line will be read, but not stored
// if the EOF is hit, atEOF is set true
{
	unsigned int
		i;
	int
		result;
	unsigned char
		c;
	bool
		hadError;
	bool
		stopReading;

	i=0;												// index into output line
	stopReading=hadError=(*atEOF)=(*overflow)=false;	
	while(!stopReading)
	{
		result=fgetc(file);
		if(result!=EOF)									// if something was read, check it, and add to output line
		{
			c=(unsigned char)result;
			if(c=='\n'||c=='\0')						// found termination?
			{
				stopReading=true;						// yes, output line is complete
			}
			else
			{
				if(c!='\r')								// see if the character should be added to the line
				{
					if(i<lineLength-1)					// make sure there is room to store it
					{
						line[i++]=c;					// store it if there is room
					}
					else
					{
						(*overflow)=true;				// complain of overflow, but continue to the end
					}
				}
			}
		}
		else
		{
			stopReading=true;
			(*atEOF)=true;								// tell caller that EOF was encountered
		}
	}
	if(lineLength)
	{
		line[i]='\0';									// terminate the line if possible
	}
	return(!hadError);									// return error status
}

bool ProcessTextBlock(TEXT_BLOCK *block,TEXT_BLOCK *substitutionList,TEXT_BLOCK *substitutionText,char sourceType)
// process the passed block of text into the assembly stream
// NOTE: if substitutionList or substitutionText is NULL, no substitutions will occur
{
	char
		*inBuffer;										// character buffers for line parsing
	TEXT_LINE
		*tempLine;
	unsigned int
		lastScopeValue;
	bool
		fail;
	bool
		overflow;

	fail=false;

	if(blockDepth<MAX_BLOCK_DEPTH)
	{
		blockDepth++;									// step deeper

		lastScopeValue=scopeValue;						// remember the last value so we can put it back (allows labels to be local within text substitution blocks)
		scopeCount++;									// forward the scope count so this block has its own local label scope
		scopeValue=scopeCount;

		if((inBuffer=(char *)NewPtr((int)(MAX_STRING))))
		{
			tempLine=block->firstLine;
			stopParsing=false;
			while(!fail&&!stopParsing&&tempLine)
			{
				currentVirtualFile=tempLine->whereFrom.file;
				currentVirtualFileLine=tempLine->whereFrom.fileLineNumber;
				if(substitutionList&&substitutionText)	// make sure substitutions are desired
				{
					if(ProcessTextSubsitutions(inBuffer,&tempLine->line[0],substitutionList,substitutionText,&overflow))
					{
						if(overflow)
						{
							AssemblyComplaint(NULL,false,"Line too long, truncation occurred\n");
						}
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					strcpy(inBuffer,&tempLine->line[0]);
				}
				if(!fail)
				{
					fail=!AssembleLine(inBuffer,sourceType,outputListingExpansions);
					tempLine=tempLine->next;			// do next line
				}
			}
			DisposePtr(inBuffer);
		}
		else
		{
			ReportComplaint(true,"Could not allocate memory for line input buffer\nOS Reports: %s\n",strerror(errno));
			fail=true;
		}

		scopeValue=lastScopeValue;				// back out to the previous scope

		stopParsing=false;

		blockDepth--;							// step back
	}
	else
	{
		AssemblyComplaint(NULL,true,"Attempt to nest text substitutions too deeply (arbitrary limit is %d)\n",MAX_BLOCK_DEPTH);
	}

	return(!fail);
}

bool ProcessSourceFile(const char *fileName,bool huntForIt)
// process the passed source file, if there are any problems, return false
// if intermediatePass is true, do not flag expressions which contain unresolved labels as errors,
// and do not generate output.
// if intermediatePass is false, then generate output, complaining about anything
// that could not be resolved.
// numUnresolvedLabels will be incremented for each unresolved label encountered.
// numModifiedLabels will be incremented each time a label's value is changed.
{
	char
		*inBuffer;									// character buffers for line parsing
	FILE
		*sourceFile;
	unsigned int
		oldLineNum;									// line number at entry
	SYM_TABLE_NODE
		*newSourceFile,
		*oldSourceFile;
	bool
		fail;
	bool
		overflow;

	fail=false;

	if(includeDepth<MAX_INCLUDE_DEPTH)
	{
		includeDepth++;								// step deeper

		oldSourceFile=currentFile;
		oldLineNum=currentFileLine;					// hold these until we are through

		if((sourceFile=OpenSourceFile(&fileName[0],huntForIt,&newSourceFile)))
		{
			if((inBuffer=(char *)NewPtr((int)(MAX_STRING))))
			{
				currentFile=newSourceFile;
				currentFileLine=0;
				stopParsing=false;
				while(!fail&&!stopParsing)
				{
					if(GetLine(sourceFile,inBuffer,MAX_STRING,&stopParsing,&overflow))
					{
						currentFileLine++;				// increment the line because we just read one
						currentVirtualFile=currentFile;
						currentVirtualFileLine=currentFileLine;
						if(overflow)
						{
							AssemblyComplaint(NULL,false,"Line too long, truncation occurred\n");
						}
						fail=!AssembleLine(inBuffer,' ',true);
					}
					else
					{
						ReportComplaint(true,"Failed to read source line\n");
						fail=true;
					}
				}
				DisposePtr(inBuffer);
			}
			else
			{
				ReportComplaint(true,"Could not allocate memory for line input buffer\nOS Reports: %s\n",strerror(errno));
				fail=true;
			}
			CloseSourceFile(sourceFile);
		}

		currentFile=oldSourceFile;
		currentFileLine=oldLineNum;
		stopParsing=false;

		includeDepth--;							// step back
	}
	else
	{
		ReportComplaint(true,"Attempt to nest include too deeply (arbitrary limit is %d)\n",MAX_INCLUDE_DEPTH);
		fail=true;
	}

	return(!fail);
}

static bool ProcessAssembly()
// Assemble the source file, and all includes it may have.
// if intermediatePass is true, do not flag expressions which contain unresolved labels as errors,
// and do not generate output.
// if intermediatePass is false, then generate output, complaining about anything
// that could not be resolved.
// numUnresolvedLabels will be incremented for each unresolved label encountered.
// numModifiedLabels will be incremented each time a label's value is changed.
{
	bool
		fail;
	bool
		found;

	ReportDiagnostic("pass %d\n",passCount+1);

	currentFile=NULL;								// pointer to the current file being assembled
	currentFileLine=0;
	currentVirtualFile=NULL;						// clear pointer to virtual file
	currentVirtualFileLine=0;

	collectingBlock=NULL;							// not collecting a text block yet

	aliasesHead=NULL;								// no operand aliases exist yet
	macrosHead=NULL;								// no macros exist yet

	currentSegment=NULL;							// no current segment yet
	segmentsHead=segmentsTail=NULL;					// no segments exist yet

	includeDepth=0;									// no includes yet

	if(SelectProcessor(defaultProcessorName,&found))
	{
		if(found)									// make sure the processor was located
		{
			if((currentSegment=CreateSegment("code",true)))		// set up a default segment to assemble into
			{
				numUnresolvedLabels=numModifiedLabels=0;	// reset these
				scope[0]='\0';								// reset scope
				scopeCount=0;
				scopeValue=0;
				blockDepth=0;								// no macro invocations yet
				contextStack=NULL;							// initialize context stack
				outputListing=true;							// output listing information by default
				outputListingExpansions=true;				// output expansion of macros and repeats by default

				if(PushContextRecord(0))					// create the root context
				{
					contextStack->contextType=CT_ROOT;		// make a default context
					contextStack->active=true;				// declare it active

					fail=!ProcessSourceFile(sourceFileName,false);// go handle this file (and any includes it may have)

					FlushContextRecords();					// unwind any contexts that happen to be left around

					DestroyAliases();						// get rid of alias definitions
					DestroyMacros();						// dispose of macro definitions

					currentFile=NULL;
					currentVirtualFile=NULL;
				}
				else
				{
					ReportComplaint(true,"Failed to create root context\n");
					fail=true;
				}

				if(!intermediatePass)
				{
					OutputListFileSegments();
					OutputListFileLabels();
					if(errorCount==0)
					{
						DumpOutputFiles();					// dump out all requested segment or symbol files
					}
				}

				DestroySegments();							// remove segments created by assembly
			}
			else
			{
				ReportComplaint(true,"Failed to create default code segment\n");
				fail=true;
			}
			SelectProcessor("",&found);						// deselect processor at end of pass
		}
		else
		{
			ReportComplaint(true,"Default processor '%s' unrecognized\n",defaultProcessorName);
			fail=true;
		}
	}
	else
	{
		fail=true;											// some hard failure in select (it was reported there)
	}

	return(!fail);
}

static bool HandleAssembly()
// Assemble the source file(s), create output and list files as needed
// If there is a problem, report it, and return false
{
	bool
		fail;
	unsigned int
		lastNumUnresolvedLabels;
	time_t
		timeIn,timeOut;
	unsigned int
		totalTime;
	bool
		keepGoing;

	fail=false;

	listFile=NULL;
	if(!listFileName||(listFile=OpenTextOutputFile(listFileName)))
	{
		time(&timeIn);							// get time at entry

		OutputListFileHeader(timeIn);

		passCount=0;
		intermediatePass=true;
		fail=!ProcessAssembly();				// make first pass on source
		passCount++;
		lastNumUnresolvedLabels=numUnresolvedLabels;	// remember how many were unresolved this time
		keepGoing=(numUnresolvedLabels!=0);		// if on first pass, nothing was unresolved, then we're done

		while(!fail&&keepGoing)					// loop until we fail, or stop making progress with label resolution
		{
			if(ProcessAssembly())				// process the passed source file (failure here means HARD failure, not assembly errors)
			{
				passCount++;
				ReportDiagnostic("Unresolved labels: %d, Modified labels: %d\n",numUnresolvedLabels,numModifiedLabels);
				keepGoing=(((numUnresolvedLabels>0)&&(numUnresolvedLabels<lastNumUnresolvedLabels))||numModifiedLabels);	// if there are still unresolved labels, but there's progess, OR something got modified, keep going
				lastNumUnresolvedLabels=numUnresolvedLabels;				// remember how many were unresolved this time
				if(passCount>=maxPasses&&keepGoing)	// hmmm, source is not resolving
				{
					ReportComplaint(true,"Assembly aborting due to too many passes (%d)\n",passCount);
					keepGoing=false;
				}
			}
			else
			{
				fail=true;
			}
		}
		if(!fail)									// make final pass, creating output/listing files, complaining about any unresolved labels that still remain
		{
			intermediatePass=false;					// this is the final pass

			fail=!ProcessAssembly();				// make final pass, generating output

			time(&timeOut);							// get time now that assembly is finished
			totalTime=(int)difftime(timeOut,timeIn);

			if(!fail)
			{
				OutputListFileStats(totalTime);
			}
		}
		if(listFileName)							// if list file was desired, then close it
		{
			CloseTextOutputFile(listFile);
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static void InitGlobals()
// initialize program globals
{
	numAllocatedPointers=0;
	infoOnly=false;						// assume we are actually assembling
	strictPseudo=false;
	displayWarnings=true;
	displayDiagnostics=false;
	errorCount=0;
	warningCount=0;

	currentFile=NULL;					// pointer to the current file being assembled
	currentFileLine=0;
	currentVirtualFile=NULL;			// clear pointer to virtual file
	currentVirtualFileLine=0;

	maxPasses=DEFAULT_MAX_PASSES;		// set default maximum number of passes

	sourceFileName=NULL;
	listFileName=NULL;
	defaultProcessorName="";			// by default, select no processor
}

static void Usage()
// print some messages to stderr that tell how to use this program
{
	fprintf(stderr,"\n");
	fprintf(stderr,"%s version %s\n",programName,VERSION);
	fprintf(stderr,"Copyright (C) 1999-2018 Core Technologies.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Cross Assembler\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [opts] sourceFile [opts]\n",programName);
	fprintf(stderr,"Assembly Options:\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   -h                Output this help message\n");
	fprintf(stderr,"   -o type fileName  Output 'type' data to fileName\n");
	fprintf(stderr,"                     Multiple -o options are allowed, all will be processed\n");
	fprintf(stderr,"                     No output is generated by default\n");
	fprintf(stderr,"   -I dir            Append dir to the list of directories searched by include\n");
	fprintf(stderr,"   -d label value    Define a label to the given value\n");
	fprintf(stderr,"   -P processor      Choose initial processor to assemble for\n");
	fprintf(stderr,"   -n passes         Set maximum number of passes (default = %d)\n",DEFAULT_MAX_PASSES);
	fprintf(stderr,"   -l listName       Create listing to listName\n");
	fprintf(stderr,"   -s                Strict pseudo-ops -- limit global pseudo-ops to those that start with a dot\n");
	fprintf(stderr,"   -w                Do not report warnings\n");
	fprintf(stderr,"   -p                Print diagnostic messages to stderr\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Information Options:\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   -show_procs       Dump the supported processor list\n");
	fprintf(stderr,"   -show_types       Dump the output file types list\n");
	fprintf(stderr,"\n");
}

static void NotEnoughArgs(char *commandName)
// complain of inadequate number of arguments for a command
{
	ReportComplaint(true,"Not enough arguments for '%s'\n",commandName);
}

static bool DoOutput(unsigned int *currentArg,unsigned int argc,char *argv[])
// register a type and name of output file
{
	if((*currentArg)+3<=argc)
	{
		(*currentArg)++;
		if(SelectOutputFileType(argv[*currentArg],argv[(*currentArg)+1]))
		{
			(*currentArg)+=2;
			return(true);
		}
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoDirectory(unsigned int *currentArg,unsigned int argc,char *argv[])
// add directory to be searched for include files
{
	if((*currentArg)+2<=argc)
	{
		(*currentArg)++;
		if(AddIncludePath(argv[(*currentArg)++]))	// remember this include path
		{
			return(true);
		}
		else
		{
			ReportComplaint(true,"Failed to add include path\n");
		}
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoDefine(unsigned int *currentArg,unsigned int argc,char *argv[])
// define a label
{
	int
		value;

	if((*currentArg)+3<=argc)
	{
		(*currentArg)++;
		value=strtol(argv[(*currentArg)+1],NULL,0);
		if(CreateLabel(argv[*currentArg],value,LF_EXT_CONST,0,true))
		{
			(*currentArg)+=2;
			return(true);
		}
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoProcessor(unsigned int *currentArg,unsigned int argc,char *argv[])
// set initial processor
{
	if((*currentArg)+2<=argc)
	{
		(*currentArg)++;
		defaultProcessorName=argv[(*currentArg)++];
		return(true);
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoPasses(unsigned int *currentArg,unsigned int argc,char *argv[])
// set initial processor
{
	if((*currentArg)+2<=argc)
	{
		(*currentArg)++;
		maxPasses=strtol(argv[(*currentArg)++],NULL,0);
		return(true);
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoListName(unsigned int *currentArg,unsigned int argc,char *argv[])
// set the name of the listing output file
{
	if((*currentArg)+2<=argc)
	{
		(*currentArg)++;
		listFileName=argv[(*currentArg)++];
		return(true);
	}
	else
	{
		NotEnoughArgs(argv[*currentArg]);
	}
	return(false);
}

static bool DoStrictPseudo(unsigned int *currentArg,unsigned int argc,char *argv[])
// Limit assembler pseudo-ops to those that start with a '.'
// This keeps the non-dotted versions from colliding with opcodes for
// the selected processor.
{
	(*currentArg)++;
	strictPseudo=true;
	return(true);
}

static bool DoWarnings(unsigned int *currentArg,unsigned int argc,char *argv[])
// set no warning display
{
	(*currentArg)++;
	displayWarnings=false;
	return(true);
}

static bool DoDebug(unsigned int *currentArg,unsigned int argc,char *argv[])
// set debug display
{
	(*currentArg)++;
	displayDiagnostics=true;
	return(true);
}

static bool DoShowProcessors(unsigned int *currentArg,unsigned int argc,char *argv[])
// dump the list of supported processors
{
	(*currentArg)++;
	fprintf(stderr,"\n");
	fprintf(stderr,"Supported processors:\n");
	fprintf(stderr,"\n");
	DumpProcessorInformation(stderr);
	fprintf(stderr,"\n");
	infoOnly=true;		// tell parser that user just wanted info -- no assembly
	return(true);
}

static bool DoShowOutputTypes(unsigned int *currentArg,unsigned int argc,char *argv[])
// dump the list of supported output file types
{
	(*currentArg)++;
	fprintf(stderr,"\n");
	fprintf(stderr,"Supported output types:\n");
	fprintf(stderr,"\n");
	DumpOutputFileTypeInformation(stderr);
	fprintf(stderr,"\n");
	infoOnly=true;		// tell parser that user just wanted info -- no assembly
	return(true);
}

static bool ParseCommandLine(unsigned int argc,char *argv[])
// Parse and interpret the command line parameters
// If there is a problem, complain and return false
{
	unsigned int
		currentArg;
	unsigned int
		token;
	bool
		fail;

	fail=false;
	currentArg=1;				// skip over the program name
	while(currentArg<argc&&!fail)
	{
		token=0;				// keep compiler quiet
		if(MatchToken(argv[currentArg],&cmdTokens[0],&token))
		{
			switch(token)
			{
				case T_HELP:
					fail=true;		// fail so that usage information will be printed
					break;
				case T_OUTPUT:
					fail=!DoOutput(&currentArg,argc,argv);
					break;
				case T_DIRECTORY:
					fail=!DoDirectory(&currentArg,argc,argv);
					break;
				case T_DEFINE:
					fail=!DoDefine(&currentArg,argc,argv);
					break;
				case T_PROCESSOR:
					fail=!DoProcessor(&currentArg,argc,argv);
					break;
				case T_PASSES:
					fail=!DoPasses(&currentArg,argc,argv);
					break;
				case T_LIST_NAME:
					fail=!DoListName(&currentArg,argc,argv);
					break;
				case T_STRICT_PSEUDO:
					fail=!DoStrictPseudo(&currentArg,argc,argv);
					break;
				case T_WARNINGS:
					fail=!DoWarnings(&currentArg,argc,argv);
					break;
				case T_DEBUG:
					fail=!DoDebug(&currentArg,argc,argv);
					break;
				case T_SHOW_PROCESSORS:
					DoShowProcessors(&currentArg,argc,argv);
					break;
				case T_SHOW_OUTPUT_TYPES:
					DoShowOutputTypes(&currentArg,argc,argv);
					break;
				default:
					currentArg++;	// this token is processed
					break;
			}
		}
		else
		{
			if(argv[currentArg][0]!='-')
			{
				if(!sourceFileName)
				{
					sourceFileName=argv[currentArg++];			// retain this as the source file name
				}
				else
				{
					ReportComplaint(true,"Source file name already given, '%s' is invalid here\n",argv[currentArg]);
					fail=true;
				}
			}
			else
			{
				ReportComplaint(true,"Unrecognized command line option '%s'\n",argv[currentArg]);
				fail=true;
			}
		}
	}

	if(!fail&&!sourceFileName&&!infoOnly)
	{
		ReportComplaint(true,"No source file name given\n");
		fail=true;
	}
	return(!fail);
}

static void UnInitAssembler()
// Call all the uninitialization routines
{
	UnInitProcessors();
	UnInitAliases();
	UnInitMacros();
	UnInitGlobalPseudoOpcodes();
	UnInitOutputFileGenerate();
	UnInitLabels();
	UnInitSegments();
	UnInitFiles();
}

static bool InitAssembler()
// Call all the initialization routines
{
	InitGlobals();									// start the ball rolling
	if(InitFiles())
	{
		if(InitSegments())							// initialize code segment handling
		{
			if(InitLabels())						// initialize label handling
			{
				if(InitOutputFileGenerate())
				{
					if(InitGlobalPseudoOpcodes())		// create symbols for global pseudo ops
					{
						if(InitMacros())				// initialize macro handling
						{
							if(InitAliases())
							{
								if(InitProcessors())		// set up all the processors
								{
									return(true);
								}
								UnInitAliases();
							}
							UnInitMacros();
						}
						UnInitGlobalPseudoOpcodes();
					}
					UnInitOutputFileGenerate();
				}
				UnInitLabels();
			}
			UnInitSegments();
		}
		UnInitFiles();
	}
	return(false);
}

int main(int argc,char *argv[])
// open source file, assemble it, output the results
{
	bool
		fail;

	fail=false;
	programName=argv[0];								// point to the program name forever more

	if(InitAssembler())
	{
		if(ParseCommandLine((unsigned int)argc,argv))
		{
			if(!infoOnly)
			{
				fail=!HandleAssembly();
			}
		}
		else
		{
			Usage();
			fail=true;
		}
		UnInitAssembler();
	}
	else
	{
		ReportComplaint(true,"Failed to initialize\n");
		fail=true;
	}

	// check to see if leaking memory
	if(numAllocatedPointers)
	{
		fprintf(stderr,"Yikes!! had %d un-deallocated pointers @ exit!\n",numAllocatedPointers);
	}
	if(fail||errorCount)
	{
		return(1);										// tell OS bad things happened
	}
	return(0);
}
