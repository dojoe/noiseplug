#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <Windows.h>
#include <MMSystem.h>
#include "binary.h"

int notes[25] = { 134, 142, 150, 159, 169, 179, 189, 201, 213, 225, 239, 253, 268, 284, 301, 319, 338, 358, 379, 401, 425, 451, 477, 506, 536 };

int arpeggio[][4] = {
	{ 0, 3, 7, 12 },
	{ 2, 5, 7, 10 },
	{ 1, 5, 7, 10 },
	{ 1, 3, 5, 8 },
	{ 1, 3, 5, 10 },
};
int arpseq[16] = { 0, 0, 1, 1, 2, 2, 4, 3, 0, 0, 1, 1, 2, 3, 4, 4 };
//int arptiming[32] = { 4, 2, 4, 2, 4, 2, 4, 5, 1, 2, 2 }
const uint32_t arptiming = B32(00001100,00110000,11000011,11101100);

int bassbeat[8] = { 0, 0, 1, 0, 0, 1, 0, 1 };
int bassline[16] = { 12, 12, 15, 10, 12, 12, 17, 10, 12, 12, 15, 7, 8, 8, 3, 7 };

#define LEADSIZE 28
int leadmelody[LEADSIZE] = { 12, 7, 0, 12, 0, 14, 15, 0, 14, 0, 12, 0, 14, 15, 0, 14, 0, 12, 0, 14, 10, 0, 7, 5, 7, 3, 1, 0 };
int leadtiming[LEADSIZE] = {  2, 1, 1,  1, 1,  1,  2, 1,  1, 1,  1, 1,  1,  2, 1,  1, 1,  1, 1,  1,  2, 1, 2, 2, 1, 2, 3, 28};

static inline unsigned char voice_lead(unsigned long i)
{
	static uint8_t leadptr = 0xFF;
	static uint16_t lead_osc = 0;
	static uint16_t lead_flange = 0;
	static uint8_t leadtimer = 1;

	if (0 == (i & 0x3FF))
		leadtimer--;
	if (0 == leadtimer)
	{
		leadptr++;
		if (leadptr == LEADSIZE)
			leadptr = 0;
		leadtimer = leadtiming[leadptr];
	}

	uint8_t melody = leadmelody[leadptr];
	int note = notes[melody == 1 ? 0 : melody]; // TODO remove this hack by using note table
	lead_osc += note;
	lead_flange += note + (i & 1);
	return (!melody) ? 0 : (((lead_osc >> 6) & 0x7F) + ((lead_osc >> 6) & 0x3F) + ((lead_flange >> 6) & 0x3F));  // xor also sounds cool
}
                     
static inline unsigned char voice_arp(unsigned long i)
{
	static uint16_t arp_osc = 0;
	int note = notes[12 + arpeggio[arpseq[(i >> 13) & 15]][(i >> 7) & 3]];
	arp_osc += note;
	return ((arptiming & (1 << (31 - (i >> 9)))) && (arp_osc & (1 << 12))) ? 0 : 140;
	//return ((arp_osc >> 5) & 128) - 1;
}

static inline unsigned char voice_bass(unsigned long i)
{
	static uint16_t bassosc = 0, flangeosc = 0;
	int note = notes[bassline[(i >> 13) & 15]];
	if (bassbeat[(i >> 10) & 7])
		note <<= 1;
	bassosc += note;
	flangeosc += note + 1;
	unsigned char ret = ((bassosc >> 8) & 0x7F) + ((flangeosc >> 8) & 0x7F);
	return ((i >> 6) & 0xF) == 0xF ? 0 : ret;
}

void fill(char *data)
{
	static unsigned long i = 0;

	for (int j = 0; j < 4096; j++)
	{
		unsigned char sample = /*(voice_lead(i) >> 1) + */(voice_bass(i) >> 2) + (voice_arp(i) >> 2);
		data[j] = sample;
		i++;
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

	fill(bufs[i].lpData);
	bufs[i].dwFlags = WHDR_PREPARED;
	waveOutPrepareHeader(out, bufs + i, sizeof(WAVEHDR));
	waveOutWrite(out, bufs + i, sizeof(WAVEHDR));
	i ^= 1;

	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
	{
		fill(bufs[i].lpData);
		bufs[i].dwFlags = WHDR_PREPARED;
		waveOutPrepareHeader(out, bufs + i, sizeof(WAVEHDR));
		waveOutWrite(out, bufs + i, sizeof(WAVEHDR));
		i ^= 1;
		while (waveOutUnprepareHeader(out, bufs + i, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
	}
}