// Test of application with a simple loop


void main()
{
	volatile int i=0, j=0;

	while(1)
	{
		i+=2;
		j+=4;
	}
}

