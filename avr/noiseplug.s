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
	.comm boost, 1
	; leaves 9 bytes for stack

.section .text

__vectors:
NULL:
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
	clr r31
	ldi r30, 0x40
	
clearsram:
	st Z+, r16
	sbrs r30, 5
	rjmp clearsram

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
	clr r16
	lds r17, int_ctr
	tst r17
	brne mainloop
	
	sbi PORTB, 2
	
	lds r17, i
	lds r18, i+1
	lds r19, i+2
	
	subi r19, 0xff
	sbci r18, 0xff
	sbci r17, 0xff
	
	; if ((i >> 13) == 76) i = 16 << 13;
	mov r20, r18
	rol r20
	mov r20, r17
	rol r20
	subi r20, 0x13
	brne norestart
	
	ldi r17, 2
	clr r18
	
norestart:
	sts i, r17
	sts i+1, r18
	sts i+2, r19

; ==== BASS ====
	; bassptr(r20) = (i >> 13) & 0xF
	mov r20, r17
	ror r20
	mov r20, r18
	ror r20
	swap r20
	andi r20, 0xF
	
	; if (i >> 19) & 1: bassptr |= 0x10
	sbrc r17, 3
	ori r20, 0x10
	
	; note = notes[bassline[bassptr]]
	ldi r31, hi8(bassline+0x4000)
	ldi r30, lo8(bassline)
	add r30, r20
	ld r20, Z
	ldi r30, lo8(notes)
	add r30, r20
	ld r21, Z+
	ld r20, Z
	
	; if (bassbeat[(i >> 10) & 7]): note <<= 1
	mov r22, r18
	lsr r22
	lsr r22
	andi r22, 7
	ldi r30, lo8(bassbeat)
	add r30, r22
	ld r22, Z
	tst r22				; assuming this resets C
	breq nobassbeat
	rol r21
	rol r20
	
nobassbeat:
	; bassosc += note, ret = (bassosc >> 8) & 0x7F
	lds r22, bassosc
	lds r23, bassosc + 1
	add r23, r21
	adc r22, r20
	sts bassosc, r22
	sts bassosc + 1, r23
	mov r24, r22
	andi r24, 0x7F
	
	; bassflange += note + 1, ret += (bassflange >> 8) & 0x7F
	lds r22, bassflange
	lds r23, bassflange + 1
	inc r21
	add r23, r21
	adc r22, r20
	sts bassflange, r22
	sts bassflange + 1, r23
	andi r22, 0x7F
	add r24, r22
	
	; if ((i >> 6) & 0xF) == 0xF: sample += (bass >> 2)
	lsr r24
	lsr r24
	mov r20, r18
	andi r20, 3
	subi r20, 3
	brne addbass
	mov r20, r19
	andi r20, 0xC0
	subi r20, 0xC0
	breq noaddbass

addbass:	
	add r16, r24
	
noaddbass:	

; ==== ARPEGGIO ====
	; arpptr(r30) = arpseq1[arpseq2[i >> 16]][(i >> 14) & 3]
	mov r30, r17
	subi r30, lo8(NULL-arpseq2)
	ld r30, Z
	lsl r30
	lsl r30
	mov r20, r18
	swap r20
	lsr r20
	lsr r20
	andi r20, 3
	or r30, r20
	subi r30, lo8(NULL-arpseq1)
	ld r30, Z
	
	; if (!(i & (1 << 13))): arpptr >>= 14
	sbrs r18, 5
	swap r30
	
	; arpptr = arpeggio[arpptr & 0xF][(i >> 8) & 1]
	andi r30, 0xF
	lsl r30
	mov r20, r18
	andi r20, 1
	or r30, r20
	subi r30, lo8(NULL-arpeggio)
	ld r30, Z
	
	; if (!(i & 0x80)): arpptr >>= 14
	sbrs r19, 7
	swap r30
	
	; note = arpnotes[arpptr & 0xF]
	andi r30, 0xF
	lsl r30
	subi r30, lo8(NULL-arpnotes)
	ld r21, Z+
	ld r20, Z
	
	; arp_osc += note
	lds r22, arposc
	lds r23, arposc + 1
	add r23, r21
	adc r22, r20	
	sts arposc, r22		; keep r22 for later!
	sts arposc + 1, r23
	
	; if (!(i >> 17)): break arp
	mov r20, r17
	lsr r20
	breq noarp
	
	; r20 = arptiming[(i >> 12) & 3]
	mov r30, r18
	swap r30
	andi r30, 3
	subi r30, lo8(NULL-arptiming)
	ld r20, Z
	
	; if (!((r20 << ((i >> 9) & 7)) & 0x80)): break arp
	mov r21, r18
	lsr r21
	andi r21, 7
	breq arptiming_noshift

arptiming_shift:
	lsl r20
	subi r21, 1
	brne arptiming_shift
	
arptiming_noshift:
	sbrs r20, 7
	rjmp noarp
	
	; if (arp_osc & (1 << 12)): sample += 35;
	sbrc r22, 4
	subi r16, -35
	
noarp:
	out OCR0AL, r16
	
	cbi PORTB, 2
	rjmp mainloop

	.org 0x300
	
notes:
	.word 	-1, 134, 159, 179, 201
arpnotes:
	.word	213, 239, 268, 301, 319, 358, 401, 425, 451, 477, 536, 601, 637, 715
	
bassline:
	.byte	14, 14, 18, 12, 14, 14, 20, 12, 14, 14, 18, 8, 10, 10, 4, 8
	.byte	10, 10, 12, 12, 14, 14, 6, 6, 10, 10, 12, 12

bassbeat:
	.byte	0, 0, 1, 0, 0, 1, 0, 1

arpseq1:
	.byte	0x00, 0x12, 0x00, 0x62
	.byte	0x00, 0x12, 0x00, 0x17
	.byte	0x00, 0x12, 0x00, 0x12
	.byte	0x33, 0x22, 0x00, 0x45

arpseq2:
	.byte	0, 1, 0, 1, 0, 1, 0, 2, 3, 3

arptiming:
	.byte	0x0C, 0x30, 0xFB, 0x0C

arpeggio:
	.byte	0x24, 0x6A
	.byte	0x46, 0x9C
	.byte	0x13, 0x59
	.byte	0x02, 0x47
	.byte	0x24, 0x59
	.byte	0x24, 0x58
	.byte	0x57, 0xAD
	.byte	0x35, 0x9B