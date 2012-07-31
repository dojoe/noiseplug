/*
 * main.c
 *
 *  Created on: 30.10.2011
 *      Author: dojoe
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

//register uint8_t int_ctr asm("r18");
uint8_t int_ctr;

#if defined(__AVR_ATmega32U4__)
ISR(TIMER0_OVF_vect)
#elif defined(__AVR_ATtiny9__)
ISR(TIM0_OVF_vect)
#else
#error "warghs"
#endif
{
	int_ctr = (int_ctr + 1) & 3;
}

#define NOISEPLUG_BIATCH
#include "sound1.c"

#if 0
static inline uint8_t next_sample(const uint32_t t)
{
//	return t*(42&t>>10);
//	return t*((42&t>>10)%14);
//	return (t*5&t>>7)|(t*3&t>>10);
//	return t*9&t>>4|t*5&t>>7|t*3&t/1024;
//	return (t*9&t>>4|t*5&t>>7|t*3&t/1024)-1;
//	return t>>4|t&((t>>5)/(t>>7-(t>>15)&-t>>7-(t>>15)));
//	return (int)(t/1e7*t*t+t)%127|t>>4|t>>5|t%127+(t>>16)|t;
	uint8_t a = t>>6&1?t>>5:-t>>4;
	uint8_t b = t>>6^t>>8|t>>12|t&63;

	return random();
//	return ((t>>6^t>>8|t>>12|t&63)&127)+((t>>6&1?t>>6:-t>>5)&63);
//	return t << 3;
}
#endif

int main()
{
#if defined(__AVR_ATmega32U4__)
#define SAMPLE_L OCR0A
	clock_prescale_set(clock_div_2);

	DDRB = 1 << PB7 | 1 << PB2; // PB7 == OC0A
	TCNT0 = 0;
	OCR0A = 0;
	TCCR0A = 3 << WGM00 | 2 << COM0A0;  // Fast PWM mode, positive polarity
	TCCR0B = 1 << CS00;  // Select main clock, no prescaling
	TIMSK0 = 1 << TOIE0; // enable overflow interrupt
	TIFR0 = 1 << TOV0;   // make sure it happens

	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();
#elif defined(__AVR_ATtiny9__)
#define SAMPLE_L OCR0AL

	CCP = 0xD8;
	CLKMSR = 0;
	CCP = 0xD8;
	CLKPSR = 0;
	PUEB = 0;
	DDRB = (1 << PB0) | (1 << PB2);
	TCNT0 = 0;
	OCR0A = 0;
	TCCR0A = (1 << WGM00) | (2 << COM0A0);
	TCCR0B = (1 << WGM02) | (1 << CS00);
	TCCR0C = 0;
	TIMSK0 = 1 << TOIE0;

	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();
	TIFR0 = 1 << TOV0;
#else
#error "not supported"
#endif

	uint8_t sample = 0;

#ifdef __AVR_ATmega32U4__
	DDRE = 1 << PE6;
	uint16_t t = 0;
#endif

	while (1) {
		sleep_mode();
		if (int_ctr == 0) {
			//PORTB = 1 << PB2;
			SAMPLE_L = sample;
			sample = next_sample();
			//PORTB = 0;

#ifdef __AVR_ATmega32U4__
			PORTE = (t & 4096) ? 1 << PE6 : 0;
			t++;
#endif
		}
	}
}
