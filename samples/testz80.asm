variable		.set	99


			seg	code
			org	0x0000

reset:
			adc	a,0xa5		; ce a5
			adc	a,(hl)		; 8e
			adc	a,(ix+disp)	; dd 8e 12
			adc	a,(iy+disp)	; fd 8e 12
			adc	a,a		; 8f
			adc	a,b		; 88
			adc	a,c		; 89
			adc	a,d		; 8a
			adc	a,e		; 8b
			adc	a,h		; 8c
			adc	a,l		; 8d
			adc	hl,bc		; ed 4a
			adc	hl,de		; ed 5a
			adc	hl,hl		; ed 6a
			adc	hl,sp		; ed 7a

			add	a,0xa5		; c6 a5
			add	a,(hl)		; 86
			add	a,(ix+disp)	; dd 86 12
			add	a,(iy+disp)	; fd 86 12
			add	a,a		; 87
			add	a,b		; 80
			add	a,c		; 81
			add	a,d		; 82
			add	a,e		; 83
			add	a,h		; 84
			add	a,l		; 85
			add	hl,bc		; 09
			add	hl,de		; 19
			add	hl,hl		; 29
			add	hl,sp		; 39
			add	ix,bc		; dd 09
			add	ix,de		; dd 19
			add	ix,hl		; dd 29
			add	ix,sp		; dd 39
			add	iy,bc		; fd 09
			add	iy,de		; fd 19
			add	iy,hl		; fd 29
			add	iy,sp		; fd 39

			and	0xa5		; e6 a5
			and	(hl)		; a6
			and	(ix+disp)	; dd a6 12
			and	(iy+disp)	; fd a6 12
			and	a		; a7
			and	b		; a0
			and	c		; a1
			and	d		; a2
			and	e		; a3
			and	h		; a4
			and	l		; a5

			bit	0,(hl)		; cb 46
			bit	0,(ix+disp)	; dd cb 12 46
			bit	0,(iy+disp)	; fd cb 12 46
			bit	0,a		; cb 47
			bit	0,b		; cb 40
			bit	0,c		; cb 41
			bit	0,d		; cb 42
			bit	0,e		; cb 43
			bit	0,h		; cb 44
			bit	0,l		; cb 45

			bit	1,(hl)		; cb 4e
			bit	1,(ix+disp)	; dd cb 12 4e
			bit	1,(iy+disp)	; fd cb 12 4e
			bit	1,a		; cb 4f
			bit	1,b		; cb 48
			bit	1,c		; cb 49
			bit	1,d		; cb 4a
			bit	1,e		; cb 4b
			bit	1,h		; cb 4c
			bit	1,l		; cb 4d

			bit	2,(hl)		; cb 56
			bit	2,(ix+disp)	; dd cb 12 56
			bit	2,(iy+disp)	; fd cb 12 56
			bit	2,a		; cb 57
			bit	2,b		; cb 50
			bit	2,c		; cb 51
			bit	2,d		; cb 52
			bit	2,e		; cb 53
			bit	2,h		; cb 54
			bit	2,l		; cb 55

			bit	3,(hl)		; cb 5e
			bit	3,(ix+disp)	; dd cb 12 5e
			bit	3,(iy+disp)	; fd cb 12 5e
			bit	3,a		; cb 5f
			bit	3,b		; cb 58
			bit	3,c		; cb 59
			bit	3,d		; cb 5a
			bit	3,e		; cb 5b
			bit	3,h		; cb 5c
			bit	3,l		; cb 5d

			bit	4,(hl)		; cb 66
			bit	4,(ix+disp)	; dd cb 12 66
			bit	4,(iy+disp)	; fd cb 12 66
			bit	4,a		; cb 67
			bit	4,b		; cb 60
			bit	4,c		; cb 61
			bit	4,d		; cb 62
			bit	4,e		; cb 63
			bit	4,h		; cb 64
			bit	4,l		; cb 65

			bit	5,(hl)		; cb 6e
			bit	5,(ix+disp)	; dd cb 12 6e
			bit	5,(iy+disp)	; fd cb 12 6e
			bit	5,a		; cb 6f
			bit	5,b		; cb 68
			bit	5,c		; cb 69
			bit	5,d		; cb 6a
			bit	5,e		; cb 6b
			bit	5,h		; cb 6c
			bit	5,l		; cb 6d

			bit	6,(hl)		; cb 76
			bit	6,(ix+disp)	; dd cb 12 76
			bit	6,(iy+disp)	; fd cb 12 76
			bit	6,a		; cb 77
			bit	6,b		; cb 70
			bit	6,c		; cb 71
			bit	6,d		; cb 72
			bit	6,e		; cb 73
			bit	6,h		; cb 74
			bit	6,l		; cb 75

			bit	7,(hl)		; cb 7e
			bit	7,(ix+disp)	; dd cb 12 7e
			bit	7,(iy+disp)	; fd cb 12 7e
			bit	7,a		; cb 7f
			bit	7,b		; cb 78
			bit	7,c		; cb 79
			bit	7,d		; cb 7a
			bit	7,e		; cb 7b
			bit	7,h		; cb 7c
			bit	7,l		; cb 7d

			call	address		; cd 34 12
			call	c,address	; dc 34 12
			call	m,address	; fc 34 12
			call	nc,address	; d4 34 12
			call	nz,address	; c4 34 12
			call	p,address	; f4 34 12
			call	pe,address	; ec 34 12
			call	po,address	; e4 34 12
			call	z,address	; cc 34 12

			ccf			; 3f

			cp	0xa5		; fe a5
			cp	(hl)		; be
			cp	(ix+disp)	; dd be 12
			cp	(iy+disp)	; fd be 12
			cp	a		; bf
			cp	b		; b8
			cp	c		; b9
			cp	d		; ba
			cp	e		; bb
			cp	h		; bc
			cp	l		; bd

			cpd			; ed a9

			cpdr			; ed 89

			cpi			; ed a1

			cpir			; ed 81

			cpl			; 2f

			daa			; 27

			dec	(hl)		; 35
			dec	(ix+disp)	; dd 35 12
			dec	(iy+disp)	; fd 35 12
			dec	a		; 3d
			dec	b		; 05
			dec	bc		; 0b
			dec	c		; 0d
			dec	d		; 15
			dec	de		; 1b
			dec	e		; 1d
			dec	h		; 25
			dec	hl		; 2b
			dec	ix		; dd 2b
			dec	iy		; fd 2b
			dec	l		; 2d
			dec	sp		; 3b

			di			; f3
			
