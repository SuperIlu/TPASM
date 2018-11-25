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


// memory allocation routines

#include	"include.h"

void DisposePtr(void *pointer)
// Free memory allocated by NewPtr
{
	free(pointer);
	numAllocatedPointers--;
}

void *NewPtr(unsigned int size)
// allocate memory, keep track of the number of allocated pointers to
// help track memory leaks
{
	void
		*result;

	result=malloc(size);
	if(result)
	{
		numAllocatedPointers++;
	}
	return(result);
}
