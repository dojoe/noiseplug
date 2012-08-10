.global __vectors

SREG = 0x3F
CCP = 0x3C
SMCR = 0x3A
CLKPSR = 0x36
TCCR0A = 0x2E
TCCR0B = 0x2D
TCCR0C = 0x2C
TIMSK0 = 0x2B
TIFR0 = 0x2A
OCR0AL = 0x26
PUEB = 0x03
PORTB = 0x02
DDRB = 0x01
PINB = 0x00

.section .bss

	.comm int_ctr, 1
	.comm i, 3
	.comm lead1, 4
	.comm lead2, 4
	.comm lead3, 4
	.comm bassosc, 2
	.comm bassflange, 2
	.comm arposc, 2
	; leaves 10 bytes for stack

.section .text

__vectors:
	clr r16
	ldi r17, 0xD8
	out PUEB, r16
	rjmp main_cont
	
	.type	__vector_4, @function
__vector_4:
	push r16
	push r24
	in r16, SREG
	lds r24,int_ctr
	subi r24,lo8(-(1))
	andi r24,lo8(3)
	sts int_ctr,r24
	out SREG,r16
	pop r24
	pop r16
	reti
	.size	__vector_4, .-__vector_4
	
main_cont:
	out CCP, r17
	out CLKPSR, r16
	ldi r17, 5
	out DDRB, r17
	ldi r17, 0x81
	out TCCR0A, r17
	ldi r17, 0x09
	out TCCR0B, r17
	ldi r17, 1
	out SMCR, r17
	out TIMSK0, r17
	sei
	out TIFR0, r17

mainloop:
	sleep
	lds r17, int_ctr
	tst r17
	brne mainloop
	
	sbi PORTB, 2
	
	lds r17, i
	lds r18, i+1
	lds r19, i+2
	
	subi r19, -1
	sbci r18, 0
	sbci r17, 0
	
	sts i, r17
	sts i+1, r18
	sts i+2, r19
	
	out OCR0AL, r18
	
	cbi PORTB, 2
	rjmp mainloop
