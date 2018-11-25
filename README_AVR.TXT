From: Jonathan Woithe    jwoithe@physics.adelaide.edu.au

AVR notes
=========
The program memory in the Atmel AVR processor family is addressed as 16 bit
words.  The SRAM is addressed as 8 bit bytes.  This has some consequences
when it comes to locations and addresses reported in the listing file.

The "location" reported for program code refers to 16-bit data locations.
As a result, throughout the listing, "location 0x0002" refers to byte number 
4 for example.  The address ranges reported for segments in the listing file
are in bytes.  If the last program code is in "location 0x32", the "maximum
address" for the segment will be 0x64 (bytes).

If using "segu" to define labels for variables in SRAM, the "location"
should be treated by the programmer as being in bytes both when defining the
segment and referring to its labels in operands.  However, internally tpasm
still treats it as a 16 bit word.  Because such segments are never output as
code this doesn't have any practical consequence.  Labels used as operands
will be expanded to their "location" value (as is normal tpasm practice) and
therefore the code produced will be correct.  However, the segment's minimum
and maximum addresses reported in the listing file will be two times the
actual maximum (byte-based) address in use.  Consider for example the
following.
        segu    data
        org     0x0060
  V1:   db      0x01
  V2:   db      0x01 0x02

V1 will be assigned "location" 0x0060 and V2 0x0061.  The last address used
is 0x0062.  This segment's address range will be reported as 0xC0 to 0xC4,
whereas in reality it spans byte addresses 0x0060 to 0x0062.  If (for some
reason) such a "data" segment is defined using "seg" rather than "segu", the
addresses reported in the output file will also be doubled, since the output
file's addresses are in terms of bytes.
