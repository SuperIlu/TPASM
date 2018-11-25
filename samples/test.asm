// Sample source file demonstrating/testing tpasm
// This file contains some random junk to show what tpasm's
// syntax looks like, and to expose some of its features.
// (this file uses 8 space tabs)
// assemble this with:
// tpasm -I ../include -l test.lst -o intel test.hex test.asm

// cause assembler to have to make a few passes:
testA		.equ		testB
testB		.equ		testC
testC		.equ		testD
testD		.equ		testE
testE		.equ		testF
testF		.equ		$AA55
high_count	.equ		0

nopMacro	macro		numWanted
		repeat		numWanted
		nop
		endr
		endm

pascalString	macro		theString
		DB		strlen(theString),theString
		endm

CString		macro		theString
		DB		theString,$00
		endm

		processor 65c02			// select a processor

BEL		EQU		$07		// Some handy ASCII definitions
BS		EQU		$08
LF		EQU		$0A
CR		EQU		$0D
SPACE		EQU		$20
DEL		EQU		$7F

TERMINAL	EQU		$FFA0		// location of serial port

		db		"this\a\v\t\b\r\xaa\077\\\0"
.loop:
		lda		high_count
		lda		#BEL

		sta		TERMINAL	// make annoying sounds
		nopMacro	20
		jmp		.loop		// local label
		pascalString	"pascal style string with length at the start\r\n"
		CString		"C style string with 0 termination at the end\r\n"

		processor 16c505		// swap architectures midstream

		include	<picmacros.inc>		// get some helpful macros
		include	<p16c505.inc>		// pull in processor definition file (you should be able to take these from mplab)

		__config H'004'			; disable reset, watchdog, use internal RC clock, RB4 is available
		__idlocs H'1234'
		SEGU	data

		org	8

temp1		ds	1			; temporary storage
serialByte	ds	1			; hold the byte being sent out serially
parityValue	ds	1			; calculates parity
bitCounter	ds	1			; keeps track of the number of bits send serially

		SEG	code
burn		macro		cycles
; burn the given number of cycles (which must be < 768)
; NOTE: this modifies W, and temp1
; NOTE: this will generate a maximum of 6 instructions

cycleValue	set		cycles			; make raw substitution macro parameter into a variable

		if		cycleValue>3

		movlw		(cycleValue-1)/3
		movwf		temp1
@loop
		decfsz		temp1,F			; 3 cycles per loop, except last which is 2 cycles
		goto		@loop

cycleValue	set		cycleValue-(((cycleValue-1)/3)*3)-1	; reduce by number of cycles used above

		endif
; handle any remaining cycles
		if		cycleValue>0
		nop
cycleValue	set		cycleValue-1
		endif

		if		cycleValue>0
		nop
cycleValue	set		cycleValue-1
		endif

		if		cycleValue>0
		nop
cycleValue	set		cycleValue-1
		endif

		endm

BIT_TIME	equ	D'104'		; number of instruction cycles per bit (9600 bps)
OUT_LOW		equ	0b11111110
OUT_HIGH	equ	0b11111111

SendSerialByte:
; send the byte in serialByte out a port serially at the baud rate given by
; BIT_TIME with odd parity

		movlw	OUT_LOW		; drop output low to make start bit
		tris	PORTC

		movlw	1
		movwf	parityValue	; initialize parity calculation for odd parity
		movlw	8
		movwf	bitCounter	; initialize bit counter

		burn	BIT_TIME-(4+7)	; let start bit stay low for one bit time (remove 4 for instructions before this, 7 for ones after this)
bitLoop:
		rrf	serialByte,F
		btfsc	STATUS,C	; one bit coming out?
		incf	parityValue,F	; yes, bump parity
		movlw	OUT_LOW		; assume driving output low
		btfsc	STATUS,C	; one bit coming out?
		movlw	OUT_HIGH	; yes let output go high
		tris	PORTC		; set output state as needed
		burn	BIT_TIME-(7+3)	; keep bit out there for a moment

		decfsz	bitCounter,F
		goto	bitLoop

; send parity bit
		nop			; balance with loop above
		nop
		nop
		rrf	parityValue,F
		movlw	OUT_LOW		; assume drive output low
		btfsc	STATUS,C	; one bit coming out?
		movlw	OUT_HIGH	; yes let output go high
		tris	PORTC		; set output state as needed
		burn	BIT_TIME-2	; keep bit out there for a moment

