DATA_START	equ	65535

	processor	"unsp2.0"
//	processor	"unsp1.1"

P_IOA_Data:	.equ	0x7000;		// Write Data into data register and read from IOA pad
P_IOA_Buffer:	.equ	0x7001;		// Write Data into buffer register and read from buffer register
P_IOA_Dir:	.equ	0x7002;		// Direction vector for IOA
P_IOA_Attrib:	.equ	0x7003;		// Attribute vector for IOA
P_IOA_Latch:	.equ	0x7004;		// Latch PortA data for key change wake-up

	r9&=[63]
	r2=r2&[63]
	
	
	r1=r1+[bp+3]
	r9=r9+[bp+3]

	r1=r1+3
	r9=r9+3

	int	off
	int	IRQ
	int	FIQ
	int	IRQ,FIQ
	fir_mov		off
	fir_mov		on
	fraction	off
	fraction	on
	irq		off
	irq		on
	secbank		off
	secbank		on
	fiq		off
	irqnest		off
	fiq		on
	irqnest		on
	r2=		exp r4
	pop R1 from [SP]
	pop R1,R2 from [SP]
	POP BP,BP FROM [SP]
	PUSH BP,BP TO [SP]
	R8 = [BP+3]
	divq	MR,R2
	TEST	R1,R2
	CMP	R1,D:[R2++]
	nop
	brk
.loop:
	jmp	.loop
	jmp	.next
.next:
	goto	$
	call	$
	call	0xA5F9
	reti
	PUSH 	BP,BP TO [SP]
	BP = 	SP & 9
	BP = 	SP + 1,carry
	PUSH 	R2,R4 TO [PC]
	PUSH 	R2,R4 TO [SP]
	R1 = 	[BP+3]
	R2 = 	[BP+4]
	[DATA_START] = R1
	D:[++R1]=R1
	[R5+63]=r5

	[0x500+23/2]=r5
	r4=-[0x500+23/2]

	R1 =-R1;
	R1 = R1 + R2 LSL 2
	R1 = R1 + R2 LSL 2,carry
	R1 = R1 + R2 LSL 2,carry
	R1 &= BP LSR 2
	R1 = R1 + R2 LSL 2,carry
	R1 = R1 LSL R3
	R1&=D:[R2++]
