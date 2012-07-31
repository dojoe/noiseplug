#include <inttypes.h>

#ifndef PROGMEM
#define PROGMEM /* nix */
#endif

#define arpeggio_DELAY 128

#define BIT(x,n) (((x)&(1<<(n)))>>(n))
#define SO(x) (sizeof((x))/sizeof(*(x)))

#define A1 0
#define As1 1
#define B1 2
#define C1 3
#define Cs1 4
#define D1 5
#define Ds1 6
#define E1 7
#define F1 8
#define Fs1 9
#define G1 10
#define Gs1 11
#define A2 12
#define Aa2 13
#define B2 14
#define C2 15
#define Cs2 16
#define D2 17
#define Ds2 18
#define E2 19
#define F2 20
#define Fs2 21
#define G2 22
#define Gs2 23
#define HOLD 24

static const PROGMEM uint8_t sin[] = {0, 49, 97, 141, 180, 212, 235, 250, 254, 250, 235, 212, 180, 141, 97, 49 };
static const PROGMEM uint8_t octave_delay[] = { 36, 34, 32, 31, 29, 27, 26, 24, 23, 22, 20, 19, 18, 17, 16, 15, 14, 14, 13, 12, 11, 11, 10, 10 };
static const PROGMEM struct { uint8_t a; uint8_t b; } synth[] = { { 7, 6}, {7, 5}, {7,5}, {6,5} };
static const PROGMEM uint8_t arpeggiobase[] =	{ 3, 4, 4, 5 };
static const PROGMEM uint8_t bassdrum[] =	{ 1, 1, 1, 1 };
static const PROGMEM uint8_t snare[] = 	{ 1, 1, 1, 1 };
static const PROGMEM uint8_t melody[] =
{
	D2, 0, D2, 0, 0, 0, 0, 0, D2, 0,
	A1, 0, B1, 0, D2, 0, D2, 0, D2, 0
};

static inline uint8_t next_note()
{

	static uint8_t idx=0;

	const uint8_t v = melody[idx++];
	
	if(idx == SO(melody))
		idx = 0;

	return v;
}

static inline uint8_t next_rnd()
{
	static unsigned short rnd = 13373;
	const uint8_t f1 = (rnd&(3<<13))>>13;
	const uint8_t f2 = (rnd&(3<<10))>>10;
	rnd <<=1;
	rnd |= f1^f2;

	return sin[((uint8_t)rnd)%SO(sin)];
}

static inline uint8_t next_sin(const uint8_t step)
{
	static uint8_t sinoff=SO(sin)-1;
	sinoff += step;
	sinoff &= SO(sin) - 1;
	return sin[sinoff];
}


static inline uint8_t next_sample()
{
static uint16_t t=0;

static uint8_t t8=0;
static uint8_t barevent=0;
static uint8_t bars=0;

static uint8_t pc = 0;
static uint8_t arpeggiocnt = 1;

static uint8_t next_sin_time = 0;
static uint8_t current_tone = 0;
static uint8_t current_tone_base = 12;

static unsigned short snaredelay;
static unsigned short bassdelay;

static uint8_t synth1;
static uint8_t synth2;

if(t%1024 == 0)
{
	// implicit rollover of t roughly every 2 seconds
	if(t==0) t8 = 0;
	else ++t8;

	// determine which note we're playing
	barevent |= 8;
	if(t8%2 == 0) barevent |= 4;
	if(t8%4 == 0) barevent |= 2;
	if(t8%8 == 0) barevent |= 1;
}
else barevent = 0;

if(barevent & 8)
{
	current_tone = current_tone_base = next_note();
}

// increment bar counter
if(barevent & 1) ++bars;

// increment pattern counter
if(bars % 8 == 0)
{
	++pc;
	pc %= SO(arpeggiobase);
}

// increment arpeggio
if(t % arpeggio_DELAY == 0)
{
	// arpeggio
	++arpeggiocnt;
	arpeggiocnt &= 3;
	current_tone = current_tone_base + 4 * arpeggiocnt;
}

// render synth

synth1 = ((t&(t>>(synth[pc].a))) | (t&(t>>(synth[pc].b))) << 1);

if(t == next_sin_time)
{
	next_sin_time += octave_delay[current_tone];
	synth2 = next_sin(5);
}

// mix two synth lines
unsigned int mix = (synth1>>1) | (synth2>>1);

// load or decrement snare delay
if(barevent & 8 && snare[pc])	snaredelay = 800;
else if(snaredelay > 0)		--snaredelay;

// add snare drum
if(snaredelay>0)	mix = (next_rnd() & 7) << 3;


// load or decrement bass delay
if(barevent & 4 && bassdrum[pc])	bassdelay = 800;
else if(bassdelay > 0)			--bassdelay;

// add bass drum
if(bassdelay>0) mix = next_sin(1);

++t;
// here comes the noize!
return mix;
}

#ifndef NOISEPLUG_BIATCH
int main()
{
	while(1) putchar(next_sample());
}
#endif
