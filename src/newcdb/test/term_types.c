// Test of application with a simple loop


int main()
{
	int z=0;
	volatile char		a=z++;
	volatile unsigned char	b=z++;
	volatile short		c=z++;
	volatile unsigned short	d=z++;
	volatile int		e=z++;
	volatile unsigned int	f=z++;
	volatile float		g=z++;
	volatile sbit		h=z++;
	
	while(1)
	{
	}
}

