tpasm README

The current version of tpasm can be found at:

http://www.sqrt.com

Please read the file LICENSE.TXT and make sure you agree with it
before installing tpasm.

tpasm is a command line based cross assembler for microcontrollers. It
should compile and function on most any Unix based system, and may
even work under DOS (I will leave that for someone else to try).

It currently supports:

Rockwell 6502
  6502  65c02
Motorola 6805
  6805   68hc05 68705  68hc08
Motorola 6809
  6809
Motorola 68hc11
  68hc11
Intel 8051
  8031   8032   8051   8052   80c390
Atmel AVR
  avr       attiny10  attiny11  attiny12  attiny15  attiny22  attiny28
  at90s1200 at90s2313 at90s2323 at90s2333 at90s2343 at90s4414 at90s4433
  at90s4434 at90s8515 at90c8534 at90s8535 atmega8   atmega103 atmega161
INFINEON C166
  c166
Core Technologies CTXP-1
  ctxp1
Microchip PIC
  12c508  12c508a 12c509  12c509a 12ce518 12ce519 12c671  12c672  16c505
  16c52   16c54   16c55   16c56   16c57   16c58   14000   16c554  16c556
  16c558  16c61   16c62   16c62a  16c62b  16c620  16c620a 16c621  16c621a
  16c622  16c622a 16c63   16c63a  16c64   16c64a  16c642  16c65   16c65a
  16c65b  16c66   16c662  16c67   16c71   16c71a  16c710  16c711  16c715
  16c72   16c72a  16c73   16c73a  16c73b  16c74   16c74a  16c74b  16c76
  16c77   16f627  16f627a 16f628  16f628a 16f648  16f648a 16f818  16f819
  16f83   16c84   16f84   16f84a  16f873  16f874  16f876  16f877  16c923
  16c924  12f629  12f675  12f683  17c42   17c43   17c44   17c52   17c56
Sunplus SPCxxx
  spc08a   spc21a   spc21a1  spc41a   spc41b   spc41b1  spc41c   spc81a
  spc121a  spc251a  spc512a  spc500a1 spc1000a spc2000a
Zilog Z80
  z80  z180


Although it is fairly complete, and bug free as far as
I know, it is bound to contain some bugs I have not found yet.

Please keep a sharp eye out for incorrectly generated code
and other subtle problems. If you discover any, please forward
them to me and I will fix them as soon as I can.

To install, read the file INSTALL.TXT.

To learn how to use it, read the file MANUAL.TXT.

If you wish to add new processor support to tpasm, just place a source
file containing support for that processor into the "processors"
directory, and issue "make" from the outer directory. The rest will
happen automatically -- there is no need to touch any other source
files, or makefiles.

I mean to go into more detail soon about how to write new processor
support, but that documentation does not exist yet.

If you are still motivated to add another processor to tpasm, take
a look at any of the source files in the "processors" directory for
clues about what to do.

If you add another processor to the assembler, I will be happy to
include it in the next release. Just send it to me.

Todd Squires
squirest@ct0.com

-----------------------------------------------------------------
Release of 1.11

Added some updates from Vernon Brooks that help tpasm build better
under Windows.

Added new output file type "incl" submitted by Ilu.

-----------------------------------------------------------------
Release of 1.10

Generate a warning when a negative repeat count is given.

-----------------------------------------------------------------
Release of 1.9

Fixed precedence of alias substitutions (they were occurring too
late to be used within other assembler pseudo-ops).

-----------------------------------------------------------------
Release of 1.8

Added -s (strict pseudo-op) option to force all pseudo-ops to begin
with a . (dot) so that they keep out of the way of opcodes for some
processors (like "set" on the z80 and AVR).

Change source file opens to be read-only.

Added some PIC processor support thanks to Marcos Vives Del Sol.

-----------------------------------------------------------------
Release of 1.7

Fixed ordering of bytes when using DW with AVR processors.

-----------------------------------------------------------------
Release of 1.6

Added c166 support from Gustaf Kröling.

-----------------------------------------------------------------
Release of 1.5

Cleaned up compile warnings from newer versions of gcc.

-----------------------------------------------------------------
Release of 1.4

Added 68hc08 support.

Modified segment code so that segments are output in the order
in which they are created (instead of the reverse order as was
happening previously).

Modified local label handling to draw a distinction between local
labels beginning with '.', and those beginning with '@'. Local labels
beginning with '.' now stay in scope within macros so that macros can
be written which reference local labels outside the macro. Local
labels beginning with '@' are only in scope within the macro. This
allows macros to use local labels which do not interfere with the code
in which they are invoked.

Modified "incbin" to speed up intermediate assembly steps when
including large files.

Allowed macro expansions to contain \ quoted characters which are
expanded literally. Among other things, this allows macros to create
strings from input parameters as in \"parameter\".

-----------------------------------------------------------------
Release of 1.3

Added support for a couple more PICs (16f818, 16f819)

Cleaned up int/unsigned int definitions.

Added refCount field to labels so that unreferenced labels can be
marked in the listing file.

-----------------------------------------------------------------
Release of 1.2

Added support for a couple of PICs (12f629, and 12f675)

Fixed bug in Z80's CPDR instruction generation (was generating 0xed89,
should have generated 0xedb9). Thanks to Chris Williams for pointing
that out.

-----------------------------------------------------------------
Release of 1.1

Got rid of edition letters in tpasm's version number (as they were
nearing the end of the alphabet, and not all that meaningful anyway).

Changed IFDEF and IFNDEF so that they recognize labels which
are forward-defined. Thanks to John Moore for pointing out that
this should be done.

