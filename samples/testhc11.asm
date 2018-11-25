



		seg	code
		org	0x1000

reset:

; test all of the inherent instructions
;
		aba			; 1B
		abx			; 3A
		aby			; 18 3A
		asla			; 48
		aslb			; 58
		asld			; 05
		asra			; 47
		asrb			; 57
		cba			; 11
		clc			; 0C
		cli			; 0E
		clra			; 4F
		clrb			; 5F
		clv			; 0A
		coma			; 43
		comb			; 53
		daa			; 19
		deca			; 4A
		decb			; 5A
		des			; 34
		dex			; 09
		dey			; 18 09
		fdiv			; 03
		idiv			; 02
		inca			; 4C
		incb			; 5C
		ins			; 31
		inx			; 08
		iny			; 18 08
		lsla			; 48
		lslb			; 58
		lsld			; 05
		lsra			; 44
		lsrb			; 54
		lsrd			; 04
		mul			; 3D
		nega			; 40
		negb			; 50
		nop			; 01
		psha			; 36
		pshb			; 37
		pshx			; 3C
		pshy			; 18 3C
		pula			; 32
		pulb			; 33
		pulx			; 38
		puly			; 18 38
		rola			; 49
		rolb			; 59
		rora			; 46
		rorb			; 56
		rti			; 3B
		rts			; 39
		sba			; 10
		sec			; 0D
		sei			; 0F
		sev			; 0B
		stop			; CF
		swi			; 3F
		tab			; 16
		tap			; 06
		tba			; 17
		test			; 00
		tpa			; 07
		tsta			; 4D
		tstb			; 5D
		tsx			; 30
		tsy			; 18 30
		txs			; 35
		tys			; 18 35
		wai			; 3E
		xgdx			; 8F
		xgdy			; 18 8F

; test the 8 bit immediate instructions

		adca	#0xab		; 89 ii
		adcb	#0xab		; C9 ii
		adda	#0xab		; 8B ii
		addb	#0xab		; CB ii
		anda	#0xab		; 84 ii
		andb	#0xab		; C4 ii
		bita	#0xab		; 85 ii
		bitb	#0xab		; C5 ii
		cmpa	#0xab		; 81 ii
		cmpb	#0xab		; C1 ii
		eora	#0xab		; 88 ii
		eorb	#0xab		; C8 ii
		ldaa	#0xab		; 86 ii
		ldab	#0xab		; C6 ii
		oraa	#0xab		; 8A ii
		orab	#0xab		; CA ii
		sbca	#0xab		; 82 ii
		sbcb	#0xab		; C2 ii
		suba	#0xab		; 80 ii
		subb	#0xab		; C0 ii




; test the 16 bit immediate instructions

		addd	#0xabcd		; C3 jj kk
		cpd	#0xabcd		; 1A 83 jj kk
		cpx	#0xabcd		; 8C jj kk
		cpy	#0xabcd		; 18 8C jj kk
		ldd	#0xabcd		; CC jj kk
		lds	#0xabcd		; 8E jj kk
		ldx	#0xabcd		; CE jj kk
		ldy	#0xabcd		; 18 CE jj kk
		subd	#0xabcd		; 83 jj kk


; test the direct instructions

		adca	0xab		; 99 dd
		adcb	0xab		; D9 dd
		adda	0xab		; 9B dd
		addb	0xab		; DB dd
		addd	0xab		; D3 dd
		anda	0xab		; 94 dd
		andb	0xab		; D4 dd
		bita	0xab		; 95 dd
		bitb	0xab		; D5 dd
		cmpa	0xab		; 91 dd
		cmpb	0xab		; D1 dd
		cpd	0xab		; 1A 93 dd
		cpx	0xab		; 9C dd
		cpy	0xab		; 18 9C dd
		eora	0xab		; 98 dd
		eorb	0xab		; D8 dd
		jsr	0xab		; 9D dd
		ldaa	0xab		; 96 dd
		ldab	0xab		; D6 dd
		ldd	0xab		; DC dd
		lds	0xab		; 9E dd
		ldx	0xab		; DE dd
		ldy	0xab		; 18 DE dd
		oraa	0xab		; 9A dd
		orab	0xab		; DA dd
		sbca	0xab		; 92 dd
		sbcb	0xab		; D2 dd
		staa	0xab		; 97 dd
		stab	0xab		; D7 dd
		std	0xab		; DD dd
		sts	0xab		; 9F dd
		stx	0xab		; DF dd
		sty	0xab		; 18 DF dd
		suba	0xab		; 90 dd
		subb	0xab		; D0 dd
		subd	0xab		; 93 dd