loop
			nop			; 00
			nop
			djnz  loop		; 10 FC

			ei			; fb

			ex	(sp),hl		; e3
			ex	(sp),ix		; dd e3
			ex	(sp),iy		; fd e3
			ex	af,af'		; 08
			ex	de,hl		; eb

			exx			; d9

			halt			; 76

			im	0		; ed 46
			im	1		; ed 56
			im	2		; ed 5e

			in	a,(disp)	; db 12
			in	a,(c)		; ed 78
			in	b,(c)		; ed 40
			in	c,(c)		; ed 48
			in	d,(c)		; ed 50
			in	e,(c)		; ed 58
			in	h,(c)		; ed 60
			in	l,(c)		; ed 68

			inc	(hl)		; 34
			inc	(ix+disp)	; dd 34 12
			inc	(iy+disp)	; fd 34 12
			inc	a		; 3c
			inc	b		; 04
			inc	bc		; 03
			inc	c		; 0c
			inc	d		; 14
			inc	de		; 13
			inc	e		; 1c
			inc	h		; 24
			inc	hl		; 23
			inc	ix		; dd 23
			inc	iy		; fd 23
			inc	l		; 2c
			inc	sp		; 33

			ind			; ed aa

			indr			; ed ba

			ini			; ed a2

			inir			; ed b2

			jp	(hl)		; e9
			jp	(ix)		; dd e9
			jp	(iy)		; fd e9
			jp	address		; c3 34 12
			jp	c,address	; da 34 12
			jp	m,address	; fa 34 12
			jp	nc,address	; d2 34 12
			jp	nz,address	; c2 34 12
			jp	p,address	; f2 34 12
			jp	pe,address	; ea 34 12
			jp	po,address	; e2 34 12
			jp	z,address	; ca 34 12

			jr	label		; 18 04
			jr	c,label		; 38 02
			jr	nc,label	; 30 00
