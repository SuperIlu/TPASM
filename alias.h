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


ALIAS_RECORD *MatchAlias(const char *operand);
bool HandleAliasMatches(char *line,unsigned int *lineIndex,LISTING_RECORD *listingRecord);
void DestroyAlias(ALIAS_RECORD *alias);
void DestroyAliases();
ALIAS_RECORD *CreateAlias(const char *aliasName,const char *aliasString);
void UnInitAliases();
bool InitAliases();
