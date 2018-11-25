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


// Low level symbol table handling

#include	"include.h"

#define 	ST_MAX_HASH_TABLE_BITS		10		// maximum number of bits that will be used for a hash table lookup

struct SYM_TABLE_NODE
{
	SYM_TABLE_NODE
		*next,
		*prev,
		*hashNext,
		*hashPrev;
	void
		*data;
	unsigned int
		hashValue;								// value that the key below hashes to
	char
		name[1];								// variable length array which contains the name of the symbol
};

struct SYM_TABLE
{
	SYM_TABLE_NODE
		*first,									// linearly linked table
		*last;
	unsigned int
		hashTableBits;							// tells how many bits of the hash value are used to index the hash list (also implies the size of the hash list)
	SYM_TABLE_NODE
		*hashList[1];							// variable length hash table
};

static unsigned int STHash(const char *name)
// Return the full hash value for the given name.
{
	unsigned int
		hash;
	unsigned int
		count;
	unsigned char
		c;

	hash=0;
	for(count=0;(c=name[count]);count++)
	{
		c=tolower(c);						// make hash case-insensitive so we can search either way
		hash^=1<<(c&0x0F);
		hash^=((c>>4)&0x0F)<<(count&0x07);
	}
	return(hash);
}

static inline bool STCompNames(const char *name1,const char *name2)
// Compares the given names and returns true if they are the same.
{
	return(strcmp(name1,name2)==0);
}

static inline bool STCompNamesNoCase(const char *name1,const char *name2)
// Compares the given names and returns true if they are the same.
{
	return(strcasecmp(name1,name2)==0);
}

SYM_TABLE_NODE *STFindNode(SYM_TABLE *table,const char *name)
// Find the node with the given name.
// return NULL if none could be located
{
	unsigned int
		hashIndex;
	SYM_TABLE_NODE
		*node;

	hashIndex=STHash(name)&((1<<table->hashTableBits)-1);
	
	node=table->hashList[hashIndex];
	while(node&&!STCompNames(name,node->name))
	{
		node=node->hashNext;
	}
	return(node);
}

SYM_TABLE_NODE *STFindNodeNoCase(SYM_TABLE *table,const char *name)
// Find the first node with the given (case insensitive) name.
// return NULL if none could be located
{
	unsigned int
		hashIndex;
	SYM_TABLE_NODE
		*node;

	hashIndex=STHash(name)&((1<<table->hashTableBits)-1);
	
	node=table->hashList[hashIndex];
	while(node&&!STCompNamesNoCase(name,node->name))
	{
		node=node->hashNext;
	}
	return(node);
}

void STRemoveEntry(SYM_TABLE *table,SYM_TABLE_NODE *node)
// Removes any entry with the given name from the table.
{
	if(node->hashNext)							// first unlink it from the hash table
	{
		node->hashNext->hashPrev=node->hashPrev;
	}

	if(node->hashPrev)
	{
		node->hashPrev->hashNext=node->hashNext;
	}
	else
	{
		table->hashList[node->hashValue&((1<<table->hashTableBits)-1)]=node->hashNext;
	}

	if(node->prev==NULL)							// then unlink it from the ordered list
	{
		table->first=node->next;
		if(node->next==NULL)
		{
			table->last=NULL;
		}
		else
		{
			node->next->prev=NULL;
		}
	}
	else
	{
		node->prev->next=node->next;
		if(node->next==NULL)
		{
			table->last=node->prev;
		}
		else
		{
			node->next->prev=node->prev;
		}
	}

	DisposePtr(node);							// and then destroy it
}

static SYM_TABLE_NODE *STCreateNode(const char *name,void *data)
// Returns a new node containing the given name and data.
// NOTE: the hash value is calculated at this time, and placed into
// the node as well
{
	SYM_TABLE_NODE
		*newNode;

	if((newNode=(SYM_TABLE_NODE*)NewPtr(sizeof(SYM_TABLE_NODE)+strlen(name)+1)))
	{
		newNode->data=data;
		newNode->hashValue=STHash(name);
		strcpy(newNode->name,name);
		return(newNode);
	}
	return(NULL);
}

SYM_TABLE_NODE *STAddEntryAtStart(SYM_TABLE *table,const char *name,void *data)
// Adds the given name and data to table as the first entry.
// It is allowed for two names in the table to have the same value, but
// it is not guaranteed which will be returned when searching
{
	SYM_TABLE_NODE
		*newNode;
	unsigned int
		hashIndex;

	if((newNode=STCreateNode(name,data)))
	{
		hashIndex=newNode->hashValue&((1<<table->hashTableBits)-1);
		if(table->hashList[hashIndex])
		{
			table->hashList[hashIndex]->hashPrev=newNode;
		}
		newNode->hashNext=table->hashList[hashIndex];					// link into hash list
		newNode->hashPrev=NULL;
		table->hashList[hashIndex]=newNode;

		if(table->first)												// then link into ordered list
		{
			table->first->prev=newNode;
		}
		else
		{
			table->last=newNode;										// first entry in the table
		}
		newNode->next=table->first;
		newNode->prev=NULL;	
		table->first=newNode;
	}
	return(newNode);
}