label
			jr	z,label		; 28 fe
			jr	nz,label	; 20 fc

			ld	a,(bc)		; 0a
			ld	a,(de)		; 1a
			ld	a,(address)	; 3a 34 12
			ld	a,0xa5		; 3e a5
			ld	a,(hl)		; 7e
			ld	a,(ix+disp)	; dd 7e 12
			ld	a,(iy+disp)	; fd 7e 12
			ld	a,a		; 7f
			ld	a,b		; 78
			ld	a,c		; 79
			ld	a,d		; 7a
			ld	a,e		; 7b
			ld	a,h		; 7c
			ld	a,l		; 7d
			ld	a,i		; ed 57
			ld	a,r		; ed 5f

			ld	b,0xa5		; 06 a5
			ld	b,(hl)		; 46
			ld	b,(ix+disp)	; dd 46 12
			ld	b,(iy+disp)	; fd 46 12
			ld	b,a		; 47
			ld	b,b		; 40
			ld	b,c		; 41
			ld	b,d		; 42
			ld	b,e		; 43
			ld	b,h		; 44
			ld	b,l		; 45

			ld	c,0xa5		; 0e a5
			ld	c,(hl)		; 4e
			ld	c,(ix+disp)	; dd 4e 12
			ld	c,(iy+disp)	; fd 4e 12
			ld	c,a		; 4f
			ld	c,b		; 48
			ld	c,c		; 49
			ld	c,d		; 4a
			ld	c,e		; 4b
			ld	c,h		; 4c
			ld	c,l		; 4d

			ld	d,0xa5		; 16 a5
			ld	d,(hl)		; 56
			ld	d,(ix+disp)	; dd 56 12
			ld	d,(iy+disp)	; fd 56 12
			ld	d,a		; 57
			ld	d,b		; 50
			ld	d,c		; 51
			ld	d,d		; 52
			ld	d,e		; 53
			ld	d,h		; 54
			ld	d,l		; 55

			ld	e,0xa5		; 1e a5
			ld	e,(hl)		; 5e
			ld	e,(ix+disp)	; dd 5e 12
			ld	e,(iy+disp)	; fd 5e 12
			ld	e,a		; 5f
			ld	e,b		; 58
			ld	e,c		; 59
			ld	e,d		; 5a
			ld	e,e		; 5b
			ld	e,h		; 5c
			ld	e,l		; 5d

			ld	h,0xa5		; 26 a5
			ld	h,(hl)		; 66
			ld	h,(ix+disp)	; dd 66 12
			ld	h,(iy+disp)	; fd 66 12
			ld	h,a		; 67
			ld	h,b		; 60
			ld	h,c		; 61
			ld	h,d		; 62
			ld	h,e		; 63
			ld	h,h		; 64
			ld	h,l		; 65

			ld	l,0xa5		; 2e a5
			ld	l,(hl)		; 6e
			ld	l,(ix+disp)	; dd 6e 12
			ld	l,(iy+disp)	; fd 6e 12
			ld	l,a		; 6f
			ld	l,b		; 68
			ld	l,c		; 69
			ld	l,d		; 6a
			ld	l,e		; 6b
			ld	l,h		; 6c
			ld	l,l		; 6d
			
			ld	(hl),0xa5	; 36 a5
			ld	(ix+disp),0xa5	; dd 36 12 a5
			ld	(iy+disp),0xa5	; fd 36 12 a5
						
			ld	(bc),a		; 02
			ld	(de),a		; 12
			ld	(address),a	; 32 34 12
			ld	(hl),a		; 77
			ld	(ix+disp),a	; dd 77 12
			ld	(iy+disp),a	; fd 77 12
			ld	i,a		; ed 47
			ld	r,a		; ed 4f
						
			ld	(hl),b		; 70
			ld	(ix+disp),b	; dd 70 12
			ld	(iy+disp),b	; fd 70 12
						
			ld	(hl),c		; 71
			ld	(ix+disp),c	; dd 71 12
			ld	(iy+disp),c	; fd 71 12
						
			ld	(hl),d		; 72
			ld	(ix+disp),d	; dd 72 12
			ld	(iy+disp),d	; fd 72 12
						
			ld	(hl),e		; 73
			ld	(ix+disp),e	; dd 73 12
			ld	(iy+disp),e	; fd 73 12
						
			ld	(hl),h		; 74
			ld	(ix+disp),h	; dd 74 12
			ld	(iy+disp),h	; fd 74 12
						
			ld	(hl),l		; 75
			ld	(ix+disp),l	; dd 75 12
			ld	(iy+disp),l	; fd 75 12
			
			ld	bc,address	; 01 34 12
			ld	de,address	; 11 34 12
			ld	hl,address	; 21 34 12
			ld	sp,address	; 31 34 12
			ld	ix,address	; dd 21 34 12
			ld	iy,address	; fd 21 34 12
			
			ld	bc,(address)	; ed 4b 34 12
			ld	de,(address)	; ed 5b 34 12
			ld	hl,(address)	; 2a 34 12
			ld	sp,(address)	; ed 7b 34 12
			ld	ix,(address)	; dd 2a 34 12
			ld	iy,(address)	; fd 2a 34 12

			ld	(address),bc	; ed 43 34 12
			ld	(address),de	; ed 53 34 12
			ld	(address),hl	; 22 34 12
			ld	(address),sp	; ed 73 34 12
			ld	(address),ix	; dd 22 34 12
			ld	(address),iy	; fd 22 34 12
						
			ld	sp,hl		; f9
			ld	sp,ix		; dd f9
			ld	sp,iy		; fd f9
						
			ldd			; ed a8

			lddr			; ed b8

			ldi			; ed a0

			ldir			; ed b0

			neg			; ed 44

			nop			; 00

			or	0xa5		; f6 a5
			or	(hl)		; b6
			or	(ix+disp)	; dd b6 12
			or	(iy+disp)	; fd b6 12
			or	a		; b7
			or	b		; b0
			or	c		; b1
			or	d		; b2
			or	e		; b3
			or	h		; b4
			or	l		; b5

 			otdr			; ed bb

			otir			; ed b3

			out	(disp),a	; d3 12
			out	(c),a		; ed 79
			out	(c),b		; ed 41
			out	(c),c		; ed 49
			out	(c),d		; ed 51
			out	(c),e		; ed 59
			out	(c),h		; ed 61
			out	(c),l		; ed 69

			outd			; ed ab

			outi			; ed a3

			pop	af		; f1
			pop	bc		; c1
			pop	de		; d1
			pop	hl		; e1
			pop	ix		; dd e1
			pop	iy		; fd e1

			push	af		; f5
			push	bc		; c5
			push	de		; d5
			push	hl		; e5
			push	ix		; dd e5
			push	iy		; fd e5

			res	0,(hl)		; cb 86
			res	0,(ix+disp)	; dd cb 12 86
			res	0,(iy+disp)	; fd cb 12 86
			res	0,a		; cb 87
			res	0,b		; cb 80
			res	0,c		; cb 81
			res	0,d		; cb 82
			res	0,e		; cb 83
			res	0,h		; cb 84
			res	0,l		; cb 85

			res	1,(hl)		; cb 8e
			res	1,(ix+disp)	; dd cb 12 8e
			res	1,(iy+disp)	; fd cb 12 8e
			res	1,a		; cb 8f
			res	1,b		; cb 88
			res	1,c		; cb 89
			res	1,d		; cb 8a
			res	1,e		; cb 8b
			res	1,h		; cb 8c
			res	1,l		; cb 8d

			res	2,(hl)		; cb 96
			res	2,(ix+disp)	; dd cb 12 96
			res	2,(iy+disp)	; fd cb 12 96
			res	2,a		; cb 97
			res	2,b		; cb 90
			res	2,c		; cb 91
			res	2,d		; cb 92
			res	2,e		; cb 93
			res	2,h		; cb 94
			res	2,l		; cb 95

			res	3,(hl)		; cb 9e
			res	3,(ix+disp)	; dd cb 12 9e
			res	3,(iy+disp)	; fd cb 12 9e
			res	3,a		; cb 9f
			res	3,b		; cb 98
			res	3,c		; cb 99
			res	3,d		; cb 9a
			res	3,e		; cb 9b
			res	3,h		; cb 9c
			res	3,l		; cb 9d

			res	4,(hl)		; cb a6
			res	4,(ix+disp)	; dd cb 12 a6
			res	4,(iy+disp)	; fd cb 12 a6
			res	4,a		; cb a7
			res	4,b		; cb a0
			res	4,c		; cb a1
			res	4,d		; cb a2
			res	4,e		; cb a3
			res	4,h		; cb a4
			res	4,l		; cb a5

			res	5,(hl)		; cb ae
			res	5,(ix+disp)	; dd cb 12 ae
			res	5,(iy+disp)	; fd cb 12 ae
			res	5,a		; cb af
			res	5,b		; cb a8
			res	5,c		; cb a9
			res	5,d		; cb aa
			res	5,e		; cb ab
			res	5,h		; cb ac
			res	5,l		; cb ad

			res	6,(hl)		; cb b6
			res	6,(ix+disp)	; dd cb 12 b6
			res	6,(iy+disp)	; fd cb 12 b6
			res	6,a		; cb b7
			res	6,b		; cb b0
			res	6,c		; cb b1
			res	6,d		; cb b2
			res	6,e		; cb b3
			res	6,h		; cb b4
			res	6,l		; cb b5

			res	7,(hl)		; cb be
			res	7,(ix+disp)	; dd cb 12 be
			res	7,(iy+disp)	; fd cb 12 be
			res	7,a		; cb bf
			res	7,b		; cb b8
			res	7,c		; cb b9
			res	7,d		; cb ba
			res	7,e		; cb bb
			res	7,h		; cb bc
			res	7,l		; cb bd

			ret			; c9
			ret	c		; d8
			ret	m		; f8
			ret	nc		; d0
			ret	nz		; c0
			ret	p		; f0
			ret	pe		; e8
			ret	po		; e0
			ret	z		; c8

			reti			; ed 4d

			retn			; ed 45

			rl	(hl)		; cb 16
			rl	(ix+disp)	; dd cb 12 16
			rl	(iy+disp)	; fd cb 12 16
			rl	a		; cb 17
			rl	b		; cb 10
			rl	c		; cb 11
			rl	d		; cb 12
			rl	e		; cb 13
			rl	h		; cb 14
			rl	l		; cb 15

			rla			; 17

			rlc	(hl)		; cb 06
			rlc	(ix+disp)	; dd cb 12 06
			rlc	(iy+disp)	; fd cb 12 06
			rlc	a		; cb 07
			rlc	b		; cb 00
			rlc	c		; cb 01
			rlc	d		; cb 02
			rlc	e		; cb 03
			rlc	h		; cb 04
			rlc	l		; cb 05

			rlca			; 07

			rld			; ed 6f

			rr	(hl)		; cb 1e
			rr	(ix+disp)	; dd cb 12 1e
			rr	(iy+disp)	; fd cb 12 1e
			rr	a		; cb 1f
			rr	b		; cb 18
			rr	c		; cb 19
			rr	d		; cb 1a
			rr	e		; cb 1b
			rr	h		; cb 1c
			rr	l		; cb 1d

			rra			; 1f

			rrc	(hl)		; cb 0e
			rrc	(ix+disp)	; dd cb 12 0e
			rrc	(iy+disp)	; fd cb 12 0e
			rrc	a		; cb 0f
			rrc	b		; cb 08
			rrc	c		; cb 09
			rrc	d		; cb 0a
			rrc	e		; cb 0b
			rrc	h		; cb 0c
			rrc	l		; cb 0d

			rrca			; 0f

			rrd			; ed 67

			rst	0x00		; c7
			rst	0x08		; cf
			rst	0x10		; d7
			rst	0x18		; df
			rst	0x20		; e7
			rst	0x28		; ef
			rst	0x30		; f7
			rst	0x38		; ff

			scf			; 37

			set	0,(hl)		; cb c6
			set	0,(ix+disp)	; dd cb 12 c6
			set	0,(iy+disp)	; fd cb 12 c6
			set	0,a		; cb c7
			set	0,b		; cb c0
			set	0,c		; cb c1
			set	0,d		; cb c2
			set	0,e		; cb c3
			set	0,h		; cb c4
			set	0,l		; cb c5

			set	1,(hl)		; cb ce
			set	1,(ix+disp)	; dd cb 12 ce
			set	1,(iy+disp)	; fd cb 12 ce
			set	1,a		; cb cf
			set	1,b		; cb c8
			set	1,c		; cb c9
			set	1,d		; cb ca
			set	1,e		; cb cb
			set	1,h		; cb cc
			set	1,l		; cb cd

			set	2,(hl)		; cb d6
			set	2,(ix+disp)	; dd cb 12 d6
			set	2,(iy+disp)	; fd cb 12 d6
			set	2,a		; cb d7
			set	2,b		; cb d0
			set	2,c		; cb d1
			set	2,d		; cb d2
			set	2,e		; cb d3
			set	2,h		; cb d4
			set	2,l		; cb d5

			set	3,(hl)		; cb de
			set	3,(ix+disp)	; dd cb 12 de
			set	3,(iy+disp)	; fd cb 12 de
			set	3,a		; cb df
			set	3,b		; cb d8
			set	3,c		; cb d9
			set	3,d		; cb da
			set	3,e		; cb db
			set	3,h		; cb dc
			set	3,l		; cb dd

			set	4,(hl)		; cb e6
			set	4,(ix+disp)	; dd cb 12 e6
			set	4,(iy+disp)	; fd cb 12 e6
			set	4,a		; cb e7
			set	4,b		; cb e0
			set	4,c		; cb e1
			set	4,d		; cb e2
			set	4,e		; cb e3
			set	4,h		; cb e4
			set	4,l		; cb e5

			set	5,(hl)		; cb ee
			set	5,(ix+disp)	; dd cb 12 ee
			set	5,(iy+disp)	; fd cb 12 ee
			set	5,a		; cb ef
			set	5,b		; cb e8
			set	5,c		; cb e9
			set	5,d		; cb ea
			set	5,e		; cb eb
			set	5,h		; cb ec
			set	5,l		; cb ed

			set	6,(hl)		; cb f6
			set	6,(ix+disp)	; dd cb 12 f6
			set	6,(iy+disp)	; fd cb 12 f6
			set	6,a		; cb f7
			set	6,b		; cb f0
			set	6,c		; cb f1
			set	6,d		; cb f2
			set	6,e		; cb f3
			set	6,h		; cb f4
			set	6,l		; cb f5

			set	7,(hl)		; cb fe
			set	7,(ix+disp)	; dd cb 12 fe
			set	7,(iy+disp)	; fd cb 12 fe
			set	7,a		; cb ff
			set	7,b		; cb f8
			set	7,c		; cb f9
			set	7,d		; cb fa
			set	7,e		; cb fb
			set	7,h		; cb fc
			set	7,l		; cb fd

			sbc	a,0xa5		; de a5
			sbc	a,(hl)		; 9e
			sbc	a,(ix+disp)	; dd 9e 12
			sbc	a,(iy+disp)	; fd 9e 12
			sbc	a,a		; 9f
			sbc	a,b		; 98
			sbc	a,c		; 99
			sbc	a,d		; 9a
			sbc	a,e		; 9b
			sbc	a,h		; 9c
			sbc	a,l		; 9d
			sbc	hl,bc		; ed 42
			sbc	hl,de		; ed 52
			sbc	hl,hl		; ed 62
			sbc	hl,sp		; ed 72

			sla	(hl)		; cb 26
			sla	(ix+disp)	; dd cb 12 26
			sla	(iy+disp)	; fd cb 12 26
			sla	a		; cb 27
			sla	b		; cb 20
			sla	c		; cb 21
			sla	d		; cb 22
			sla	e		; cb 23
			sla	h		; cb 24
			sla	l		; cb 25

			sra	(hl)		; cb 2e
			sra	(ix+disp)	; dd cb 12 2e
			sra	(iy+disp)	; fd cb 12 2e
			sra	a		; cb 2f
			sra	b		; cb 28
			sra	c		; cb 29
			sra	d		; cb 2a
			sra	e		; cb 2b
			sra	h		; cb 2c
			sra	l		; cb 2d

			srl	(hl)		; cb 3e
			srl	(ix+disp)	; dd cb 12 3e
			srl	(iy+disp)	; fd cb 12 3e
			srl	a		; cb 3f
			srl	b		; cb 38
			srl	c		; cb 39
			srl	d		; cb 3a
			srl	e		; cb 3b
			srl	h		; cb 3c
			srl	l		; cb 3d

			sub	0xa5		; d6 a5
			sub	(hl)		; 96
			sub	(ix+disp)	; dd 96 12
			sub	(iy+disp)	; fd 96 12
			sub	a		; 97
			sub	b		; 90
			sub	c		; 91
			sub	d		; 92
			sub	e		; 93
			sub	h		; 94
			sub	l		; 95

			xor	0xa5		; ee a5
			xor	(hl)		; ae
			xor	(ix+disp)	; dd ae 12
			xor	(iy+disp)	; fd ae 12
			xor	a		; af
			xor	b		; a8
			xor	c		; a9
			xor	d		; aa
			xor	e		; ab
			xor	h		; ac
			xor	l		; ad


; test for no displacement and various whitespace
;
			xor	( ix )		; dd ae 00
			xor	( ix + disp )	; dd ae 12

disp			equ	this+that	; make it work a little to resolve this
this			equ	0x10
that			equ	0x02

address			equ	0x1234

; test pseudo ops
;

pseudo0:
			ds	4
pseudo1:
			ds.b	4
pseudo2:
			ds.w	4

			db	"abcdefgh",0
			db	0,-1

			dw	0,-1
			dw	0x1234



			if	0		; set to 1 to test error trapping
; test range
			db	0x100		; should be too big
			xor	( ix+0x101 )	; displacement too big
			rst	0x01		; out of range
			inc	af		; can't use this register on this opcode
			push	sp		; can't use this register on this opcode
			pop	sp		; can't use this register on this opcode
			jp	(ix+1)		; doesn't allow displacement
			in	a,(0x103)	; out of range
			xor	(ix + disp  4)	; stray operand
			adc	a,(hl a)	; another stray operand

			endif