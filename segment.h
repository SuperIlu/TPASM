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


bool AddBytesToSegment(SEGMENT_RECORD *segment,unsigned int address,unsigned char *bytes,unsigned int numBytes);
bool AddSpaceToSegment(SEGMENT_RECORD *segment,unsigned int address,unsigned int numBytes);
SEGMENT_RECORD *MatchSegment(const char *segment);
void DestroySegment(SEGMENT_RECORD *segment);
void DestroySegments();
SEGMENT_RECORD *CreateSegment(const char *segmentName,bool generateOutput);
void DumpSegmentsListing(FILE *file);
void UnInitSegments();
bool InitSegments();
