/***************************************************************************
 *   Copyright (C) 2005 by Ricky White   *
 *   rickyw@neatstuff.co.nz   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <stdint.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>

#include <iostream.h>
using namespace std;

#include "ec2.h"



EC2::BLOCK init[] = {
	{ "\x04",1,"\x0D",1 },
	{ "\x1A\x06\x00\x00\x00\x00\x00\x00",8,"\x0D",1 },
	{ "\x0B\x02\x02\x00",4,"\x0D",1 },
	{ "\x14\x02\x10\x00",4,"\x04",1 },
	{ "\x16\x02\x01\x20",4,"\x01\x00",2 },
	
	{ "\x14\x02\x10\x00",4,"\x04",2 },
	{ "\x16\x02\x81\x20",4,"\x01\x00",2 },
	{ "\x14\x02\x10\x00",4,"\x04",1 },
	{ "\x16\x02\x81\x30",4,"\x01\x00",2 },
	{ "\x15\x02\x08\x00",4,"\x04",1 },
	
	{ "\x16\x01\xE0",3,"\x00",1 },
	{ "\x0B\x02\x01\x00",4,"\x0D",1 },
	{ "\x13\x00",2,"\x01",1 },
	{ "\x03\x02\x00\x00",4,"\x0D",1 },
	{ "\x0A\x00",2,"\x21\x01\x03\x00\x00\x12",6 },
	
	{ "\x10\x00",2,"\x07",1 },
	{ "\x0C\x02\x80\x12",4,"\x00\x07\x1C",3 },
	{ "\x02\x02\xB6\x01",4,"\x80",1 },
	{ "\x02\x02\xB2\x01",4,"\x14",1 },
	{ "\x03\x02\xB2\x04",4,"\x0D",1 },
	
	{ "\x0B\x02\x04\x00",4,"\x0D",1 },
	{ "\x0D\x05\x85\x08\x01\x00\x00",7,"\x0D",1 },
	{ "\x0D\x05\x84\x10\xFE\xFD\x00",7,"\x0D",1 },
	{ "\x0D\x05\x82\x08\x01\x00\x00",7,"\x0D",1 },
	{ "\x0D\x05\x84\x10\xFE\xFD\x00",7,"\x0D",1 },

	{ "\x11\x02\x01\x00",4,"\xFF",1 },
	{ "\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1 },
	{ "\x0B\x02\x01\x00",4,"\x0D",1 },
	{ "\x03\x02\xB6\x80",4,"\x0D",1 },
	{ "\x03\x02\xB2\x14",4,"\x0D",1 },
	
	{ "\x02\x02\xB6\x01",4,"\x80",1 },
	{ "\x02\x02\xB2\x01",4,"\x14",1 },
	{ "\x03\x02\xB2\x04",4,"\x0D",1 },
	{ "\x0B\x02\x04\x00",4,"\x0D",1 },
	{ "\x0D\x05\x85\x08\x01\x00\x00",7,"\x0D",1 },
	
	{ "\x0D\x05\x84\x10\xFF\xFD\x00",7,"\x0D",1 },
	{ "\x0D\x05\x82\x08\x01\x00\x00",7,"\x0D",1 },
	{ "\x0D\x05\x84\x10\xFF\xFD\x00",7,"\x0D",1 },
	{ "\x11\x02\x01\x00",4,"\xFF",1 },
	{ "\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1 },
	
	{ "\x0B\x02\x01\x00",4,"\x0D",1 },
	{ "\x03\x02\xB6\x80",4,"\x0D",1 },
	{ "\x03\x02\xB2\x14",4,"\x0D",1 },
	
	{ "",-1,"",-1}
};




bool EC2::txblock( BLOCK *blk )
{
	int i = 0;
	while( blk[i].tlen != -1 )
	{
		trx(blk[i].tx, blk[i].tlen, blk[i].rx, blk[i].rlen );
		i++;
	}
}


EC2::EC2()
{
}


EC2::~EC2()
{
	close_port();
}

bool EC2::connect( char *port )
{
	const char cmd1[] = { 00,00,00 };
	
	cout <<"Connecting...";
	if( open_port(port) )
	{
		cout <<"OK"<<endl;
	}
	else
	{
		cerr << "Coulden't connect to EC2"<<endl;
	}
	char buf[4096];
	reset();
	write_port( 0x55 );
//	if( !read_port() == 0x5A )
//		return false;
	
	// ???
	write_port("\x00\x00\x00",3);
//	if( !read_port() == 0x03 )
//		return false;
	
	write_port("\x01\x03\x00",3);
//	if( !read_port() == 0x00 )
//		return false;
	
	write_port("\x06\x00\x00",3);
	
	
#if 0
	int ver = read_port();
	printf("EC2 firmware version = 0x%02X\n",ver);
	if( ver != 0x12 )
	{
		printf("Incompatible EC2 firmware version, version 0x12 required\n");
		return false;
	}
#endif
	//reset();	
	// miscelanious init
	// buid from captured data.
	// \todo figure out what this really does
	txblock(&init[0]);
	
//	reset();
//	target_go();
//	reset();
//	target_halt();
//	sleep(2);

//	sleep(1);
	memset(buf,0xFF,0xFF);
#if 0
		cout << "read flash"<<endl;
	trx("\x02\x02\xB6\x01",4,"\x80",1);
	trx("\x02\x02\xB2\x01",4,"\x14",1);
	trx("\x03\x02\xB2\x04",4,"\x0D",1);
	trx("\x0B\x02\x04\x00",4,"\x0D",1);
	trx("\x0D\x05\x85\x08\x01\x00\x00",7,"\x0D",1);
	trx("\x0D\x05\x84\x10\x00\x01\x00",7,"\x0D",1);
	trx("\x0D\x05\x82\x08\x01\x00\x00",7,"\x0D",1);
	trx("\x0D\x05\x84\x10\x00\x01\x00",7,"\x0D",1);
	
	write_port("\x11\x02\x05\x00",4);
	read_port( buf,4 );
	print_buf( buf,4);

	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);

	read_ram( buf, 0x00, 0xFF );
	print_buf( buf, 0xFF );
	

	memset( buf,0x45,0xFF);
	print_buf( buf, 0xFF );
#endif
#if 0
	write_ram( buf, 0, 255 );
	memset(buf,0xFF,0xFF);

	read_ram( buf, 0x00, 0xFF );
	print_buf( buf, 0xFF );
	
	
	cout << "Done"<<endl;
#endif
//	read( 0x5A );
#if 0
TX: 55						, ,"\x5A



TX: 00 00 00				, ,"\x03
TX: 01 03 00				, ,"\x00
TX: 06 00 00				, ,"\x10		// 0x10 is firmware version, this is 12 with the new version
#endif
#if 0
	cout << "======================================"<<endl<<endl;
	memset(buf,0xAA,512);
	//write_xdata_page(buf,0x00,0x00,0x0d);
	write_xdata(buf,0x0000,0x256);
	memset(buf,0xFF,512);
	//read_xdata( buf, 0x0000, 512 );
	//read_xdata_page(buf,0x00,0x00,0x0d);
	read_xdata( buf, 0x0000, 256 );
	print_buf(buf,256);
	cout << "??????????????????????????????????????????"<<endl<<endl;
	read_xdata_page(buf,0x00,0x00,0x0d);
	print_buf(buf,256);
#endif

	
	cout << "==============================================================="<<endl;
	cout << "SFR dump:" << endl;
	memset(buf,0x00,256);
	read_sfr( buf, 127, 0x80 );
	print_buf(buf,128);
	
	
	// attempt to read devid
//	memset(buf,0xFF,256);
//	write_port("\x0D\x05\x04\x00\x00\xff\xff",7);
//	read_port(buf,5);
//	print_buf(buf,5);
	flash_erase();
	// Read flash at 0x5A00
	cout << " initial state:"<<endl;
	memset(buf,0xFF,513);
	read_flash(buf,0x0000,513);
	print_buf(buf,512);
	
	for(int i=0; i<513;i++)
		buf[i] = i&0xFF;
//	write_flash_sector( buf,0x0000 );
	char *b = new char[0x10000];
	memset( b, 0x55, 0x10000 );
	write_flash( b, 0, 0xFDFD );


	cout << " after write:"<<endl;
	memset(buf,0xFF,513);
	read_flash(buf,0x0000,513);
	print_buf(buf,513);
	
}


void EC2::read_ram( char *buf, int start_addr, int len )
{
	char cmd[4];
	
	for(int i=start_addr; i<len; i+=0x0C )
	{
		cmd[0] = 0x06;
		cmd[1] = 0x02;
		cmd[2] = start_addr+i;
		cmd[3] = (len-i)>=0x0C ? 0x0C : (len-i);
		write_port( cmd, 0x04 );
		read_port( buf+i, cmd[3] );
	}
}


/** Write data into the micros RAM
  * cmd  07 <addr> <len> <a> <b>
  * len is 1 or 2
  * addr is micros data ram location
  * a  = first databyte to write
  * b = second databyte to write
  */