;	R1 = r2&r3 ASR 1
	R1 = -R1 LSR 2
	MR = R5*R5,us
	MR = [R5]*[R5],ss,4
	[P_IOA_Dir] = R1;	 
	[P_IOA_Attrib] = R1;
	[P_IOA_Data] = R1;       
	call 0xA689
	R1=0x0001
	R2 = [R1 ++];
	R1 = [R1];
	R1 += 0x9BDD;
	R1 = R1 LSL 4
	R1 = R1 LSL 2
	R4 = D:[R3++];
	R2 -= 1
	cmp	r3,0
	R1 &= ~0x2000
	R1 |= 0x2000
	R1+=0xA176
	retf
	tstb	r1,r2
	tstb	r1,2
	tstb	[r5],r4
	tstb	D:[r5],r4
	tstb	[r5],5
	tstb	D:[r5],5
	tstb	[r5],5

	tstb	[0x400],0
	setb	[0x400],0
	clrb	[0x400],0
	invb	[0x400],0
	PC-=D:[BP++]
	push r1,r4 to [sp]
	pop r1,r4 from [sp]
	push r9,r15 to [sp]
	pop r15,r9 from [sp]


	r1 += 0
	r1 = r1 + 0
	r1 = r2 + 0x2000
	r1 += [BP + 0]
	r1 = r1 + [BP + 0]
	r1 += [0]
	r1 = r1 + [0]
	r1 = r2 + [0x2000]
	r1 += r2
	r1 += [r2]
	r1 += D:[r2]
	r1 += [++r2]
	r1 += D:[++r2]
	r1 += [r2--]
	r1 += D:[r2--]
	r1 += [r2++]
	r1 += D:[r2++]

	r1 += 0,carry
	r1 = r1 + 0,carry
	r1 = r2 + 0x2000,carry
	r1 += [BP + 0],carry
	r1 = r1 + [BP + 0],carry
	r1 += [0],carry
	r1 = r1 + [0],carry
	r1 = r2 + [0x2000],carry
	r1 += r2,carry
	r1 += [r2],carry
	r1 += D:[r2],carry
	r1 += [++r2],carry
	r1 += D:[++r2],carry
	r1 += [r2--],carry
	r1 += D:[r2--],carry
	r1 += [r2++],carry
	r1 += D:[r2++],carry

	r1 -= 0
	r1 = r1 - 0
	r1 = r2 - 0x2000
	r1 -= [BP + 0]
	r1 = r1 - [BP + 0]
	r1 -= [0]
	r1 = r1 - [0]
	r1 = r2 - [0x2000]
	r1 -= r2
	r1 -= [r2]
	r1 -= D:[r2]
	r1 -= [++r2]
	r1 -= D:[++r2]
	r1 -= [r2--]
	r1 -= D:[r2--]
	r1 -= [r2++]
	r1 -= D:[r2++]

	r1 -= 0,carry
	r1 = r1 - 0,carry
	r1 = r2 - 0x2000,carry
	r1 -= [BP + 0],carry
	r1 = r1 - [BP + 0],carry
	r1 -= [0],carry
	r1 = r1 - [0],carry
	r1 = r2 - [0x2000],carry
	r1 -= r2,carry
	r1 -= [r2],carry
	r1 -= D:[r2],carry
	r1 -= [++r2],carry
	r1 -= D:[++r2],carry
	r1 -= [r2--],carry
	r1 -= D:[r2--],carry
	r1 -= [r2++],carry
	r1 -= D:[r2++],carry

	r1 &= 0
	r1 = r1 & 0
	r1 = r2 & 0x2000
	r1 &= [BP + 0]
	r1 = r1 & [BP + 0]
	r1 &= [0]
	r1 = r1 & [0]
	r1 = r2 & [0x2000]
	r1 &= r2
	r1 &= [r2]
	r1 &= D:[r2]
	r1 &= [++r2]
	r1 &= D:[++r2]
	r1 &= [r2--]
	r1 &= D:[r2--]
	r1 &= [r2++]
	r1 &= D:[r2++]


