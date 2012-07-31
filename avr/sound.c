



int main(r)
{

unsigned short rnd = 13373;
while(1)
{
	unsigned char f1 = (rnd&(3<<13))>>13;
	unsigned char f2 = (rnd&(3<<10))>>10;
	rnd <<=1;
	rnd |= f1^f2;
	putchar(rnd);	

}
}
