		nolist
; This file is a sample of an include file for the pic 16c505
; I could not use Microchip's include file here because I
; do not have the authority to distribute it.
; But.... Normally, you would grab the header file you wanted
; from the mplab distribution, and include it into your
; assembly to get these definitions....

		ifndef __16C505
		message		"This header file does not match the selected processor."
		endif

W		EQU		0x0000
w		EQU		0x0000	; make lower case versions too
F		EQU		0x0001
f		EQU		0x0001

INDF		EQU		0x0000
TMR0		EQU		0x0001
PCL		EQU		0x0002
STATUS		EQU		0x0003
FSR		EQU		0x0004
OSCCAL		EQU		0x0005
PORTB		EQU		0x0006
PORTC		EQU		0x0007

RBWUF		EQU		0x0007
PA0		EQU		0x0005
NOT_TO		EQU		0x0004
NOT_PD		EQU		0x0003
Z		EQU		0x0002
DC		EQU		0x0001
C		EQU		0x0000

NOT_RBWU	EQU		0x0007
NOT_RBPU	EQU		0x0006
T0CS		EQU		0x0005
T0SE		EQU		0x0004
PSA		EQU		0x0003
PS2		EQU		0x0002
PS1		EQU		0x0001
PS0		EQU		0x0000

CAL5		EQU		0x0007
CAL4		EQU		0x0006
CAL3		EQU		0x0005
CAL2		EQU		0x0004
CAL1		EQU		0x0003
CAL0		EQU		0x0002

		__MAXRAM	0x7F

_MCLRE_ON	EQU		0x0FFF
_MCLRE_OFF	EQU		0x0FDF
_CP_ON		EQU		0x002F
_CP_OFF		EQU		0x0FFF
_WDT_ON		EQU		0x0FFF
_WDT_OFF	EQU		0x0FF7
_LP_OSC		EQU		0x0FF8
_XT_OSC		EQU		0x0FF9
_HS_OSC		EQU		0x0FFA
_IntRC_OSC_RB4EN	EQU	0x0FFC
_IntRC_OSC_CLKOUTEN	EQU	0x0FFD
_ExtRC_OSC_RB4EN	EQU	0x0FFE
_ExtRC_OSC_CLKOUTEN	EQU	0x0FFF

		list