// generate code that comes as close to incrementing numbers as we can
	sp=sp+[bp+0]
	sp=sp+[bp+1]
	sp=sp+[bp+2]
	sp=sp+[bp+3]
	sp=sp+[bp+4]
	sp=sp+[bp+5]
	sp=sp+[bp+6]
	sp=sp+[bp+7]
	sp=sp+[bp+8]
	sp=sp+[bp+9]
	sp=sp+[bp+10]
	sp=sp+[bp+11]
	sp=sp+[bp+12]
	sp=sp+[bp+13]
	sp=sp+[bp+14]
	sp=sp+[bp+15]
	sp=sp+[bp+16]
	sp=sp+[bp+17]
	sp=sp+[bp+18]
	sp=sp+[bp+19]
	sp=sp+[bp+20]
	sp=sp+[bp+21]
	sp=sp+[bp+22]
	sp=sp+[bp+23]
	sp=sp+[bp+24]
	sp=sp+[bp+25]
	sp=sp+[bp+26]
	sp=sp+[bp+27]
	sp=sp+[bp+28]
	sp=sp+[bp+29]
	sp=sp+[bp+30]
	sp=sp+[bp+31]
	sp=sp+[bp+32]
	sp=sp+[bp+33]
	sp=sp+[bp+34]
	sp=sp+[bp+35]
	sp=sp+[bp+36]
	sp=sp+[bp+37]
	sp=sp+[bp+38]
	sp=sp+[bp+39]
	sp=sp+[bp+40]
	sp=sp+[bp+41]
	sp=sp+[bp+42]
	sp=sp+[bp+43]
	sp=sp+[bp+44]
	sp=sp+[bp+45]
	sp=sp+[bp+46]
	sp=sp+[bp+47]
	sp=sp+[bp+48]
	sp=sp+[bp+49]
	sp=sp+[bp+50]
	sp=sp+[bp+51]
	sp=sp+[bp+52]
	sp=sp+[bp+53]
	sp=sp+[bp+54]
	sp=sp+[bp+55]
	sp=sp+[bp+56]
	sp=sp+[bp+57]
	sp=sp+[bp+58]
	sp=sp+[bp+59]
	sp=sp+[bp+60]
	sp=sp+[bp+61]
	sp=sp+[bp+62]
	sp=sp+[bp+63]

	sp=sp+0
	sp=sp+1
	sp=sp+2
	sp=sp+3
	sp=sp+4
	sp=sp+5
	sp=sp+6
	sp=sp+7
	sp=sp+8
	sp=sp+9
	sp=sp+10
	sp=sp+11
	sp=sp+12
	sp=sp+13
	sp=sp+14
	sp=sp+15
	sp=sp+16
	sp=sp+17
	sp=sp+18
	sp=sp+19
	sp=sp+20
	sp=sp+21
	sp=sp+22
	sp=sp+23
	sp=sp+24
	sp=sp+25
	sp=sp+26
	sp=sp+27
	sp=sp+28
	sp=sp+29
	sp=sp+30
	sp=sp+31
	sp=sp+32
	sp=sp+33
	sp=sp+34
	sp=sp+35
	sp=sp+36
	sp=sp+37
	sp=sp+38
	sp=sp+39
	sp=sp+40
	sp=sp+41
	sp=sp+42
	sp=sp+43
	sp=sp+44
	sp=sp+45
	sp=sp+46
	sp=sp+47
	sp=sp+48
	sp=sp+49
	sp=sp+50
	sp=sp+51
	sp=sp+52
	sp=sp+53
	sp=sp+54
	sp=sp+55
	sp=sp+56
	sp=sp+57
	sp=sp+58
	sp=sp+59
	sp=sp+60
	sp=sp+61
	sp=sp+62
	sp=sp+63

	sp=sp+[sp]
	sp=sp+[r1]
	sp=sp+[r2]
	sp=sp+[r3]
	sp=sp+[r4]
	sp=sp+[bp]
	sp=sp+[sr]
	sp=sp+[pc]
	sp=sp+[sp--]
	sp=sp+[r1--]
	sp=sp+[r2--]
	sp=sp+[r3--]
	sp=sp+[r4--]
	sp=sp+[bp--]
	sp=sp+[sr--]
	sp=sp+[pc--]
	sp=sp+[sp++]
	sp=sp+[r1++]
	sp=sp+[r2++]
	sp=sp+[r3++]
	sp=sp+[r4++]
	sp=sp+[bp++]
	sp=sp+[sr++]
	sp=sp+[pc++]
	sp=sp+[++sp]
	sp=sp+[++r1]
	sp=sp+[++r2]
	sp=sp+[++r3]
	sp=sp+[++r4]
	sp=sp+[++bp]
	sp=sp+[++sr]
	sp=sp+[++pc]
	sp=sp+D:[sp]
	sp=sp+D:[r1]
	sp=sp+D:[r2]
	sp=sp+D:[r3]
	sp=sp+D:[r4]
	sp=sp+D:[bp]
	sp=sp+D:[sr]
	sp=sp+D:[pc]
	sp=sp+D:[sp--]
	sp=sp+D:[r1--]
	sp=sp+D:[r2--]
	sp=sp+D:[r3--]
	sp=sp+D:[r4--]
	sp=sp+D:[bp--]
	sp=sp+D:[sr--]
	sp=sp+D:[pc--]
	sp=sp+D:[sp++]
	sp=sp+D:[r1++]
	sp=sp+D:[r2++]
	sp=sp+D:[r3++]
	sp=sp+D:[r4++]
	sp=sp+D:[bp++]
	sp=sp+D:[sr++]
	sp=sp+D:[pc++]
	sp=sp+D:[++sp]
	sp=sp+D:[++r1]
	sp=sp+D:[++r2]
	sp=sp+D:[++r3]
	sp=sp+D:[++r4]
	sp=sp+D:[++bp]
	sp=sp+D:[++sr]
	sp=sp+D:[++pc]

