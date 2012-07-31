
#define SO(x) (sizeof((x))/sizeof(*(x)))

static unsigned char sin[] = {0, 49, 97, 141, 180, 212, 235, 250, 254, 250, 235, 212, 180, 141, 97, 49 };

unsigned char getRand()
{
	static unsigned short rnd = 13373;
	unsigned char f1 = (rnd&(3<<13))>>13;
	unsigned char f2 = (rnd&(3<<10))>>10;
	rnd <<=1;
	rnd |= f1^f2;
	return (unsigned char)(rnd&0x0f);
}

static inline unsigned char rnd_adv()
{
	return sin[getRand()%SO(sin)];
}

static inline unsigned char sin_adv()
{
	static unsigned char sinoff=SO(sin)-1;
	++sinoff;
	sinoff %= SO(sin);
	return sin[sinoff];
}

int main()
{
	while(1)
	{
		putchar(sin_adv());
	}
}
