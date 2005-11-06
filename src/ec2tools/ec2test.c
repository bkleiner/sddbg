#include <stdio.h>
#include <stdlib.h>
#include "ec2drv.h"


// foward declarations
int test_data_ram();
int test_xdata_ram();
int test_flash();
int test_sfr();
void print_buf( char *buf, int len );

int main(int argc, char *argv[])
{
	char buf[4096];
	int i;
	if( argc != 2 )
	{
		printf("ec2test\nSyntax:\n\tec2test /dev/ttyS0\n");
		return EXIT_FAILURE;
	}
		
	ec2_connect( argv[1] );
	
#if 0
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
#endif
	
	printf("RAM access test %s\n",test_data_ram()==0 ? "PASS":"FAIL");
	printf("XRAM access test %s\n",test_xdata_ram()==0 ? "PASS":"FAIL");
	printf("SFR access test %s\n",test_sfr()==0 ? "PASS":"FAIL");
	printf("FLASH access test %s\n",test_flash()==0 ? "PASS":"FAIL");
	return EXIT_SUCCESS;
}


// returns number of failed tests
int test_data_ram()
{
	int fail = 0;
	char tbuf[256], rbuf[256];
	int i;

	printf("Testing dataram access\n");
	
	// write / read 0x00
	printf("\twrite / read 0x00\n");
	memset( tbuf, 0, sizeof(tbuf) );	
	ec2_write_ram( tbuf, 0, sizeof(tbuf) );
	memset( rbuf, 0xff, sizeof(rbuf) );
	ec2_read_ram( rbuf, 0, sizeof(rbuf) );
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0x00 FAILED\n");
		print_buf(tbuf,sizeof(tbuf));
		print_buf(rbuf,sizeof(rbuf));
		fail++;
	}
	
	printf("\twrite / read 0xff\n");
	memset( tbuf, 0xff, sizeof(tbuf) );	
	ec2_write_ram( tbuf, 0, sizeof(tbuf) );
	memset( rbuf, 0x00, sizeof(rbuf) );
	ec2_read_ram( rbuf, 0, sizeof(rbuf) );
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0xff FAILED\n");
		fail++;
	}
	
	printf("\twrite / read 0-ff sequence\n");
	for( i=0; i<=0xff; i++ )
		tbuf[i] = i;
	print_buf(tbuf,sizeof(tbuf));
	ec2_write_ram( tbuf, 0, sizeof(tbuf) );
	
	memset( rbuf, 0x00, sizeof(rbuf) );
	ec2_read_ram( rbuf, 0, sizeof(rbuf) );
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0-ff sequence FAILED\n");
		fail++;
		print_buf(rbuf,sizeof(rbuf));
	}
	
	
	return fail;	
}


int test_xdata_ram()
{
	int fail = 0;
	char tbuf[4096], rbuf[4096];
	int i;

	printf("Testing xdata ram access\n");
	printf("\tTesting RW 0x00\n");
	memset( tbuf, 0x00, sizeof(tbuf) );
	memset( rbuf, 0xff, sizeof(tbuf) );
	ec2_write_xdata(tbuf,0,sizeof(tbuf));
	ec2_read_xdata(rbuf,0,sizeof(rbuf));
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0x00 FAILED\n");
		fail++;
		print_buf(rbuf,sizeof(rbuf));
	}
	
	printf("\tTesting RW 0xff\n");
	memset( tbuf, 0xff, sizeof(tbuf) );
	memset( rbuf, 0x00, sizeof(tbuf) );
	ec2_write_xdata(tbuf,0,sizeof(tbuf));
	ec2_read_xdata(rbuf,0,sizeof(rbuf));
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0xff FAILED\n");
		fail++;
		print_buf(rbuf,sizeof(rbuf));
	}

	printf("\tTesting RW 0x00-0xff sequence\n");
	for(i=0; i<=4096; i++)
		tbuf[i] = i;
	memset( rbuf, 0x00, sizeof(tbuf) );
	ec2_write_xdata(tbuf,0,sizeof(tbuf));
	ec2_read_xdata(rbuf,0,sizeof(rbuf));
	if( memcmp( rbuf, tbuf, sizeof(rbuf) )!=0 )
	{
		printf("\tRW 0x00-0xff sequence FAILED\n");
		fail++;
		print_buf(rbuf,sizeof(rbuf));
	}

	printf("\tTesting RW mid page\n");
	// first blank out the page
	memset( tbuf, 0x0, 0x100 );
	ec2_write_xdata( tbuf, 0x0100, 0x100 );	// second page
	// now write in middle
	memset( tbuf, 0x55, 5  );
	ec2_write_xdata( tbuf, 0x010A, 5 );		// 10 bytes in in second page
	// make tbuf what we expect
	memset( tbuf, 0x0, 0x100 );
	memset( tbuf+0x0A, 0x55, 5  );
	ec2_read_xdata( rbuf, 0x100, 0x100 );		// read entire page
	if( memcmp( rbuf, tbuf, 0x100 )!=0 )
	{
		printf("\tRW mid page write FAILED\n");
		fail++;
		print_buf(rbuf,0x100);
	}
	printf("\tTesting RW random data\n");
	for( i=0; i<sizeof(tbuf); i++)
		tbuf[i] = rand() & 0x00FF;
	ec2_write_xdata( tbuf, 0, sizeof(tbuf) );
	memset( rbuf, 0x00, sizeof(tbuf) );
	ec2_read_xdata( rbuf, 0, sizeof(tbuf) );
	if( memcmp( rbuf, tbuf, sizeof(tbuf) )!=0 )
	{
		printf("\tRW random data FAILED\n");
		fail++;
		print_buf(rbuf,sizeof(tbuf));
	}
	return fail;
}

int test_flash()
{

}

int test_sfr()
{
	int i,j;
	char t,r;

	// @FIXME: the following test will write to PCON which will shutdown the processor, breaking all further tests
	for(i=0x80; i<=0xFF; i++)
	{
		t = 0xFF;
		if(i!=0x87&&i!=0xb1&&i!=0xb2)
		{
			ec2_write_sfr( &t, i, 1 );
			ec2_read_sfr( &r, i, 1 );
			printf("sfr = 0x%02x, %s\n",(unsigned int)i, (r==t) ? "PASS" : "FAIL" );
		}
		t = 0x00;
		if(i!=0x87&&i!=0xb1&&i!=0xb2)
		{
			ec2_write_sfr( &t, i, 1 );
			ec2_read_sfr( &r, i, 1 );
			printf("sfr = 0x%02x, %s\n",(unsigned int)i, (r==t) ? "PASS" : "FAIL" );
		}
	}
}


void print_buf( char *buf, int len )
{
	while( len-- !=0 )
		printf("%02X ",(unsigned char)*buf++);
	printf("\n");
}