bool EC2::write_ram( char *buf, int start_addr, int len )
{
	assert( start_addr>=0 && start_addr<=0xFF );
	char cmd[5];
	for(int i=0; i<len; i+=2 )
	{
		cmd[0] = 0x07;
		cmd[1] = start_addr + i;
		int blen = len-i;
		if( blen>=2 )
		{
			cmd[2] = 0x02;		// two bytes
			cmd[3] = buf[i];
			cmd[4] = buf[i+1];
			write_port( cmd, 5 );
		}
		else
		{
			cmd[2] = 0x01;		// must be 1
			cmd[3] = buf[i];
			write_port( cmd, 4 );
		}
	}
}



/** write to targets XDATA address space
  * Preamble... trx("\x03\x02\x2D\x01",4,"\x0D",1);
  *
  * Select page address
  * trx("\x03\x02\x32\x00",4,"\x0D",1);
  * cmd: 03 02 32 <addrH>
  * where addrH is the top 8 bits of the address
  * cmd : 07 <addrL> <len> <a> <b>
  * addrL is low byte of address
  * len is 1 of 2
  * a is first byte to write
  * b is second byte to write
  *
  * closing :
  * cmd 03 02 2D 00
  */
bool EC2::write_xdata( char *buf, int start_addr, int len )
{ 
	assert( start_addr>=0 && start_addr<=0xFFFF );
	int addr;
	
	// how many pages?
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len) >> 8 ) & 0xFF;
	char ofs=0;
	int blen;
	for( int page = start_page; page<=last_page; page++ )
	{
		char start = page==start_page ? start_addr&0x00FF : 0x00;
		blen = start_page+len - page;
		blen = blen > 0xFF ? 0xFF : blen;	// only one page at a time
		write_xdata_page( buf+ofs, page, start, blen );
		ofs += 0xFF;	// next page
	}
	return true;
}

