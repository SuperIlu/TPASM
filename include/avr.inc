; Register definitions

TWBR		equ		$00			; TWI Bit Rate Register
TWSR		equ		$01			; TWI Status Register
TWAR		equ		$02			; TWI Address Register
TWDR		equ		$03			; TWI Data Register
ADCL		equ		$04			; ADC Data Register Low
ADCH		equ		$05			; ADC Data Register High
ADCSR		equ		$06			; ADC Control and Status Register
ADCSRA		equ		$06			; ADC Control and Status Register A
ADMUX		equ		$07			; ADC Multiplexer Select Register
ACSR		equ		$08			; Analog Comparator Control and Status Register
UBRR		equ		$09			; USART Baud Rate Register
UBRRL		equ		$09			; USART Baud Rate Register (low part)
UCR		equ		$0A			; For compatibility with S8535
UCSRB		equ		$0A			; USART Control/Status Register B
USR		equ		$0B			; For compatibility with S8535
UCSRA		equ		$0B			; USART Control/Status Register A
UDR		equ		$0C			; UART I/O Data Register
SPCR		equ		$0D			; SPI Control Register
SPSR		equ		$0E			; SPI Status Register
SPDR		equ		$0F			; SPI I/O Data Register
PIND		equ		$10			; Input Pins, Port D
DDRD		equ		$11			; Data Direction Register, Port D
PORTD		equ		$12			; Data Register, Port D
PINC		equ		$13			; Input Pins, Port C
DDRC		equ		$14			; Data Direction Register, Port C
PORTC		equ		$15			; Data Register, Port C
PINB		equ		$16			; Input Pins, Port B
DDRB		equ		$17			; Data Direction Register, Port B
PORTB		equ		$18			; Data Register, Port B
PINA		equ		$19			; Input Pins, Port A
DDRA		equ		$1A			; Data Direction Register, Port A
PORTA		equ		$1B			; Data Register, Port A
EECR		equ		$1C			; EEPROM Control Register
EEDR		equ		$1D			; EEPROM Data Register
EEARL		equ		$1E			; EEPROM Address Register Low Byte
EEARH		equ		$1F			; EEPROM Address Register High Byte
UCSRC		equ		$20			; USART Control/Status Register C (NOTE: ugly access method)
UBRRH		equ		$20			; USART Baud Rate Register (high part) (NOTE: ugly access method)
WDTCR		equ		$21			; Watchdog Timer Control Register
ASSR		equ		$22			; Asynchronous Mode Status Register
OCR2		equ		$23			; Timer/Counter2 Output Compare Register
TCNT2		equ		$24			; Timer/Counter2 (8-bit)
TCCR2		equ		$25			; Timer/Counter2 Control Register
ICR1L		equ		$26			; T/C 1 Input Capture Register Low Byte
ICR1H		equ		$27			; T/C 1 Input Capture Register High Byte
OCR1BL		equ		$28			; Timer/Counter1 Output Compare Register B Low Byte
OCR1BH		equ		$29			; Timer/Counter1 Output Compare Register B High Byte
OCR1AL		equ		$2A			; Timer/Counter1 Output Compare Register A Low Byte
OCR1AH		equ		$2B			; Timer/Counter1 Output Compare Register A High Byte
TCNT1L		equ		$2C			; Timer/Counter1 Low Byte
TCNT1H		equ		$2D			; Timer/Counter1 High Byte
TCCR1B		equ		$2E			; Timer/Counter1 Control Register B
TCCR1A		equ		$2F			; Timer/Counter1 Control Register A
SFIOR		equ		$30			; Special Function IO Register
OSCCAL		equ		$31			; Oscillator Calibration Register 
TCNT0		equ		$32			; Timer/Counter0 (8-bit)
TCCR0		equ		$33			; Timer/Counter0 Control Register
MCUSR		equ		$34			; MCU general Status Register
MCUCR		equ		$35			; MCU general Control Register
TWCR		equ		$36			; TWI Control Register
SPMCR		equ		$37			; Store Program Memory Control Register
TIFR		equ		$38			; Timer/Counter Interrupt Flag register
TIMSK		equ		$39			; Timer/Counter Interrupt MaSK register
GIFR		equ		$3A			; General Interrupt Flag Register
GIMSK		equ		$3B			; General Interrupt Control Register
GICR		equ		$3B			; new name for GIMSK
SPL		equ		$3D			; Stack Pointer Low
SPH		equ		$3E			; Stack Pointer High
SREG		equ		$3F			; Status REGister

;***** Bit Definitions

; GICR (GIMSK)
INT1		equ		7
INT0		equ		6
INT2		equ		5
IVSEL		equ		1			; interrupt vector select
IVCE		equ		0			; interrupt vector change enable

; GIFR
INTF1		equ		7
INTF0		equ		6

; TIMSK
OCIE2		equ		7
TOIE2		equ		6
TICIE1		equ		5
OCIE1A		equ		4
OCIE1B		equ		3
TOIE1		equ		2
TOIE0		equ		0

; TIFR
OCF2		equ		7
TOV2		equ		6
ICF1		equ		5
OCF1A		equ		4
OCF1B		equ		3
TOV1		equ		2
TOV0		equ		0

; SPMCR
SPMIE		equ		7
ASB		equ		6
ASRE		equ		4
BLBSET		equ		3
PGWRT		equ		2
PGERS		equ		1
SPMEN		equ		0

; TWCR
TWINT		equ		7
TWEA		equ		6
TWSTA		equ		5
TWSTO		equ		4
TWWC		equ		3
TWEN		equ		2
TWI_TST		equ		1			; Present in core test mode only. Write Only.
TWIE		equ		0

; MCUCR
SE		equ		6
SM1		equ		5
SM0		equ		4
ISC11		equ		3
ISC10		equ		2
ISC01		equ		1
ISC00		equ		0