; test the extended instructions

		adca	0xabcd		; B9 hh ll
		adcb	0xabcd		; F9 hh ll
		adda	0xabcd		; BB hh ll
		addb	0xabcd		; FB hh ll
		addd	0xabcd		; F3 hh ll
		anda	0xabcd		; B4 hh ll
		andb	0xabcd		; F4 hh ll
		asl	0xabcd		; 78 hh ll
		asr	0xabcd		; 77 hh ll
		bita	0xabcd		; B5 hh ll
		bitb	0xabcd		; F5 hh ll
		clr	0xabcd		; 7F hh ll
		cmpa	0xabcd		; B1 hh ll
		cmpb	0xabcd		; F1 hh ll
		com	0xabcd		; 73 hh ll
		cpd	0xabcd		; 1A B3 hh ll
		cpx	0xabcd		; BC hh ll
		cpy	0xabcd		; 18 BC hh ll
		dec	0xabcd		; 7A hh ll
		eora	0xabcd		; B8 hh ll
		eorb	0xabcd		; F8 hh ll
		inc	0xabcd		; 7C hh ll
		jmp	0xabcd		; 7E hh ll
		jsr	0xabcd		; BD hh ll
		ldaa	0xabcd		; B6 hh ll
		ldab	0xabcd		; F6 hh ll
		ldd	0xabcd		; FC hh ll
		lds	0xabcd		; BE hh ll
		ldx	0xabcd		; FE hh ll
		ldy	0xabcd		; 18 FE hh ll
		lsl	0xabcd		; 78 hh ll
		lsr	0xabcd		; 74 hh ll
		neg	0xabcd		; 70 hh ll
		oraa	0xabcd		; BA hh ll
		orab	0xabcd		; FA hh ll
		rol	0xabcd		; 79 hh ll
		ror	0xabcd		; 76 hh ll
		sbca	0xabcd		; B2 hh ll
		sbcb	0xabcd		; F2 hh ll
		staa	0xabcd		; B7 hh ll
		stab	0xabcd		; F7 hh ll
		std	0xabcd		; FD hh ll
		sts	0xabcd		; BF hh ll
		stx	0xabcd		; FF hh ll
		sty	0xabcd		; 18 FF hh ll
		suba	0xabcd		; B0 hh ll
		subb	0xabcd		; F0 hh ll
		subd	0xabcd		; B3 hh ll
		tst	0xabcd		; 7D hh ll



; test the relative instructions
label
		bcc	label2		; 24 rr (24)
		bcs	label		; 25 rr (fc)
		beq	label		; 27 rr (fa)
		bge	label		; 2C rr (f8)
		bgt	label		; 2E rr (f6)
		bhi	label		; 22 rr (f4)
		bhs	label		; 24 rr (f2)
		ble	label		; 2F rr (f0)
		blo	label		; 25 rr (ee)
		bls	label		; 23 rr (ec)
		blt	label		; 2D rr (ea)
		bmi	label		; 2B rr (e8)
		bne	label		; 26 rr (e6)
		bpl	label		; 2A rr (e4)
		bra	label2		; 20 rr (08)
		brn	label2		; 21 rr (06)
		bsr	label2		; 8D rr (04)
		bvc	label2		; 28 rr (02)
		bvs	label2		; 29 rr (00)

label2


; test indirect x instructions

		addd	x		; E3 00 test the various forms
		addd	,x		; E3 00 (these three should all be the same)
		addd	0,x		; E3 00

		adca	0xab,x		; A9 ff
		adcb	0xab,x		; E9 ff
		adda	0xab,x		; AB ff
		addb	0xab,x		; EB ff
		addd	0xab,x		; E3 ff
		anda	0xab,x		; A4 ff
		andb	0xab,x		; E4 ff
		asl	0xab,x		; 68 ff
		asr	0xab,x		; 67 ff
		bita	0xab,x		; A5 ff
		bitb	0xab,x		; E5 ff
		clr	0xab,x		; 6F ff
		cmpa	0xab,x		; A1 ff
		cmpb	0xab,x		; E1 ff
		com	0xab,x		; 63 ff
		cpd	0xab,x		; 1A A3 ff
		cpx	0xab,x		; AC ff
		cpy	0xab,x		; 1A AC ff
		dec	0xab,x		; 6A ff
		eora	0xab,x		; A8 ff
		eorb	0xab,x		; E8 ff
		inc	0xab,x		; 6C ff
		jmp	0xab,x		; 6E ff
		jsr	0xab,x		; AD ff
		ldaa	0xab,x		; A6 ff
		ldab	0xab,x		; E6 ff
		ldd	0xab,x		; EC ff
		lds	0xab,x		; AE ff
		ldx	0xab,x		; EE ff
		ldy	0xab,x		; 1A EE ff
		lsl	0xab,x		; 68 ff
		lsr	0xab,x		; 64 ff
		neg	0xab,x		; 60 ff
		oraa	0xab,x		; AA ff
		orab	0xab,x		; EA ff
		rol	0xab,x		; 69 ff
		ror	0xab,x		; 66 ff
		sbca	0xab,x		; A2 ff
		sbcb	0xab,x		; E2 ff
		staa	0xab,x		; A7 ff
		stab	0xab,x		; E7 ff
		std	0xab,x		; ED ff
		sts	0xab,x		; AF ff
		stx	0xab,x		; EF ff
		sty	0xab,x		; 1A EF ff
		suba	0xab,x		; A0 ff
		subb	0xab,x		; E0 ff
		subd	0xab,x		; A3 ff
		tst	0xab,x		; 6D ff