/** this function performs the preamble and postamble
*/
bool EC2::write_xdata_page( char *buf, unsigned char page, unsigned char start, int len )
{
	unsigned char cmd[5];
	trx("\x03\x02\x2D\x01",4,"\x0D",1);		// preamble
	
	// select page
	cmd[0] = 0x03;
	cmd[1] = 0x02;
	cmd[2] = 0x32;
	cmd[3] = page;
	trx( (char*)cmd,4,"\x0D",1 );
	
	// write bytes to page
	// upto 2 at a time
	for( int i=start; i<len; i+=2 )
	{
		if( (len-i) > 1 )
		{
			cmd[0] = 0x07;
			cmd[1] = i;
			cmd[2] = 2;
			cmd[3] = (char)buf[i];
			cmd[4] = (char)buf[i+1];
			trx((char*)cmd,5,"\x0d",1);
		}
		else
		{
			cmd[0] = 0x07;
			cmd[1] = i;
			cmd[2] = 1;
			cmd[3] = (char)buf[i];
			trx((char*)cmd,4,"\x0d",1);
		}
		
	}
	
	trx("\x03\x02\x2D\x00",4,"\x0D",1);		// close xdata write session
	return true;
}

/** Read len bytes of data from the target
  * starting at start_addr into buf
  *
  * T 03 02 2D 01  R 0D
  * T 03 02 32 <addrH>
  * T 06 02 addrL len
  * where len <= 0x0C
  */
void EC2::read_xdata( char *buf, int start_addr, int len )
{
	assert( start_addr>=0 && start_addr<=0xFFFF );
	int addr;
	
	// how many pages?
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len) >> 8 ) & 0xFF;
	char ofs=0;
	int blen;
	for( int page = start_page; page<=last_page; page++ )
	{
		char start = page==start_page ? start_addr&0x00FF : 0x00;
		blen = start_page+len - page;
		blen = blen > 0xFF ? 0xFF : blen;	// only one page at a time
		read_xdata_page( buf+ofs, page, start, blen );
		ofs += 0xFF;	// next page
	}
}