SYM_TABLE_NODE *STAddEntryAtEnd(SYM_TABLE *table,const char *name,void *data)
// Adds the given name and data to table as the last entry.
// It is allowed for two names in the table to have the same value, but
// it is not guaranteed which will be returned when searching
{
	SYM_TABLE_NODE
		*newNode;
	unsigned int
		hashIndex;

	if((newNode=STCreateNode(name,data)))
	{
		hashIndex=newNode->hashValue&((1<<table->hashTableBits)-1);

		if(table->hashList[hashIndex])
		{
			table->hashList[hashIndex]->hashPrev=newNode;
		}
		newNode->hashNext=table->hashList[hashIndex];					// link into hash list
		newNode->hashPrev=NULL;
		table->hashList[hashIndex]=newNode;

		if(table->last)													// then link into ordered list
		{
			table->last->next=newNode;
		}
		else
		{
			table->first=newNode;										// first entry in the table
		}
		newNode->prev=table->last;
		newNode->next=NULL;	
		table->last=newNode;
	}
	return(newNode);
}

void *STFindDataForName(SYM_TABLE *table,const char *name)
// Finds the data for name in the table.
// Returns pointer to the node's data on success, NULL otherwise.
{
	SYM_TABLE_NODE
		*curNode;

	if((curNode=STFindNode(table,name)))
	{
		return(curNode->data);
	}
	return(NULL);
}

void *STFindDataForNameNoCase(SYM_TABLE *table,const char *name)
// Finds the data for (case insensitive) name in the table.
// Returns pointer to the node's data on success, NULL otherwise.
{
	SYM_TABLE_NODE
		*curNode;

	if((curNode=STFindNodeNoCase(table,name)))
	{
		return(curNode->data);
	}
	return(NULL);
}

SYM_TABLE_NODE *STFindFirstEntry(SYM_TABLE *table)
// Return the first node linked to table
{
	return(table->first);
}

SYM_TABLE_NODE *STFindLastEntry(SYM_TABLE *table)
// Return the last node linked to table
{
	return(table->last);
}

SYM_TABLE_NODE *STFindNextEntry(SYM_TABLE *table,SYM_TABLE_NODE *previousEntry)
// return the entry just after previousEntry in table
{
	return(previousEntry->next);
}

SYM_TABLE_NODE *STFindPrevEntry(SYM_TABLE *table,SYM_TABLE_NODE *nextEntry)
// return the entry just before nextEntry in table
{
	return(nextEntry->prev);
}

const char *STNodeName(SYM_TABLE_NODE *node)
// Return the name of the passed symbol table node.
{
	return(node->name);
}

void *STNodeData(SYM_TABLE_NODE *node)
// Return the data of the passed symbol table node.
{
	return(node->data);
}

unsigned int STNumEntries(SYM_TABLE *table)
// Returns the number of entries in the table.
{
	SYM_TABLE_NODE
		*curEnt;
	unsigned int
		numEnts;

	numEnts=0;

	curEnt=table->first;
	while(curEnt!=NULL)
	{
		numEnts++;
		curEnt=curEnt->next;
	}

	return(numEnts);
}

void STDisposeSymbolTable(SYM_TABLE *table)
// Dispose of all memory allocated for the given symbol table.  Note that
// the links are not maintained during the process.
{
	SYM_TABLE_NODE
		*curEnt,
		*tmpEnt;

	curEnt=table->first;
	while(curEnt)
	{
		tmpEnt=curEnt->next;
		DisposePtr(curEnt);
		curEnt=tmpEnt;
	}
	DisposePtr(table);
}

SYM_TABLE *STNewSymbolTable(unsigned int expectedEntries)
// Create and initialize a new symbol table.
// expectedEntries is used to work out the size of the hash table.
// If it is passed as 0, it suggests that the number of entries could
// be arbitrarily large.
{
	SYM_TABLE
		*table;
	unsigned int
		count;
	unsigned int
		hashTableBits;

	if(expectedEntries)
	{
		hashTableBits=0;
		while((hashTableBits<ST_MAX_HASH_TABLE_BITS)&&(((unsigned int)(1<<hashTableBits))<expectedEntries/2))	// shoot for two entries per hash bucket
		{
			hashTableBits++;
		}
	}
	else
	{
		hashTableBits=ST_MAX_HASH_TABLE_BITS;
	}

	if((table=(SYM_TABLE*)NewPtr(sizeof(SYM_TABLE)+(1<<hashTableBits)*sizeof(SYM_TABLE_NODE *))))
	{
		table->first=table->last=NULL;
		table->hashTableBits=hashTableBits;
		for(count=0;count<(unsigned int)(1<<hashTableBits);count++)
		{
			table->hashList[count]=NULL;
		}
		return(table);
	}
	return(NULL);
}
