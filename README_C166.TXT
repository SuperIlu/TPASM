This is a minimal implementation of the C166 instruction set used by
a number of processors from STMicroelectronics, Siemens and Infineon.

Some (older) processors have issues with pipeline delays putting
restrictions on the code. Most assemblers can handle this by inserting
'nop's or at least give a warning. Not so this assembler!

It is common that the assembler keeps track of protected instruction
ranges to warn you about possible mistakes. Not so this assembler!

It also seems that assemblers try to help you by fixing or warning when
a known hardware bug may result in unexpected things. Not so this assembler!

tpasm does not have the 'standard' names of all registers hardcoded in it.
Instead they are meant to be "included" from a header file together with
some other usable stuff normally known to the assembler by default. This
is partly because not all processors have the same memory layout. Two
files REG_XE167xM.INC and REG_167.INC are currently available in the 
'include' subdirectory. Others may come.

tpasm has a SEG pseudo-op that is similar to the SECTION directive used
with C166. The tpasm *operator* SEG is used for getting the 64k segment
number of an operand address. Confusing!

There are no datatypes like NEAR, FAR, SHORT and so on. This may limit
the freedom of creating finetuned code but since tpasm does not support
linking of libraries other than using 'include' you will probably use
some other assembler for big projects anyway.

The instruction set used by the MAC coprocessor is not (yet) implemented.

Many processors can handle more than 64k of memory. This memory is divided
in chunks of 64k for code and 16k for data. This is called SEGMENTED mode.
If in SEGMENTED mode you have to use the ASSUME directive to tell tpasm
how to assemble memory addresses.

There is just one processor type as of now called C166. Some old processors
don't execute all of these instructions.

In general you are expected to know what you are doing. The assembler
is just there to produce the binary for you :-)

Have fun





Here follows a list of things that are not supported or need recoding.
The tpasm syntax very often differs from what is normal with these
processors so don't expect to port things without recoding. Most of the
stuff used when building programs by linking modules is of no meaning here
and has been left out. Note that tpasm can include files from many 
directories. See also the file CONVERSION_166.TXT for more info.

Directives not supported:

ENDS	Marks the end of a 'SECTION'. Not needed by tpasm and conflicts
	with ENDS used by tpasm to signal the end of a SWITCH directive  
        
GROUP	Ignored

DGROUP	Ignored

REGBANK	Ignored

REGDEF	Ignored

PECDEF	Ignored

SSKDEF	Ignored

PROC	Ignored

ENDP	Ignored

BIT	Use EQU and/or LIT to give a bit a name

LABEL

DEFA	Handled as EQU

DEFB	Handled as LIT

DEFR	Handled as EQU

TYPEDEC

EXTRN	Ignored

EXTERN	Ignored

PUBLIC	Ignored

GLOBAL	Ignored

DBIT

DBPTR

DPPTR

DSPTR

NAME	Ignored


Directives replaced:

SECTION	Use SEG to get a similar functionality.


Operators not supported:

PTR

DATA3

DATA4

DATA8

DATA16

SHORT

BOF


Assembler controls not supported:

ABSOLUTE

CASE/NOCASE

COND/NOCOND

DATE

DEBUG/NODEBUG

EJECT

ERRORPRINT/NOERRORPRINT

EXPDECNUM

NOGEN			Use EXPAND to expand macros *and* repeats

GENONLY

GEN

INCDIR			See the command line options of tpasm

MACRO/NOMACRO

MOD166/NOMOD166

MOD167

OBJECT/NOOBJECT

PAGELENGTH

PAGEWIDTH

PAGING/NOPAGING

PRINT/NOPRINT		See LIST, NOLIST and command line option -l

REGUSE

SAVE/RESTORE

SEGMENTED/NONSEGMENTED

SYMBOLS/NOSYMBOLS

TABS

TITLE

TYPE/NOTYPE

USEDEXTONLY

XREF/NOXREF


Directives for conditional assemby:

SET/RESET		Corresponds to SET and UNSET in tpasm

ELSEIF			Not present in tpasm

IF/ELSE/ENDIF		Have simuilar meaning in tpasm


Macros:

tpasm has macro capability but functionality and syntax differs. See
the tpasm MANUAL.TXT for a short description of macros.