void EC2::read_xdata_page( char *buf, unsigned char page, unsigned char start, int len )
{
	assert( (start+len) <= 0x100 );	// must be in one page only
	unsigned char cmd[0x0C];
	trx("\x03\x02\x2D\x01",4,"\x0D",1);
	
	// select page
	cmd[0] = 0x03;
	cmd[1] = 0x02;
	cmd[2] = 0x32;
	cmd[3] = page;
	trx((char*)cmd,4,"\x0D",1);
	
	cmd[0] = 0x06;
	cmd[1] = 0x02;
	cout <<"LOOP"<<endl;
	// read the rest
	for( unsigned int i=0; i<len; i+=0x0C )
	{
		cmd[2] = i & 0xFF;
		cmd[3] = (len-i)>=0x0C ? 0x0C : (len-i);
		write_port( (char*)cmd, 4 );
		read_port( buf, cmd[3] );
		buf += 0x0C;
	}
}


bool EC2::read_flash( char *buf, int start_addr, int len )
{
	unsigned char cmd[0x0C];
	unsigned char acmd[7];
	int addr;
	
	// Preamble
	trx("\x02\x02\xB6\x01",4,"\x80",1);
	trx("\x02\x02\xB2\x01",4,"\x14",1);
	trx("\x03\x02\xB2\x04",4,"\x0D",1);
	trx("\x0B\x02\x04\x00",4,"\x0D",1);
	trx("\x0D\x05\x85\x08\x01\x00\x00",7,"\x0D",1);
	addr = start_addr;
	memcpy(acmd,"\x0D\x05\x84\x10\x00\x00\x00",7);
	acmd[4] = addr & 0xFF;		// Little endian
	acmd[5] = (addr>>8) & 0xFF;	// patch in actual address
	trx( (char*)acmd, 7, "\x0D", 1 );	// first address write

	trx("\x0D\x05\x82\x08\x01\x00\x00",7,"\x0D",1);	// 82 flash control reg
	cout <<"flash read begin"<<endl;
	for( int i=0; i<len; i+=0x0C )
	{
		addr = start_addr + i;
		acmd[4] = addr & 0xFF;		// Little endian, flash address
		acmd[5] = (addr>>8) & 0xFF;	// patch in actual address
		trx( (char*)acmd, 7, "\x0D", 1 );	// write address
		// read command
		// cmd 0x11 0x02 <len> 00
		// where len <= 0xC0
		cmd[0] = 0x11;
		cmd[1] = 0x02;
		cmd[2] = (len-i)>=0x0C ? 0x0C : (len-i);
		cmd[3] = 0x00;
		write_port( (char*)cmd, 4 );
		read_port( buf+i, cmd[2] ); 
	}
	cout <<"flash read complete"<<endl;
	// postamble
	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);
	return true;
}

/** Write to flash memory
  * This function assumes the specified area of flash is already erased 
  * to 0xFF before it is called.
  */