; test indirect y instructions

		addd	y		; 18 E3 00 test the various forms
		addd	,y		; 18 E3 00 (these three should all be the same)
		addd	0,y		; 18 E3 00

		adca	0xab,y		; 18 A9 ff
		adcb	0xab,y		; 18 E9 ff
		adda	0xab,y		; 18 AB ff
		addb	0xab,y		; 18 EB ff
		addd	0xab,y		; 18 E3 ff
		anda	0xab,y		; 18 A4 ff
		andb	0xab,y		; 18 E4 ff
		asl	0xab,y		; 18 68 ff
		asr	0xab,y		; 18 67 ff
		bita	0xab,y		; 18 A5 ff
		bitb	0xab,y		; 18 E5 ff
		clr	0xab,y		; 18 6F ff
		cmpa	0xab,y		; 18 A1 ff
		cmpb	0xab,y		; 18 E1 ff
		com	0xab,y		; 18 63 ff
		cpd	0xab,y		; CD A3 ff
		cpx	0xab,y		; CD AC ff
		cpy	0xab,y		; 18 AC ff
		dec	0xab,y		; 18 6A ff
		eora	0xab,y		; 18 A8 ff
		eorb	0xab,y		; 18 E8 ff
		inc	0xab,y		; 18 6C ff
		jmp	0xab,y		; 18 6E ff
		jsr	0xab,y		; 18 AD ff
		ldaa	0xab,y		; 18 A6 ff
		ldab	0xab,y		; 18 E6 ff
		ldd	0xab,y		; 18 EC ff
		lds	0xab,y		; 18 AE ff
		ldx	0xab,y		; CD EE ff
		ldy	0xab,y		; 18 EE ff
		lsl	0xab,y		; 18 68 ff
		lsr	0xab,y		; 18 64 ff
		neg	0xab,y		; 18 60 ff
		oraa	0xab,y		; 18 AA ff
		orab	0xab,y		; 18 EA ff
		rol	0xab,y		; 18 69 ff
		ror	0xab,y		; 18 66 ff
		sbca	0xab,y		; 18 A2 ff
		sbcb	0xab,y		; 18 E2 ff
		staa	0xab,y		; 18 A7 ff
		stab	0xab,y		; 18 E7 ff
		std	0xab,y		; 18 ED ff
		sts	0xab,y		; 18 AF ff
		stx	0xab,y		; CD EF ff
		sty	0xab,y		; 18 EF ff
		suba	0xab,y		; 18 A0 ff
		subb	0xab,y		; 18 E0 ff
		subd	0xab,y		; 18 A3 ff
		tst	0xab,y		; 18 6D ff


; test bit indexed x instructions

		bclr	0xab,x,0xcd		; 1D ff mm (1D AB CD)
		bset	0xab,x,0xcd		; 1C ff mm (1C AB CD)


; test bit indexed y instructions

		bclr	0xab,y,0xcd		; 18 1D ff mm (18 1D AB CD)
		bset	0xab,y,0xcd		; 18 1C ff mm (18 1C AB CD)


; test bit indexed x relative instructions

		brclr	0xab,x,0xcd,label3	; 1F ff mm rr (1F AB CD 0E)
		brset	0xab,x,0xcd,label3	; 1E ff mm rr (1E AB CD 0A)


; test bit indexed y relative instructions

		brclr	0xab,y,0xcd,label3	; 18 1F ff mm rr (18 1F AB CD 05)
		brset	0xab,y,0xcd,label3	; 18 1E ff mm rr (18 1E AB CD 00)


label3


; test bit direct

		bclr	0xab,0xcd		; 15 dd mm
		bset	0xab,0xcd		; 14 dd mm


; test bit direct relative

		brclr	0xab,0xcd,label3	; 13 dd mm rr (13 AB CD F6)
		brset	0xab,0xcd,label3	; 12 dd mm rr (12 AB CD F2)



; check for proper size promotion

		bita	0xff		; should fit in direct
		bita	0x100		; too big for direct, should bump up to extended


; check for range errors  -- these should generate fatal errors

		addb	#0x100		; 8-bit immediate
		addd	#0x10000	; 16 bit immediate
		jsr	0x10000		; extended
		addd	0x100,x		; indirect x
		addd	0x100,y		; indirect y
