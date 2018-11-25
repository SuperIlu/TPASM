// AVR test code
// assemble this with:
// tpasm -P avr avrtest.asm -l avrtest.lst

RAMPZ	equ		0
SPMCR	equ		0
temp	alias		r16
	processor	avr
;	org		0x1FA009
	eor		r19,r19		; Clear r19
	AND		R16,R7
loop:
	inc		r19		; Increase r19
	and		r0,temp
	ldi		temp,99		//this
	ldd		r10,Y + 33
	cpi		r19,$10	; Compare r19 with $10
	brlo		loop		; Branch if r19 < $10 (unsigned)
	nop				; Exit from loop (do nothing)
	movw		r2,r30
	elpm		r2,Z+
	clv
	db		3,5,7,9,11
	db		"This is a temp string"
;	org		0x10A000
	brmi		loop2
	nop
	sleep
	db		3,5
	adc		R5,R6
	adiw		r24,16
	asr		R4
	bclr		7
	brbc		7,loop2
	bld		r9,1
loop2:
	clr		r31
	clr		r30
	ldi		temp,$F0
	out		RAMPZ, r16
	ldi		r16, $CF
	mov		r1, r16
	ldi		r16, $FF
	mov		r0, r16
	ldi		r16,$03
	out		SPMCR, r16
	espm
	ldi		r16,$01
	out		SPMCR, r16
	espm
	ldi		r16,$05
	out		SPMCR, r16
	espm
	ser		R16
	st		-x,r6
;	st		x+5,r6
	st		y+5,r6
	clr		r29
	ldi		r28,$60
	st		Y+,r0
	st		Y,r1
	ldi		r28,$63
	st		Y,r2
	st		-Y,r3
	std		Y+2,r4
	lds		r2,$FF00
	add		r2,r1
	sts		$FF00,r2
	inc		r1
	swap		r1
	inc		r1
	swap		r1
	tst		r10