; send stop bit
		movlw	OUT_HIGH
		tris	PORTC		; set output state
		burn	BIT_TIME	; make sure stop bit stays out for at least one bit time

		retlw	0

		DT	0,1,2
		processor	14000
		DT	3,4,5
		processor	17c42
		DT	6,7,8

R0test		EQU	5
		processor 8051		// swap architectures again
		nop
		mov	24,R0test
		jmp	@A+DPTR
		clr	020h.1

// 6805 test
		processor	68hc05
		org		0x1000

		adc		#45
		adc		0x20
		adc		0x2000
		adc		0x2000,X
		adc		0x20,X
		adc		,X

		add		#45
		add		0x20
		add		0x2000
		add		0x2000,X
		add		0x20,X
		add		,X

		and		#45
		and		0x20
		and		0x2000
		and		0x2000,X
		and		0x20,X
		and		,X

		asl		0x20
		asla
		aslx
		asl		0x20,X
		asl		,X

		asr		0x20
		asra
		asrx
		asr		0x20,X
		asr		,X

		bclr		0,0x20
		bclr0		0x20
		bclr		1,0x20
		bclr1		0x20
		bclr		2,0x20
		bclr2		0x20
		bclr		3,0x20
		bclr3		0x20
		bclr		4,0x20
		bclr4		0x20
		bclr		5,0x20
		bclr5		0x20
		bclr		6,0x20
		bclr6		0x20
		bclr		7,0x20
		bclr7		0x20

		bset		0,0x20
		bset0		0x20
		bset		1,0x20
		bset1		0x20
		bset		2,0x20
		bset2		0x20
		bset		3,0x20
		bset3		0x20
		bset		4,0x20
		bset4		0x20
		bset		5,0x20
		bset5		0x20
		bset		6,0x20
		bset6		0x20
		bset		7,0x20
		bset7		0x20

		bcc		$+2-128
		bcc		$+2+127
		bcs		$+2-128
		bcs		$+2+127
		beq		$+2-128
		beq		$+2+127
		bhcc		$+2-128
		bhcc		$+2+127
		bhcs		$+2-128
		bhcs		$+2+127
		bhi		$+2-128
		bhi		$+2+127
		bhs		$+2-128
		bhs		$+2+127
		bih		$+2-128
		bih		$+2+127
		bil		$+2-128
		bil		$+2+127
		blo		$+2-128
		blo		$+2+127
		bls		$+2-128
		bls		$+2+127
		bmc		$+2-128
		bmc		$+2+127
		bmi		$+2-128
		bmi		$+2+127
		bms		$+2-128
		bms		$+2+127
		bne		$+2-128
		bne		$+2+127
		bpl		$+2-128
		bpl		$+2+127
		bra		$+2-128
		bra		$+2+127
		brn		$+2-128
		brn		$+2+127
		bsr		$+2-128
		bsr		$+2+127

		bit		#45
		bit		0x20
		bit		0x2000
		bit		0x2000,X
		bit		0x20,X
		bit		,X

		brclr		0,0x20,$+3-128
		brclr		0,0x20,$+3+127
		brclr0		0x20,$+3-128
		brclr0		0x20,$+3+127
		brclr		1,0x20,$+3-128
		brclr		1,0x20,$+3+127
		brclr1		0x20,$+3-128
		brclr1		0x20,$+3+127
		brclr		2,0x20,$+3-128
		brclr		2,0x20,$+3+127
		brclr2		0x20,$+3-128
		brclr2		0x20,$+3+127
		brclr		3,0x20,$+3-128
		brclr		3,0x20,$+3+127
		brclr3		0x20,$+3-128
		brclr3		0x20,$+3+127
		brclr		4,0x20,$+3-128
		brclr		4,0x20,$+3+127
		brclr4		0x20,$+3-128
		brclr4		0x20,$+3+127
		brclr		5,0x20,$+3-128
		brclr		5,0x20,$+3+127
		brclr5		0x20,$+3-128
		brclr5		0x20,$+3+127
		brclr		6,0x20,$+3-128
		brclr		6,0x20,$+3+127
		brclr6		0x20,$+3-128
		brclr6		0x20,$+3+127
		brclr		7,0x20,$+3-128
		brclr		7,0x20,$+3+127
		brclr7		0x20,$+3-128
		brclr7		0x20,$+3+127

		brset		0,0x20,$+3-128
		brset		0,0x20,$+3+127
		brset0		0x20,$+3-128
		brset0		0x20,$+3+127
		brset		1,0x20,$+3-128
		brset		1,0x20,$+3+127
		brset1		0x20,$+3-128
		brset1		0x20,$+3+127
		brset		2,0x20,$+3-128
		brset		2,0x20,$+3+127
		brset2		0x20,$+3-128
		brset2		0x20,$+3+127
		brset		3,0x20,$+3-128
		brset		3,0x20,$+3+127
		brset3		0x20,$+3-128
		brset3		0x20,$+3+127
		brset		4,0x20,$+3-128
		brset		4,0x20,$+3+127
		brset4		0x20,$+3-128
		brset4		0x20,$+3+127
		brset		5,0x20,$+3-128
		brset		5,0x20,$+3+127
		brset5		0x20,$+3-128
		brset5		0x20,$+3+127
		brset		6,0x20,$+3-128
		brset		6,0x20,$+3+127
		brset6		0x20,$+3-128
		brset6		0x20,$+3+127
		brset		7,0x20,$+3-128
		brset		7,0x20,$+3+127
		brset7		0x20,$+3-128
		brset7		0x20,$+3+127

		clc
		cli

		clr		0x20
		clra
		clrx
		clr		0x20,X
		clr		,X

		cmp		#45
		cmp		0x20
		cmp		0x2000
		cmp		0x2000,X
		cmp		0x20,X
		cmp		,X

		com		0x20
		coma
		comx
		com		0x20,X
		com		,X

		cpx		#45
		cpx		0x20
		cpx		0x2000
		cpx		0x2000,X
		cpx		0x20,X
		cpx		,X

		dec		0x20
		deca
		decx
		dec		0x20,X
		dec		,X

		eor		#45
		eor		0x20
		eor		0x2000
		eor		0x2000,X
		eor		0x20,X
		eor		,X

		inc		0x20
		inca
		incx
		inc		0x20,X
		inc		,X

		jmp		0x20
		jmp		0x2000
		jmp		0x2000,X
		jmp		0x20,X
		jmp		,X

		jsr		0x20
		jsr		0x2000
		jsr		0x2000,X
		jsr		0x20,X
		jsr		,X

		lda		#45
		lda		0x20
		lda		0x2000
		lda		0x2000,X
		lda		0x20,X
		lda		,X

		ldx		#45
		ldx		0x20
		ldx		0x2000
		ldx		0x2000,X
		ldx		0x20,X
		ldx		,X

		lsl		0x20
		lsla
		lslx
		lsl		0x20,X
		lsl		,X

		lsr		0x20
		lsra
		lsrx
		lsr		0x20,X
		lsr		,X

		mul

		neg		0x20
		nega
		negx
		neg		0x20,X
		neg		,X

		nop

		ora		#45
		ora		0x20
		ora		0x2000
		ora		0x2000,X
		ora		0x20,X
		ora		,X

		rol		0x20
		rola
		rolx
		rol		0x20,X
		rol		,X

		ror		0x20
		rora
		rorx
		ror		0x20,X
		ror		,X

		rsp

		rti

		rts

		sbc		#45
		sbc		0x20
		sbc		0x2000
		sbc		0x2000,X
		sbc		0x20,X
		sbc		,X

		sec

		sei

		sta		0x20
		sta		0x2000
		sta		0x2000,X
		sta		0x20,X
		sta		,X

		stop

		stx		0x20
		stx		0x2000
		stx		0x2000,X
		stx		0x20,X
		stx		,X

		sub		#45
		sub		0x20
		sub		0x2000
		sub		0x2000,X
		sub		0x20,X
		sub		,X

		swi
		tax

		tst		0x20
		tsta
		tstx
		tst		0x20,X
		tst		,X

		txa
		wait

