# TPASM
Mirror of tpasm sources with DOS and WIN32 binray releases. TPASM is "a Unix based cross assembler for 6805, 6809, 68HC11, 6502, Sunplus, 8051, Z80, PIC, AVR, and c166" (now with DOS and WIN32 binaries).

This is a copy of the official TPASM sources from [http://www.sqrt.com/](http://www.sqrt.com/). Todd still maintains TPASM, so please contact him for any issues regarding assembly errors or improvements.

## Added files
* **Makefile.dj2**: Makefile to build TPASM using [DJGPP2](http://www.delorie.com/djgpp/). You will need a DOS/Windows system supporting 16-Bit binaries and LFN (long file names) to build. E.g.: [FreeDOS](http://www.freedos.org/), [DOSBox-LFN](https://sourceforge.net/projects/dosbox-svn-lfn/) or Win9x.
* **tpasm-1.11.dev**: Projectfile for [Dev-Cpp](https://sourceforge.net/projects/orwelldevcpp/)

## Binaries
The DOS binaries were created with DJGPP, gcc version 8.2.0.

The WIN32 binaries were created using Dev-Cpp 5.11 and TDM-GCC 4.9.2.
