		nolist
// PIC cross assembler macros to take care of special mnemonics
// These were lifted directly from the PIC Quick Reference Guide

addcf		macro		f,d
		btfsc		3,0
		incf		f,d
		endm

adddcf		macro		f,d
		btfsc		3,1
		incf		f,d
		endm

b		macro		k
		goto		k
		endm

bc		macro		k
		btfsc		3,0
		goto		k
		endm

bdc		macro		k
		btfsc		3,1
		goto		k
		endm

bnc		macro		k
		btfss		3,0
		goto		k
		endm

bndc		macro		k
		btfss		3,1
		goto		k
		endm

bnz		macro		k
		btfss		3,2
		goto		k
		endm

bz		macro		k
		btfsc		3,2
		goto		k
		endm

clrc		macro
		bcf		3,0
		endm

clrdc		macro
		bcf		3,1
		endm

clrz		macro
		bcf		3,2
		endm

movfw		macro		f
		movf		f,0
		endm

negf		macro		f,d
		comf		f,1
		incf		f,d
		endm

setc		macro
		bsf		3,0
		endm

setdc		macro
		bsf		3,1
		endm

setz		macro
		bsf		3,2
		endm

skpc		macro
		btfss		3,0
		endm

skpdc		macro
		btfss		3,1
		endm

skpnc		macro
		btfsc		3,0
		endm

skpndc		macro
		btfsc		3,1
		endm

skpnz		macro
		btfsc		3,2
		endm

skpz		macro
		btfss		3,2
		endm

subcf		macro		f,d
		btfsc		3,0
		decf		f,d
		endm

subdcf		macro		f,d
		btfsc		3,1
		decf		f,d
		endm

tstf		macro		f
		mov		f,1
		endm

		list
