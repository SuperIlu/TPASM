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


SYM_TABLE_NODE *STFindNode(SYM_TABLE *table,const char *name);
SYM_TABLE_NODE *STFindNodeNoCase(SYM_TABLE *table,const char *name);
void STRemoveEntry(SYM_TABLE *table,SYM_TABLE_NODE *node);
SYM_TABLE_NODE *STAddEntryAtStart(SYM_TABLE *table,const char *name,void *data);
SYM_TABLE_NODE *STAddEntryAtEnd(SYM_TABLE *table,const char *name,void *data);
void *STFindDataForName(SYM_TABLE *table,const char *name);
void *STFindDataForNameNoCase(SYM_TABLE *table,const char *name);
SYM_TABLE_NODE *STFindFirstEntry(SYM_TABLE *table);
SYM_TABLE_NODE *STFindLastEntry(SYM_TABLE *table);
SYM_TABLE_NODE *STFindNextEntry(SYM_TABLE *table,SYM_TABLE_NODE *previousEntry);
SYM_TABLE_NODE *STFindPrevEntry(SYM_TABLE *table,SYM_TABLE_NODE *nextEntry);
const char *STNodeName(SYM_TABLE_NODE *node);
void *STNodeData(SYM_TABLE_NODE *node);
unsigned int STNumEntries(SYM_TABLE *table);
void STDisposeSymbolTable(SYM_TABLE *table);
SYM_TABLE *STNewSymbolTable(unsigned int expectedEntries);
