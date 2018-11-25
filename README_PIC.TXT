If you are assembling for the PIC, you may wish to include
the file:

include/picmacros.inc

into your source. It provides some macros to implement
some common commands.

Also, you should be able to include the processor header files
provided by microchip as part of mplab with little or no modification.
These header files are handy to define all the processor register
names, etc...

The current version of TPASM supports the following pseudo-ops
to make it somewhat compatible with mpasm:
__config
__idlocs
__maxram
__badram
banksel
bankisel
pagesel

Most of the other functionality of mpasm can be had with
tpasm, but will probably require different syntax.

If you are using tpasm because you have mplab, but dont like
working on the PC, you might also want to check out a program
called "picp". This program allows a Linux box (and presumably
other Unix boxes as well) to directly talk to the PICStart Plus
programmer.
picp is available at:

www.cosmodog.com/pic

