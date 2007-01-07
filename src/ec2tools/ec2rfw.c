/**	Author Kolja Waschk
*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ec2drv.h"

EC2DRV ec2obj;

#define BUFLEN 256

int main(int argc, char *argv[])
{
	int i,j,out;
	unsigned char buf[BUFLEN];
	
	if(argc!=3)
	{
		printf(	"ec2rb syntax:\n"
				"\tec2rb (/dev/ttyS0|USB) outfile\n\n" );
		return -1;
	}

	out=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY, 0644);
	if(out<0) { perror(argv[2]); return -1; };

	ec2_connect_fw_update( &ec2obj, argv[1] );
	ec2obj.debug=TRUE;
	for(i=0;i<0x3E00;i+=BUFLEN)
    {
		printf("Reading 256 bytes at 0x%04X\n", i);
		for(j=0;j<BUFLEN;j++)
		{
			buf[j] = boot_read_byte(&ec2obj, i+j);
		}
		write(out, buf, BUFLEN);
    };
	close(out);

	ec2_disconnect( &ec2obj );

	return EXIT_SUCCESS;
}


void progress( uint8_t percent )
{
	printf("%i\n",percent);
}


 	  	 
