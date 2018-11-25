// 8051 include

ACC		EQU		0E0H
B		EQU		0F0H
DPH		EQU		083H
DPL		EQU		082H
IE		EQU		0A8H
IP		EQU		0B8H
P0		EQU		080H
P1		EQU		090H
P2		EQU		0A0H
P3		EQU		0B0H
PCON		EQU		087H
PSW		EQU		0D0H
SBUF		EQU		099H
SCON		EQU		098H
SP		EQU		081H
TCON		EQU		088H
TH0		EQU		08CH
TH1		EQU		08DH
TL0		EQU		08AH
TL1		EQU		08BH
TMOD		EQU		089H

Reg0		EQU		000H
Reg1		EQU		001H
Reg2		EQU		002H
Reg3		EQU		003H
Reg4		EQU		004H
Reg5		EQU		005H
Reg6		EQU		006H
Reg7		EQU		007H

// status flags
CY		EQU		0D7H
AC		EQU		0D6H
F0		EQU		0D5H
RS1		EQU		0D4H
RS0		EQU		0D3H
OV		EQU		0D2H
P		EQU		0D0H

// interrupt enable bits
EA		EQU		0AFH
ES		EQU		0ACH
ET1		EQU		0ABH
EX1		EQU		0AAH
ET0		EQU		0A9H
EX0		EQU		0A8H

// interrupt priority bits
PS		EQU		0BCH
PT1		EQU		0BBH
PX1		EQU		0BAH
PT0		EQU		0B9H
PX0		EQU		0B8H

// timer control bits
TF1		EQU		08FH
TR1		EQU		08EH
TF0		EQU		08DH
TR0		EQU		08CH
IE1		EQU		08BH
IT1		EQU		08AH
IE0		EQU		089H
IT0		EQU		088H

// serial port control bits
SM0		EQU		09FH
SM1		EQU		09EH
SM2		EQU		09DH
REN		EQU		09CH
TB8		EQU		09BH
RB8		EQU		09AH
TI		EQU		099H
RI		EQU		098H

// port 3 bits
T1		EQU		0B5H
T0		EQU		0B4H
INT1		EQU		0B3H
INT0		EQU		0B2H
TXD		EQU		0B1H
RXD		EQU		0B0H

// handy macros

// Use this to call a subroutine with the shortest instruction based
// on where the subroutine is relative to the call
call		macro		dest
		if		((dest)&0F800H)==(($+2)&0F800H)
		acall		dest
		else
		lcall		dest
		endif
		endm

// Use this to jump to a location with the shortest instruction based
// on where the location is relative to the jump
jump		macro		dest
		if		(((dest)-($+2))>=-128)&&(((dest)-($+2))<128)	// relative jump
		sjmp		dest
		else
		if		((dest)&0F800H)==(($+2)&0F800H)
		ajmp		dest
		else
		ljmp		dest
		endif
		endif
		endm
