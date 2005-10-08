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
	int i;
	if( argc != 2 )
	{
		printf("ec2test\nSyntax:\n\tec2test /dev/ttyS0\n");
		return EXIT_FAILURE;
	}
		
	ec2_connect( argv[1] );
	

	printf("add breakpint 0x%x\n",ec2_addBreakpoint( 0x000A ));
	printf("add breakpint 0x%x\n",ec2_addBreakpoint( 0x0008 ));
	//ec2_target_go();
	
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	ec2_target_run_bp();
	printf("PC = 0x%04X\n",ec2_read_pc());
	
	return EXIT_SUCCESS;
}
