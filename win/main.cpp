#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <Windows.h>
#include <MMSystem.h>
#include "binary.h"

static inline uint8_t THREEQUARTERS(uint8_t x)
{
	return (x >> 2) + (x >> 1);
}

const uint16_t notes[] = {
	-1, 134, 159, 179, 201, 213, 239, 268, 301, 319, 358, 401, 425, 451, 477, 536, 601, 637, 715
};
#define arpnotes (notes + 5)

const uint16_t arpeggio[][2] = {
	{ 0x24, 0x6A },
	{ 0x46, 0x9C },
	{ 0x13, 0x59 },
	{ 0x02, 0x47 },
	{ 0x24, 0x59 },
	{ 0x24, 0x58 },
	{ 0x57, 0xAD },
	{ 0x35, 0x9B }
};

#define ARPSIZE 76

#if 0
uint8_t arpseq1[4][8] = {
	{ 0, 0, 1, 2, 0, 0, 6, 2, },
	{ 0, 0, 1, 2, 0, 0, 1, 7, },
	{ 0, 0, 1, 2, 0, 0, 1, 2, },
	{ 3, 3, 2, 2, 0, 0, 4, 5, },
};
#else
const uint8_t arpseq1[4][4] = {
	{ 0x00, 0x12, 0x00, 0x62 },
	{ 0x00, 0x12, 0x00, 0x17 },
	{ 0x00, 0x12, 0x00, 0x12 },
	{ 0x33, 0x22, 0x00, 0x45 },
};
#endif
const int arpseq2[] = { 0, 1, 0, 1, 0, 1, 0, 2, 3, 3 };
const uint32_t arptiming = B32(00001100,00110000,11111011,00001100);

const int bassbeat[8] = { 0, 0, 1, 0, 0, 1, 0, 1 };
const int bassline[] = {
	7, 7, 9, 6, 7, 7, 10, 6, 7, 7, 9, 4, 5, 5, 2, 4,
	5, 5, 6, 6, 7, 7, 3, 3, 5, 5, 6, 6
};

const uint8_t leadtimes[] = {
	1, 2, 3, 4, 5, 6, 28, 14
};
const uint8_t leaddata[] = {
	0x67, 0x24, 0x20, 0x27, 0x20, 0x28, 0x89, 0x0, 0x28, 0x20, 0x27, 0x20, 0x28, 0x89, 0x0, 0x28,
	0x20, 0x27, 0x20, 0x28,	0x86, 0x0, 0x44, 0x0, 0x63, 0x24, 0x62, 0xA1, 0xE0, 0xE0, 0xE0, 0xE0,
	0x20, 0x29, 0x20, 0x2A, 0x8B, 0x0, 0x4E, 0x0, 0x6F, 0x30, 0x71, 0xAF, 0xE0, 0xE0, 0xE0, 0xE0,
	0x20, 0x29, 0x20, 0x2A, 0x8B, 0x0, 0x4E, 0x0, 0x6F, 0x30, 0x6F, 0xAC, 0xE0, 0xE0, 0xE0, 0xE0,
	0x65, 0x22, 0x20, 0x65, 0x26, 0x87, 0x0, 0x68, 0x69, 0x2B, 0xAA, 0xC0, 0x67, 0x24, 0x20, 0x67,
	0x28, 0x89, 0x0, 0x68, 0x69, 0x2B, 0xAA, 0xC0, 0x65, 0x22, 0x20, 0x65, 0x26, 0xA7, 0x28, 0x20,
	0x69, 0x2B, 0xAA, 0x29, 0x20, 0x68, 0x29, 0xAA, 0x2B, 0x20, 0x69, 0x28, 0x69, 0x67,
};
const uint8_t leadseq[] = { 0, 1, 0, 2, 0, 1, 0, 3, 4, 5, 6 };
#define LEADSIZE 174

struct leadvoice_s {
	uint8_t ptr, timer;
	uint16_t osc;
} leads[3] = {
	{ LEADSIZE, 0, 0 },
	{ LEADSIZE, 0, 1601 },
	{ LEADSIZE, 0, 3571 },
};

uint8_t boosts;

