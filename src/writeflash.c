// copy the supplied binary file into the devices flash at 0x0000
// (C) Ricky White 2005
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ec2drv.h"

void help()
{
	printf("ec2writeflash\n"
		   "syntax:\n"
		   "\tec2writeflash /dev/ttyS0 file.bin\n"
		   "\twhere /dev/ttyS0 is your desired serial port"
		   "and file.bin is the file to program.\n"
		   "The current contents of the microprocessor will be erased without question!\n\n");
}

int main(int argc, char *argv[])
{
	int in;
	char buf[0x10000];
	int cnt;
	
	if( argc!=3 )
	{
		help();
		return EXIT_FAILURE;
	}
	if( !ec2_connect( argv[1] ) )
	{
		printf("Error: Coulden't connect to EC2 on '%s'\n",argv[1]);
		return EXIT_FAILURE;
	}
	printf("Erasing Flash\n");
	ec2_flash_erase();
	
	in = open( argv[2], O_RDONLY, 0);
	if( in )
	{
		cnt = read( in, buf, 0x10000 );
		printf("Writing %i bytes\n",cnt);
		if( ec2_write_flash(buf,0x0000,cnt) )
		{
			printf("%i bytes written successfully\n",cnt);
		}
		else
		{
			printf("Error: flash write failed\n");
			close(in);
			return EXIT_FAILURE;
		}
	}
	else
	{
		printf("Error: coulden't open %s\n",argv[2]);
		close(in);
		return EXIT_FAILURE;
	}
	close(in);
	return EXIT_SUCCESS;
}

