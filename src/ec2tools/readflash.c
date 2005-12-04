/** ec2readflash utility
  *
  *   Copyright (C) 2005 by Ricky White
  *   rickyw@neatstuff.co.nz
  *
  *   This program is free software; you can redistribute it and/or modify
  *   it under the terms of the GNU General Public License as published by
  *   the Free Software Foundation; either version 2 of the License, or
  *   (at your option) any later version.
  *
  *   This program is distributed in the hope that it will be useful,
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *   GNU General Public License for more details.
  *
  *   You should have received a copy of the GNU General Public License
  *   along with this program; if not, write to the
  *   Free Software Foundation, Inc.,
  *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include "ec2drv.h"
#include "ihex.h"

void help()
{
	printf("ec2readflash\n"
		   "syntax:\n"
		   "\tec2readflash --port=/dev/ttyS0 --start=0x0000 --len=100 --bin file.bin\n"
		   "\twhere /dev/ttyS0 is your desired serial port"
		   "and file.bin is the file to write the data to\n"
		   "\n"
		   "Options:\n"
		   "\t--hex                 Save dump as intel hex file\n"
		   "\t--bin                 Save dump as binary file\n"
		   "\t--console             Print hex dump to console\n"
		   "\t--port <serial dev>   Specify serial port to connect to EC2 on\n"
		   "\t--start <addr>        Address to start reading from\n"
		   "\t--len <length>        Number of bytes to read\n"
		   "\t--scratch             Cause read to occur from scratchpad area of flash\n"
		   "\t--help                Display this help\n"
		   "\n");
}

#define MAXPORTLEN 1024
int main(int argc, char *argv[])
{
	char buf[0x10000];
	char port[MAXPORTLEN]="";
	uint16_t start=0, length=0x10000;
	static int hex, bin, console, help_flag, scratch_flag, out;
	static struct option long_options[] = 
	{
		{"hex", no_argument, &hex, 1},
		{"bin", no_argument, &bin, 1},
		{"console", no_argument, &console, 1},
		{"help", no_argument, &help_flag, 'h'},
		{"scratch", no_argument, &scratch_flag, 'z'},
		{"port", required_argument, 0, 'p'},
		{"start", required_argument, 0, 's'},
		{"len", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	int c, i;
	
	while(1)
	{
	 	c = getopt_long (argc, argv, "", long_options, &option_index);
		if( c==-1)
			break;
		switch(c)
		{
			case 0:		// set a flag, nothing to do
				break;
			case 'p':	// port
				strncpy( port, optarg, MAXPORTLEN );
				break;
			case 's':	// start address
				start = strtoul( optarg, 0, 0);
				break;
			case 'l':	// length of data to read
				length = strtoul( optarg, 0, 0);
				break;
			default:
				printf("unexpected option\n");
				break;
		}
	};

	if( help_flag || strlen(port)==0 )
	{
		help();
		return  help_flag ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	if( bin && hex  )
	{
		printf("ERROR :- you can either use binary or hex but not both!\n");
		return EXIT_FAILURE;
	}

	ec2_connect( port );
	
	if( scratch_flag )
		ec2_read_flash_scratchpad( buf, start, length );
	else
		ec2_read_flash( buf, start, length );
	
	if( hex )
	{
		if( argc-optind == 1 )
			ihex_save_file( argv[optind], buf, 0, length );
	}
	
	if(bin)
	{
		if( (argc-optind)==1)
		{
			out = open( argv[optind], O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR );
			if( out!=-1 )
			{
				i = write( out, buf, length );
				printf("%i bytes written\n",i);
				close(out);
			}
		}
	}
	
	if(console)
	{
		int j;
		for( i=0; i<length; i+=16)
		{
			printf( "0x%04X  ", (unsigned char)(start+i) );
			// print hex
			for( j=0; j<16; j++ )
			{
				if( (i+j)<length )
					printf( "%02X ", (unsigned char)buf[i+j] );
				else
					printf("   ");
			}
			putchar(' ');
			// print ASCII
			for( j=0; j<16; j++ )
			{
				if( (i+j)<length )
				{	
					if( isprint( buf[i+j] ) )
						putchar( buf[i+j] );
					else
						putchar('.');
				}
			}
			putchar('\n');
		}
	}
	
	return EXIT_SUCCESS;
}