bool EC2::write_flash( char *buf, int start_addr, int len )
{
	char wr_erase_lock;
	typedef struct FLBLOCK
	{
		int start;
		int end;
	};
	const FLBLOCK map[] = {
	{ 0x0000, 0x1FFF },
	{ 0x2000, 0x3FFF },
	{ 0x4000, 0x5FFF },
	{ 0x6000, 0x7FFF },
	{ 0x8000, 0x9FFF },
	{ 0xA000, 0xBFFF },
	{ 0xC000, 0xDFFF },
	{ 0xE000, 0xFDFD }};
#if 0		
	// IDE write a sector at a time so we do likewise for now
	// all unchanged bytes are written to FF
	//
	read_flash( &wr_erase_lock, 0xFDFE, 1 );
	
	// check if the sector we wish to write is locked.
	// if it is we must fail or erase the entire device
	
	// which block are we in?
	for( int i=0; i<8; i++ )
	{
		if( ( map[i].start <= start_addr )	&&
			( start_addr <= map[i].end ) )
		{
			if( (wr_erase_lock>>i)&0x01 )
			{
				return false;	// fail, attempt to write to locked sector
			}
		}
	}
	
	// check read lock byte
	//...
	
#endif
		BLOCK flash_pre[] = {
	{ "\x02\x02\xB6\x01", 4, "\x80", 1 },
	{ "\x02\x02\xB2\x01", 4, "\x14", 1 },
	{ "\x03\x02\xB2\x04", 4, "\x0D", 1 },
	{ "\x0B\x02\x04\x00", 4, "\x0D", 1 },
	{ "\x0D\x05\x85\x08\x01\x00\x00", 7,"\x0D", 1 },
	{ "\x0D\x05\x82\x08\x20\x00\x00", 7,"\x0D", 1 },
	{ "", -1, "", -1 } };
	char cmd[255];
	txblock( flash_pre );

	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (start_addr >> 8) & 0xFF;
	trx( cmd, 7, "\x0D", 1 ); 
	
//T 0F 01 A5				R 0D
//T 0D 05 82 08 02 00 00	R 0D
//T 0E 00					R A5	// read back low address?
//T 0E 00					R FF	// checkigng for blank?
//	trx("\x0F\x01\x00",3,"\x0D",1);
//	trx("\x0D\x05\x82\x08\x02\x00\x00",7,"\x0D",1);
//	trx("\x0E\x00",2,"\x00",1);
//	trx("\x0E\x00",2,"\xFF",1);
	
	trx("\x0D\x05\x82\x08\x10\x00\x00", 7, "\x0D", 1 );	// Flash CTRL 
	
	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (start_addr >> 8) & 0xFE;
	trx( cmd, 7, "\x0D", 1 ); 

	// write the sector
	for( int i=0; i <=len; i+=0x0C )
	{
		cmd[0] = 0x12;
		cmd[1] = 0x02;
		cmd[2] = (len-i) > 0x0C ? 0x0C : (len-i);
		cmd[3] = 0x00;
//		printf("cmd[2] = %i\n",cmd[2]);
		memcpy( &cmd[4], &buf[i], cmd[2] );
		trx( cmd, 4 + cmd[2], "\x0D", 1 );
	}
	cout << "bulk done"<<endl;
	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);	// FLASHCON
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);


}

/** write an entire flash sector
  *
  */
bool EC2::write_flash_sector( char *buf, int sector_start_addr )
{
	BLOCK flash_pre[] = {
	{ "\x02\x02\xB6\x01", 4, "\x80", 1 },
	{ "\x02\x02\xB2\x01", 4, "\x14", 1 },
	{ "\x03\x02\xB2\x04", 4, "\x0D", 1 },
	{ "\x0B\x02\x04\x00", 4, "\x0D", 1 },
	{ "\x0D\x05\x85\x08\x01\x00\x00", 7,"\x0D", 1 },
	{ "\x0D\x05\x82\x08\x20\x00\x00", 7,"\x0D", 1 },
	{ "", -1, "", -1 } };
	char cmd[255];
	txblock( flash_pre );

	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (sector_start_addr >> 8) & 0xFF;
	trx( cmd, 7, "\x0D", 1 ); 
	
//T 0F 01 A5				R 0D
//T 0D 05 82 08 02 00 00	R 0D
//T 0E 00					R A5	// read back low address?
//T 0E 00					R FF	// checkigng for blank?
//	trx("\x0F\x01\x00",3,"\x0D",1);
//	trx("\x0D\x05\x82\x08\x02\x00\x00",7,"\x0D",1);
//	trx("\x0E\x00",2,"\x00",1);
//	trx("\x0E\x00",2,"\xFF",1);
	
	trx("\x0D\x05\x82\x08\x10\x00\x00", 7, "\x0D", 1 );	// Flash CTRL 
	
	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (sector_start_addr >> 8) & 0xFE;
	trx( cmd, 7, "\x0D", 1 ); 

	// write the sector
	for( int i=0; i <=0x1FF; i+=0x0C )
	{
		cmd[0] = 0x12;
		cmd[1] = 0x02;
		cmd[2] = (512-i) > 0x0C ? 0x0C : (512-i);
		cmd[3] = 0x00;
		printf("cmd[2] = %i\n",cmd[2]);
		memcpy( &cmd[4], &buf[i], cmd[2] );
		trx( cmd, 4 + cmd[2], "\x0D", 1 );
	}
	cout << "bulk done"<<endl;
	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);	// FLASHCON
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);
}


/** Erase all flash in the device
  */