Updated the manual to make IFDEF/IFNDEF behavior more clear.

Changed initialization of processor objects to be easier to
modify/add to.

Added 16f627 and 16f628 PIC support.

Modified ALIAS so that it can accept a quoted replacement
string containing arbitrary characters.

Updated the manual to reflect new ALIAS behavior.

Sped up INCBIN for those processors that have it.

tpasm now keeps track of the minimum and maximum DS (define space)
commands within a segment and will take them into account when
reporting the segment size to the listing file. Previously if you had
a segment with only DS commands in it (like you might when declaring
zero-page variables for instance), tpasm's listing would always show
the segment as containing no bytes (because it only kept track of
generated output bytes).

Added -lstdc++ to the makefile for the benefit of those
running OS X. Thanks to Robert Zusman for pointing this out.

-----------------------------------------------------------------
Release of 1.0v

Found/fixed some problems with relative offsets in AVR code.

Added support for AVR ATmega8.

Fixed bug in register aliasing where memcpy was being called
instead of memmove (needed to handle overlapping cases).

Andrew Pines changed the 6502 code to better support the BRK
instruction.

-----------------------------------------------------------------
Release of 1.0u

Richard Reeve pointed out that I broke a couple of the PIC
pseudo-ops last time I messed with everything. That has been
corrected.

-----------------------------------------------------------------
Release of 1.0t

Added 6809 support (not that anybody is manufacturing it anymore,
but I needed it for a project I am working on)

Also, fixed up some checks of currentSegment (there was some code
which was trying to use it without checking to see if it was set
up or not).

RORG allows tpasm to generate code which will be relocated later.
tpasm used to complain if the output location of this code (before
relocation) was outside of the processor's address space. It has
been modified so that it will now only complain if relocated
addresses will fall outside the processor's address space. Thanks
to Andrew Pines for making this change.

Jonathan Woithe, jwoithe@physics.adelaide.edu.au added the `alias'
pseudo-opcode. Among other things this allows a more meaningful string
to be used when using registers in the source which correspond to 
global variables.

Made some major changes to the way the processor modules handle
pseudo-ops. Hopefully I did not break anything -- if you have
code which used to assemble, but now does not, let me know.

Cleaned up byte handling in the AVR HandleDB, DW, and incbin
routines.

Modified all the copyright messages for the new year.
-----------------------------------------------------------------
Release of 1.0s

Changed some PIC errors into warnings based on a suggestion from
Nigel Orr.

Added support for the CTXP1 (A Xilinx Virtex based processor core
which was created for a project I am working on, and which no one but
me cares about :-).

Added some PIC processor definitions supplied by Reto Felix.

Added special symbol $$ which gives the PC without the effects
of "RORG".

-----------------------------------------------------------------
Release of 1.0r

Fixed bug in 8051 code which was allowing invalid bit addresses.

-----------------------------------------------------------------
Release of 1.0q

Fixed ldx offset by y instruction in 6502 processor
(Thanks Andrew)

Makefile was updated so that .o files always end up in
their respective directories (Thanks Dan)

Makefile was updated to use patterns for locating processors and
output file formats. This allows processors and output file formats to
be added to the assembler simply by placing their source files into
their respective directories. Everything else is automatic.

-----------------------------------------------------------------
Release of 1.0p

Fixed incorrect processor definitions for pic12c671, and pic12c672
(Thanks Jerome)

Updated the copyright message.

-----------------------------------------------------------------
Release of 1.0o

A bug in the z80 module which allowed out of range forward
branches was corrected.

Fixed problem with RORG.

Fixed some stuff to make Windows compilation easier.

Fixed a bug in the z80 code which was referencing an undefined
local variable.

Some AVR changes.

-----------------------------------------------------------------
Release of 1.0n

Fixed bug in processor name dumping routine which caused it
to exceed 80 columns.

Enhanced AVR support.

-----------------------------------------------------------------
Release of 1.0m

Changed hex dump files to be "text" instead of "binary".
This will not change anything on unix machines, but will
work better on DOS machines.

Added some PIC support.

-----------------------------------------------------------------
Release of 1.0l

Added motorola S-record output support. Changed -o option
to allow type to be specified. Removed redundant -s option.

Removed the ever growing processor list from the usage
message because it was getting too large, and added some
command line options to display the additional information.

-----------------------------------------------------------------
Release of 1.0k

Bug fix with respect to high and low operators being parsed
too agressively

More new processor support.

Slightly improved documentation.

-----------------------------------------------------------------
Release of 1.0h

Fixed DT pseudo-op to generate RETLW based on the PIC family
that is selected, instead of just for the 16C5X family, as
it was doing.

Modified PIC 14000 processor definition to allow assembly of
code in the calibration space.

Added a forgotten instruction (sublw) in the pic 16CXX code
and fixed 2 bugs related to bounds checking in the pic code --
thanks Neil.

1.0h -- released some new processor support

1.0g -- tpasm released

-----------------------------------------------------------------
Special thanks to:

Andrew Pines for the 68hc11 module, the sunplus module, the Z80
module, enhancements to the AVR support, testing and suggestions.

Neil Darlow for reporting some bugs in the PIC code,
providing the 80C390 support, and for providing the 8052.inc
file.

Jerome Tonneson for updates to the previously incorrect
pic12c671, and pic12c672 definitions.

Dan @ MIT for updates to the makefile.

Nigel Orr for suggestions about handling jumps/calls on
PICs with banked code space.

Reto Felix for some additional PIC definitions

Chris Williams for fixing a bug in the Z80 module
