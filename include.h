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


#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<errno.h>
#include	<ctype.h>
#include	<time.h>

#include	"defines.h"
#include	"tpasm.h"
#include	"memory.h"
#include	"files.h"
#include	"symbols.h"
#include	"label.h"
#include	"segment.h"
#include	"parser.h"
#include	"expression.h"
#include	"pseudo.h"
#include	"context.h"
#include	"alias.h"
#include	"macro.h"
#include	"globals.h"
#include	"processors.h"
#include	"support.h"
#include	"listing.h"
#include	"outfile.h"