static unsigned char voice_lead(unsigned long i, int voice_nr)
{
#define leadptr leads[voice_nr].ptr
#define lead_osc leads[voice_nr].osc
#define leadtimer leads[voice_nr].timer

	if (leadptr == LEADSIZE)
	{
		if (i == (0x40000 + 0x400 * voice_nr))
		{
			leadptr = -1;
			leadtimer = 1;
		}
		else
			return 0;
	}

	if (0 == (i & 0x0FF))
		boosts &= ~(1 << voice_nr);
	if (0 == (i & 0x1FF))
		leadtimer--;
	if (0 == leadtimer)
	{
		leadptr++;
		leadtimer = leadtimes[leaddata[(leadseq[leadptr >> 4] << 4) | (leadptr & 0xF)] >> 5];
		boosts |= 1 << voice_nr;
	}


	uint8_t melody = leaddata[(leadseq[leadptr >> 4] << 4) | (leadptr & 0xF)] & 0x1F;
	lead_osc += notes[melody];
	uint8_t sample = ((lead_osc >> 6) & 0x7F) + ((lead_osc >> 6) & 0x3F);
	return (0 == melody) ? 0 : ((boosts & (1 << voice_nr)) ? sample : THREEQUARTERS(sample));
}
                     
static inline unsigned char voice_arp(unsigned long i)
{
	static uint16_t arp_osc = 0;
	uint8_t arpptr = i >> 13;
	uint8_t arpptr2 = arpseq1[arpseq2[arpptr >> 3]][(arpptr >> 1) & 3];
	if (!(arpptr & 1))
		arpptr2 >>= 4;
	arpptr = arpeggio[arpptr2 & 0xF][(i >> 8) & 1];
	if (!(i & 0x80))
		arpptr >>= 4;

	int note = arpnotes[arpptr & 0xF];
	arp_osc += note;
	return ((arptiming & (1 << (31 - (i >> 9)))) && (arp_osc & (1 << 12)) && ((i >> 13) > 15)) ? 0 : 140;
}

static inline unsigned char voice_bass(unsigned long i)
{
	static uint16_t bassosc = 0, flangeosc = 0;
	uint8_t bassptr = (i >> 13) & 0xF;
	if (i >> 19)
		bassptr |= 0x10;
	int note = notes[bassline[bassptr]];
	if (bassbeat[(i >> 10) & 7])
		note <<= 1;
	bassosc += note;
	flangeosc += note + 1;
	unsigned char ret = ((bassosc >> 8) & 0x7F) + ((flangeosc >> 8) & 0x7F);
	return ((i >> 6) & 0xF) == 0xF ? 0 : ret;
}

static inline uint8_t next_sample()
{
	static unsigned long i = 0;//x40000;
	uint8_t ret = (voice_lead(i, 0) >> 1) + THREEQUARTERS(voice_lead(i, 1) >> 2) + (voice_lead(i, 2) >> 3) + (voice_bass(i) >> 2) + (voice_arp(i) >> 2);
	i++;
	if ((i >> 13) == ARPSIZE)
		i = 16 << 13;
	return ret;
}

void fill(uint8_t *data)
{
	static uint8_t max = 0;

	for (int j = 0; j < 4096; j++)
	{
		data[j] = next_sample();
		if (data[j] > max)
		{
			max = data[j];
			printf("%x\n", max);
		}
	}
}

static const WAVEFORMATEX fmt = {
	WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0
};

int main(int argc, char *argv[])
{
	HWAVEOUT out;
	HRESULT rc = waveOutOpen(&out, WAVE_MAPPER, &fmt, NULL, NULL, CALLBACK_NULL);
	if (rc != MMSYSERR_NOERROR)
	{
		printf("error %d on open\n");
		exit(1);
	}

	WAVEHDR bufs[2] = {
		{ (char *)malloc(4096), 4096 },
		{ (char *)malloc(4096), 4096 },
	};
	int i = 0;

	fill((uint8_t *)bufs[i].lpData);
	bufs[i].dwFlags = WHDR_PREPARED;
	waveOutPrepareHeader(out, bufs + i, sizeof(WAVEHDR));
	waveOutWrite(out, bufs + i, sizeof(WAVEHDR));
	i ^= 1;

	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
	{
		fill((uint8_t *)bufs[i].lpData);
		bufs[i].dwFlags = WHDR_PREPARED;
		waveOutPrepareHeader(out, bufs + i, sizeof(WAVEHDR));
		waveOutWrite(out, bufs + i, sizeof(WAVEHDR));
		i ^= 1;
		while (waveOutUnprepareHeader(out, bufs + i, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
	}
}