//	68hc08 test
		processor	68hc08
		div
		nsa
		daa
		tap
		tpa
		pula
		psha
		pulx
		pshx
		pulh
		pshh
		clrh
		txs
		tsx
		bge		$+2+127
		bge		$+2-128
		blt		$+2+127
		blt		$+2-128
		bgt		$+2+127
		bgt		$+2-128
		ble		$+2+127
		ble		$+2-128
		ais		#45
		aix		#45
tstLabel:
		neg		0x20,SP
		com		0x20,SP
		lsr		0x20,SP
		ror		0x20,SP
		asr		0x20,SP
		lsl		0x20,SP
		rol		0x20,SP
		dec		0x20,SP
		inc		0x20,SP
		tst		0x20,SP
		clr		0x20,SP
		sub		0x2000,SP
		sub		-1,SP
		sub		-32768,SP
		cmp		0x2000,SP
		sbc		0x2000,SP
		cpx		0x2000,SP
		and		0x2000,SP
		bit		0x2000,SP
		lda		0x2000,SP
		sta		0x2000,SP
		eor		0x2000,SP
		adc		0x2000,SP
		ora		0x2000,SP
		add		0x2000,SP
		ldx		0x2000,SP
		stx		0x2000,SP
		sub		0x20,SP
		cmp		0x20,SP
		sbc		0x20,SP
		cpx		0x20,SP
		and		0x20,SP
		bit		0x20,SP
		lda		0x20,SP
		sta		0x20,SP
		eor		0x20,SP
		adc		0x20,SP
		ora		0x20,SP
		add		0x20,SP
		ldx		0x20,SP
		stx		0xFF,SP

		cbeq		0x20,$+3+127
		cbeq		0x20,$+3-128
		cbeqa		#45,$+3+127
		cbeqa		#45,$+3-128
		cbeqx		#45,$+3+127
		cbeqx		#45,$+3-128
		cbeq		0x20,X+,$+3+127
		cbeq		0x20,X+,$+3-128
		cbeq		,X+,$+2+127
		cbeq		X+,$+2-128
		cbeq		0x20,SP,$+4+127
		cbeq		0x20,SP,$+4-128

		sthx		0x20
		ldhx		#43605
		ldhx		#-88
		ldhx		0x20
		cphx		#65535
		cphx		0x20

		dbnz		0x20,$+3+127
		dbnz		0x20,$+3-128
		dbnza		$+2+127
		dbnza		$+2-128
		dbnzx		$+2+127
		dbnzx		$+2-128
		dbnz		0x20,X,$+3+127
		dbnz		0x20,X,$+3-128
		dbnz		,X,$+2+127
		dbnz		X,$+2-128
		dbnz		0x20,SP,$+4+127
		dbnz		0x20,SP,$+4-128

		mov		0xAA,0x55
		mov		0x20,X+
		mov		#45,0x20
		mov		X+,0x20
		mov		,X+,0x20