; MCUSR
WDRF		equ		3
BORF		equ		2
EXTRF		equ		1
PORF		equ		0

; TCCR0
CS02		equ		2
CS01		equ		1
CS00		equ		0

; SFIOR
ACME		equ		3
PUD		equ		2
PSR2		equ		1
PSR10		equ		0

; TCCR1A
COM1A1		equ		7
COM1A0		equ		6
COM1B1		equ		5
COM1B0		equ		4
FOC1A		equ		3
FOC1B		equ		2
WGM11		equ		1
PWM11		equ		1
WGM10		equ		0
PWM10		equ		0

; TCCR1B
ICNC1		equ		7
ICES1		equ		6
WGM13		equ		4
WGM12		equ		3
CTC1		equ		3
CS12		equ		2
CS11		equ		1
CS10		equ		0

; TCCR2
FOC2		equ		7
PWM2		equ		6
COM21		equ		5
COM20		equ		4
CTC2		equ		3
CS22		equ		2
CS21		equ		1
CS20		equ		0

; ASSR
AS2		equ		3
TCN2UB		equ		2
OCR2UB		equ		1
TCR2UB		equ		0

; WDTCR
WDTOE		equ		4
WDE		equ		3
WDP2		equ		2
WDP1		equ		1
WDP0		equ		0

; EECR
EERIE		equ		3
EEMWE		equ		2
EEWE		equ		1
EERE		equ		0

; PORTA
PA7		equ		7
PA6		equ		6
PA5		equ		5
PA4		equ		4
PA3		equ		3
PA2		equ		2
PA1		equ		1
PA0		equ		0

; DDRA
DDA7		equ		7
DDA6		equ		6
DDA5		equ		5
DDA4		equ		4
DDA3		equ		3
DDA2		equ		2
DDA1		equ		1
DDA0		equ		0

; PINA
PINA7		equ		7
PINA6		equ		6
PINA5		equ		5
PINA4		equ		4
PINA3		equ		3
PINA2		equ		2
PINA1		equ		1
PINA0		equ		0

; PORTB
PB7		equ		7
PB6		equ		6
PB5		equ		5
PB4		equ		4
PB3		equ		3
PB2		equ		2
PB1		equ		1
PB0		equ		0

; DDRB
DDB7		equ		7
DDB6		equ		6
DDB5		equ		5
DDB4		equ		4
DDB3		equ		3
DDB2		equ		2
DDB1		equ		1
DDB0		equ		0

; PINB
PINB7		equ		7
PINB6		equ		6
PINB5		equ		5
PINB4		equ		4
PINB3		equ		3
PINB2		equ		2
PINB1		equ		1
PINB0		equ		0

; PORTC
PC7		equ		7
PC6		equ		6
PC5		equ		5
PC4		equ		4
PC3		equ		3
PC2		equ		2
PC1		equ		1
PC0		equ		0

; DDRC
DDC7		equ		7
DDC6		equ		6
DDC5		equ		5
DDC4		equ		4
DDC3		equ		3
DDC2		equ		2
DDC1		equ		1
DDC0		equ		0

; PINC
PINC7		equ		7
PINC6		equ		6
PINC5		equ		5
PINC4		equ		4
PINC3		equ		3
PINC2		equ		2
PINC1		equ		1
PINC0		equ		0

; PORTD
PD7		equ		7
PD6		equ		6
PD5		equ		5
PD4		equ		4
PD3		equ		3
PD2		equ		2
PD1		equ		1
PD0		equ		0

; DDRD
DDD7		equ		7
DDD6		equ		6
DDD5		equ		5
DDD4		equ		4
DDD3		equ		3
DDD2		equ		2
DDD1		equ		1
DDD0		equ		0

; PIND
PIND7		equ		7
PIND6		equ		6
PIND5		equ		5
PIND4		equ		4
PIND3		equ		3
PIND2		equ		2
PIND1		equ		1
PIND0		equ		0

; SPSR
SPIF		equ		7
WCOL		equ		6
SPI2X		equ		0

; SPCR
SPIE		equ		7
SPE		equ		6
DORD		equ		5
MSTR		equ		4
CPOL		equ		3
CPHA		equ		2
SPR1		equ		1
SPR0		equ		0

; UCSRA
RXC		equ		7
TXC		equ		6
UDRE		equ		5
FE		equ		4
OR		equ		3
U2X		equ		1
MPCM		equ		0

; UCSRB
RXCIE		equ		7
TXCIE		equ		6
UDRIE		equ		5
RXEN		equ		4
TXEN		equ		3
UCSZ2		equ		2
CHR9		equ		2
RXB8		equ		1
TXB8		equ		0

; UCSRC
URSEL		equ		7
UMSEL		equ		6
UPM1		equ		5
UPM0		equ		4
USBS		equ		3
UCSZ1		equ		2
UCSZ0		equ		1
UCPL		equ		0

; ACSR
ACD		equ		7
ACBG		equ		6
ACO		equ		5
ACI		equ		4
ACIE		equ		3
ACIC		equ		2
ACIS1		equ		1
ACIS0		equ		0

; ADMUX
REFS1		equ		7
REFS0		equ		6
ADLAR		equ		5
MUX4		equ		4
MUX3		equ		3
MUX2		equ		2
MUX1		equ		1
MUX0		equ		0

; ADCSR
ADEN		equ		7
ADSC		equ		6
ADFR		equ		5
ADIF		equ		4
ADIE		equ		3
ADPS2		equ		2
ADPS1		equ		1
ADPS0		equ		0

; TWAR
TWGCE		equ		0

XL		alias		r26
XH		alias		r27
YL		alias		r28
YH		alias		r29
ZL		alias		r30
ZH		alias		r31
