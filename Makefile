#	Copyright (C) 1999-2012 Core Technologies.
#
#	This file is part of tpasm.
#
#	tpasm is free software; you can redistribute it and/or modify
#	it under the terms of the tpasm LICENSE AGREEMENT.
#
#	tpasm is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	tpasm LICENSE AGREEMENT for more details.
#
#	You should have received a copy of the tpasm LICENSE AGREEMENT
#	along with tpasm; see the file "LICENSE.TXT".

INCLUDES=-I.

# new gnu compilers have a really annoying default behavior of wrapping
# error message lines. The default should be NO-WRAPPING.
OPTIONS=-O2 -Wall -x c++ -fmessage-length=0 -fno-exceptions
CFLAGS=$(INCLUDES) $(OPTIONS)

OBJECTS = \
	globals.o \
	tpasm.o \
	memory.o \
	files.o \
	symbols.o \
	label.o \
	segment.o \
	pseudo.o \
	parser.o \
	expression.o \
	context.o \
	alias.o \
	macro.o \
	listing.o \
	outfile.o \
	processors.o \
	support.o \
	$(patsubst %.c,%.o,$(wildcard outfiles/*.c)) \
	$(patsubst %.c,%.o,$(wildcard processors/*.c))

all : tpasm

tpasm : $(OBJECTS)
	$(CC) -O $(OBJECTS) -lstdc++ -o tpasm

clean :
	rm -f *.o
	rm -f outfiles/*.o
	rm -f processors/*.o
	rm -f tpasm

install :
	cp tpasm /usr/local/bin

# suffix rules (this makes sure that the ".o" files
# end up in their respective directories on all systems)
.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

