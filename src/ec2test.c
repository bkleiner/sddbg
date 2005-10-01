#include <stdio.h>
#include <stdlib.h>
#include "ec2drv.h"

void print_buf( char *buf, int len )
{
	while( len-- !=0 )
		printf("%02X ",(unsigned char)*buf++);
	printf("\n");
}

int main(int argc, char *argv[])
{
	char buf[1024];
	if( argc != 2 )
	{
		printf("ec2test\nSyntax:\n\tec2test /dev/ttyS0\n");
		return EXIT_FAILURE;
	}
		
	ec2_connect( argv[1] );
	
	printf("sfr dump:\n");
	ec2_read_sfr( buf, 0x7F, 0x80 );
	print_buf( buf, 0x80 );
	
	printf("data RAM dump:\n");
	ec2_read_ram( buf, 0x00, 0xFF );
	print_buf( buf, 0xFF );
	
	printf("Fill data ram with 0x55 and dump RAM dump:\n");
	memset( buf, 0x55, 0xFF);
	ec2_write_ram( buf, 0x00, 0xFF );
	memset( buf, 0x00, 0xFF);
	ec2_read_ram( buf, 0x00, 0xFF );
	print_buf( buf, 0xFF );
	
	return EXIT_SUCCESS;
}