void EC2::flash_erase()
{
	BLOCK fe[] =
	{
		{"\x55",1,"\x5A",1},
		{"\x01\x03\x00",3,"\x00",1},
		{"\x06\x00\x00",3,"\x12",1},
		{"\x04",1,"\x0D",1},
		{"\x1A\x06\x00\x00\x00\x00\x00\x00",8,"\x0D",1},
		{"\x0B\x02\x02\x00",4,"\x0D",1},
		{"\x14\x02\x10\x00",4,"\x04",1},
		{"\x16\x02\x01\x20",4,"\x01\x00",2},
		{"\x14\x02\x10\x00",4,"\x04",1},
		{"\x16\x02\x81\x20",4,"\x01\x00",2},
		{"\x14\x02\x10\x00",4,"\x04",1},
		{"\x16\x02\x81\x30",4,"\x01\x00",2},
		{"\x15\x02\x08\x00",4,"\x04",1},
		{"\x16\x01\xE0",3,"\x00",1},
		{"\x0B\x02\x01\x00",4,"\x0D",1},
		{"\x13\x00",2,"\x01",1},
		{"\x0A\x00",2,"\x21\x01\x03\x00\x00\x12",6},
		{"\x0B\x02\x04\x00",4,"\x0D",1},
		{"\x0D\x05\x85\x08\x00\x00\x00",7,"\x0D",1},
		{"\x0D\x05\x82\x08\x20\x00\x00",7,"\x0D",1},
		{"\x0D\x05\x84\x10\xFF\x7F\x00",7,"\x0D",1},
		{"\x0F\x01\xA5",3,"\x0D",1},
		{"\x0D\x05\x84\x10\xFF\xFD\x00",7,"\x0D",1},
		{"\x0F\x01\xA5",3,"\x0D",1},
		{"\x0D\x05\x82\x08\x02\x00\x00",7,"\x0D",1},
		{"\x0E\x00",2,"\xA5",1},
		{"\x0E\x00",2,"\xFF",1},
		{ "",-1,"",-1}};

	reset();
	txblock(fe);
	reset();
	// init after reset
	write_port( 0x55 );
	write_port("\x00\x00\x00",3);
	write_port("\x01\x03\x00",3);
	write_port("\x06\x00\x00",3);
	txblock(&init[0]);
}


/** SFR read command
  * T 02 02 <addr> <len>
  * len <= 0x0C
  * addr = SFR address 0x80 - 0xFF
  */
unsigned char EC2::read_sfr( uint8_t addr )
{
	char buf[0x0C];
	
	assert( addr >= 0x80 );
	buf[0] = 0x02;
	buf[1] = 0x02;
	buf[2] = addr;
	buf[3] = 0x01;
	write_port( buf, 4 );
	return read_port();
}

/** SFR read command
  * T 02 02 <addr> <len>
  * len <= 0x0C
  * addr = SFR address 0x80 - 0xFF
  */
void EC2::read_sfr( char *buf, uint8_t len, uint8_t addr  )
{
	assert( int(addr+len) <= 0xFF );
	assert( addr >= 0x80 );
	char cmd[4];
	
	cmd[0] = 0x02;
	cmd[1] = 0x02;
	for( unsigned int i=0; i<len; i+=0x0C )
	{
		cmd[2] = addr + i;
		cmd[3] = (len-i)>=0x0C ? 0x0C : (len-i);
		write_port( cmd, 4 );
		read_port( buf, cmd[3] );
		buf += 0x0C;
	}
}

bool EC2::target_go()
{
	write_port("\x0B\x02\x00\x00",4);
	if( read_port()!=0x0D )
		return false;
	write_port("\x09\x00",2);
	if( read_port()!=0x0D )
		return false;
	else
	return true;
}

bool EC2::target_halt()
{
	// FIXME: this is broken, need to understand the byte sequence more
	cout <<"Halting target"<<endl;
	write_port("\x0B\x02\x01\x00",4);
	if( read_port()!=0x0D )
		return false;
	for(int i=0; i<8;i++)
	{
		write_port("\x13\x00",2);
		printf("halt = 0x%02X\n",read_port());
	}
	write_port("\x13\x00",2);
	if( read_port()!=0x0D )		// currently we read 0x00 but that is because we can't halt
		return false;
	else
	return true;
}

