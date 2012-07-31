#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <Windows.h>
#include <MMSystem.h>

int notes[16] = { 122, 115, 109, 103, 97, 92, 86, 82, 77, 73, 69, 65, 61, 58, 54, 51 };
int arpeggio[][4] = {
	{ 0, 3, 7, 12 },
	{ 2, 5, 7, 10 },
	{ 1, 5, 7, 10 },
	{ 1, 3, 5, 8 },
	{ 1, 3, 5, 10 },
};
int arpseq[8] = { 0, 0, 1, 1, 2, 2, 4, 3 };

static inline unsigned char voice_arp(unsigned long i)
{
	return (((i << 1) / (notes[arpeggio[arpseq[(i >> 13) & 7]][(i >> 8) & 3]] >> 2)) & 1) << 7;
}

void fill(char *data)
{
	static unsigned long i = 0;

	for (int j = 0; j < 4096; j++)
	{
		data[j] = voice_arp(i);
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