; Random expressions
exp1		equ	0b00110111
exp2		equ	0xaBcD
exp3		equ	-0xaBcD
exp4		equ	B'00110111'
exp5		equ	H'09'
exp6		equ	A'0A'
exp7		equ	H'0B'
exp8		equ	H'0C'
exp9		equ	'ABCD'
exp10		equ	0x3eac1234
exp11		equ	B'111100001111000011110000'
exp12		equ	1!=4
exp13		equ	65!='A'
exp14		equ	'A'==('a'-0x20)
exp15		equ	!('A'==('a'-0x20))
exp16		equ	(!!('A'==('a'-0x20)))<<4	// should be 0x10
exp17		equ	$14-.19				// should be 1
exp18		equ	$-.19				// instruction pointer - 19
exp19		equ	low((high(0x11114444))<<5)
exp20		equ	strlen("this is a string")*16
exp21		equ	1^2^3^4

// mildly confusing consequences of allowing all the various numeric notations
exp22		equ	0b111		; binary
exp23		equ	0b111h		; hex
exp24		equ	0111b		; binary
exp25		equ	0111bh		; hex
exp26		equ	0111o		; octal
exp27		equ	0111d		; decimal
exp28		equ	0111dh		; hex
exp29		equ	0d111		; decimal
exp30		equ	0d111h		; hex
exp31		equ	0111		; decimal
exp32		equ	0b0		; binary
exp33		equ	0b		; binary
exp34		equ	1b		; binary
exp35		equ	0h		; hex
exp37		equ	0d		; decimal
exp38		equ	5d		; decimal
exp39		equ	5dh		; hex
exp40		equ	0o		; octal
exp41		equ	0o0		; octal

		end