bool EC2::trx( char *txbuf, int txlen, char *rxexpect, int rxlen )
{
	char rxbuf[256];
	write_port( txbuf, txlen );
	if( read_port( rxbuf, rxlen ) )
		return memcmp( rxbuf, rxexpect, rxlen )==0 ? true : false;
	else
	{
		cout <<"Unexpected response"<<endl;
		return false;
	}
}

/** Reset the EC2 by turning off DTR for a short period
  */
void EC2::reset()
{
	cout <<"Resetting EC2..."<<flush;
	usleep(100);
	DTR(false);
	usleep(100);
	DTR(true);
	usleep(10000);	// 10ms minimum appears to be about 8ms so play it safe
	cout << "done"<<endl;
}

///////////////////////////////////////////////////////////////////////////////
// RS232 PORT control														 //
///////////////////////////////////////////////////////////////////////////////
void EC2::DTR(bool on)
{
	int status;
	ioctl( m_fd, TIOCMGET, &status );
	if( on )
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;
	ioctl( m_fd, TIOCMSET, &status );
}

void EC2::RTS(bool on)
{
	int status;
	ioctl( m_fd, TIOCMGET, &status );
	if( on )
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;
	ioctl( m_fd, TIOCMSET, &status );
}

bool EC2::open_port( char *port )
{
	m_fd = open( port, O_RDWR | O_NOCTTY | O_NDELAY);
	if( m_fd == -1 )
	{
		/*
		* Could not open the port.
		*/
		perror("open_port: Unable to open /dev/ttyS0 - ");
		return false;
	}
	else
	{
		fcntl( m_fd, F_SETFL, 0 );
		struct termios options;

		// Get the current options for the port...
		tcgetattr( m_fd, &options );
		
		// Set the baud rates to 115200
		cfsetispeed(&options, B115200);
		cfsetospeed(&options, B115200);

		// Enable the receiver and set local mode...
		options.c_cflag |= (CLOCAL | CREAD);

		// set 8N1
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		
		// Disable hardware flow control
		options.c_cflag &= ~CRTSCTS;
		
		// Disable software flow control
		options.c_iflag &= ~(IXON | IXOFF | IXANY);
		
		// select RAW input
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		
		// select raw output
		options.c_oflag &= ~OPOST;
		
		// Set the new options for the port...
		tcsetattr( m_fd, TCSANOW, &options );
	}
	RTS(true);
	DTR(true);
	return true;
}

bool EC2::write_port( char ch )
{
	return write_port( &ch, 1 );
}

bool EC2::write_port( char *buf, int len )
{
	cout << "TX: ";
	tx_flush();
	rx_flush();
	print_buf( buf, len );
	
	for(int i=0 ; i<len; i++ )
	{
		write( m_fd, buf+i, 1);
		usleep(5000);			// 5ms sleep
	}
	return true;
}


bool EC2::read_port( char *buf, int len )
{
	fd_set			input;
	struct timeval	timeout;
	
	// Initialize the input set
    FD_ZERO( &input );
    FD_SET( m_fd, &input );
	fcntl(m_fd, F_SETFL, 0);	// block if not enough characters available
	
	// Initialize the timeout structure
    timeout.tv_sec  = 2;	// 1 seconds timeout
    timeout.tv_usec = 0;
	
	char *cur_ptr = buf;
	int cnt=0, r, n;
	
	// Do the select
	n = select( m_fd+1, &input, NULL, NULL, &timeout );
	if (n < 0)
	{
		perror("select failed");
		exit(-1);
		return false;
	}
	else if (n == 0)
	{
		puts("TIMEOUT");
		return false;
	}
	else
	{
		r = read( m_fd, cur_ptr, len-cnt );
		cout <<"RX: ";
		print_buf( buf, len);
		return true;
	}
}

void EC2::rx_flush()
{
	tcflush( m_fd, TCIFLUSH );
}

void EC2::tx_flush()
{
	tcflush( m_fd, TCOFLUSH );
}

int EC2::read_port()
{
	char ch;
	if( read_port(&ch,1) )
		return ch;
	else
		return -1;
}


void EC2::close_port()
{
	close( m_fd );
}


void EC2::print_buf( char *buf, int len )
{
	for( int i=0; i<len; i++ )
	{
		printf("%02X ",(unsigned char)buf[i]);
	}
	printf("\n");
}
