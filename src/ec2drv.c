/** EC2 Driver Library
  *
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>			// UNIX standard function definitions
#include <fcntl.h>			// File control definitions
#include <errno.h>			// Error number definitions
#include <termios.h>		// POSIX terminal control definitions
#include <sys/ioctl.h>
#include "ec2drv.h"


static int m_fd;			///< file descriptor for com port

typedef struct
{
	char *tx;
	int tlen;
	char *rx;
	int rlen;
} EC2BLOCK;

// Foward declarations

// Internal functions
static void init_ec2();
static BOOL txblock( EC2BLOCK *blk );
static BOOL trx( char *txbuf, int txlen, char *rxexpect, int rxlen );

// PORT support
static BOOL open_port( char *port );
static BOOL write_port_ch( char ch );
static BOOL write_port( char *buf, int len );
static int read_port_ch();
static BOOL read_port( char *buf, int len );
static void rx_flush();
static void tx_flush();
static void close_port();
static void DTR(BOOL on);
static void RTS(BOOL on);




/** Connect to the ec2 device on the specified port.
  * This will perform any initialisation required to bring the device into
  * an active state
  */
BOOL ec2_connect( char *port )
{
	int ec2_sw_ver;
	const char cmd1[] = { 00,00,00 };

	if( !open_port(port) )
	{
		printf("Coulden't connect to EC2\n");
		return FALSE;
	}
	ec2_reset();
	
	if( !trx("\x55",1,"\x5A",1) )
		return FALSE;
	if( !trx("\x00\x00\x00",3,"\x03",1) )
		return FALSE;
	if( !trx("\x01\x03\x00",3,"\x00",1) )
		return FALSE;

	write_port("\x06\x00\x00",3);
	ec2_sw_ver = read_port_ch();
	printf("EC2 firmware version = 0x%02X\n",ec2_sw_ver);
	if( ec2_sw_ver != 0x12 )
	{
		printf("Incompatible EC2 firmware version, version 0x12 required\n");
		return FALSE;
	}
	init_ec2();
	
}


/** SFR read command
  * T 02 02 <addr> <len>
  * len <= 0x0C
  * addr = SFR address 0x80 - 0xFF
  */
void ec2_read_sfr( char *buf, uint8_t len, uint8_t addr  )
{
	char cmd[4];
	unsigned int i;
	assert( addr >= 0x80 );
	assert( (int)addr+len <= 0xFF );

	cmd[0] = 0x02;
	cmd[1] = 0x02;
	for( i=0; i<len; i+=0x0C )
	{
		cmd[2] = addr + i;
		cmd[3] = (len-i)>=0x0C ? 0x0C : (len-i);
		write_port( cmd, 4 );
		read_port( buf, cmd[3] );
		buf += 0x0C;
	}
}

void ec2_read_ram( char *buf, int start_addr, int len )
{
	int i;
	char cmd[4];
	
	for( i=start_addr; i<len; i+=0x0C )
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
BOOL ec2_write_ram( char *buf, int start_addr, int len )
{
	int i, blen;
	char cmd[5];
	

	assert( start_addr>=0 && start_addr<=0xFF );
	for( i=0; i<len; i+=2 )
	{
		cmd[0] = 0x07;
		cmd[1] = start_addr + i;
		blen = len-i;
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
BOOL ec2_write_xdata( char *buf, int start_addr, int len )
{ 
	int addr, blen, page;
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len) >> 8 ) & 0xFF;
	char ofs=0;
	char start;

	assert( start_addr>=0 && start_addr<=0xFFFF );	
	for( page = start_page; page<=last_page; page++ )
	{
		start = page==start_page ? start_addr&0x00FF : 0x00;
		blen = start_page+len - page;
		blen = blen > 0xFF ? 0xFF : blen;	// only one page at a time
		ec2_write_xdata_page( buf+ofs, page, start, blen );
		ofs += 0xFF;	// next page
	}
	return TRUE;
}

/** this function performs the preamble and postamble
*/
BOOL ec2_write_xdata_page( char *buf, unsigned char page,
						   unsigned char start, int len )
{
	int i;
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
	for( i=start; i<len; i+=2 )
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
	return TRUE;
}

/** Read len bytes of data from the target
  * starting at start_addr into buf
  *
  * T 03 02 2D 01  R 0D
  * T 03 02 32 <addrH>
  * T 06 02 addrL len
  * where len <= 0x0C
  */
void ec2_read_xdata( char *buf, int start_addr, int len )
{
	int addr, blen, page;
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len) >> 8 ) & 0xFF;
	char start, ofs=0;
	
	assert( start_addr>=0 && start_addr<=0xFFFF );
	
	for( page = start_page; page<=last_page; page++ )
	{
		start = page==start_page ? start_addr&0x00FF : 0x00;
		blen = start_page+len - page;
		blen = blen > 0xFF ? 0xFF : blen;	// only one page at a time
		ec2_read_xdata_page( buf+ofs, page, start, blen );
		ofs += 0xFF;						// next page
	}
}


void ec2_read_xdata_page( char *buf, unsigned char page,
						  unsigned char start, int len )
{
	unsigned int i;
	unsigned char cmd[0x0C];
	assert( (start+len) <= 0x100 );	// must be in one page only
	
	trx("\x03\x02\x2D\x01",4,"\x0D",1);
	
	// select page
	cmd[0] = 0x03;
	cmd[1] = 0x02;
	cmd[2] = 0x32;
	cmd[3] = page;
	trx((char*)cmd,4,"\x0D",1);
	
	cmd[0] = 0x06;
	cmd[1] = 0x02;
	// read the rest
	for( i=0; i<len; i+=0x0C )
	{
		cmd[2] = i & 0xFF;
		cmd[3] = (len-i)>=0x0C ? 0x0C : (len-i);
		write_port( (char*)cmd, 4 );
		read_port( buf, cmd[3] );
		buf += 0x0C;
	}
}

BOOL ec2_read_flash( char *buf, int start_addr, int len )
{
	unsigned char cmd[0x0C];
	unsigned char acmd[7];
	int addr, i;
	
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

	for( i=0; i<len; i+=0x0C )
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

	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);
	return TRUE;
}

/** Write to flash memory
  * This function assumes the specified area of flash is already erased 
  * to 0xFF before it is called.
  */
BOOL ec2_write_flash( char *buf, int start_addr, int len )
{
	int		i;
	char cmd[255];
	EC2BLOCK flash_pre[] = {
	{ "\x02\x02\xB6\x01", 4, "\x80", 1 },
	{ "\x02\x02\xB2\x01", 4, "\x14", 1 },
	{ "\x03\x02\xB2\x04", 4, "\x0D", 1 },
	{ "\x0B\x02\x04\x00", 4, "\x0D", 1 },
	{ "\x0D\x05\x85\x08\x01\x00\x00", 7,"\x0D", 1 },
	{ "\x0D\x05\x82\x08\x20\x00\x00", 7,"\x0D", 1 },
	{ "", -1, "", -1 } };

	txblock( flash_pre );

	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (start_addr >> 8) & 0xFF;
	trx( cmd, 7, "\x0D", 1 ); 
	
	trx("\x0D\x05\x82\x08\x10\x00\x00", 7, "\x0D", 1 );	// Flash CTRL 
	memcpy( cmd, "\x0D\x05\x84\x10\x00\x00\x00", 7 );
	cmd[5] = (start_addr >> 8) & 0xFE;
	trx( cmd, 7, "\x0D", 1 ); 

	// write the sector
	for( i=0; i <=len; i+=0x0C )
	{
		cmd[0] = 0x12;
		cmd[1] = 0x02;
		cmd[2] = (len-i) > 0x0C ? 0x0C : (len-i);
		cmd[3] = 0x00;
		memcpy( &cmd[4], &buf[i], cmd[2] );
		trx( cmd, 4 + cmd[2], "\x0D", 1 );
	}

	trx("\x0D\x05\x82\x08\x00\x00\x00",7,"\x0D",1);	// FLASHCON
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);
}
/** Erase all flash in the device
  */
void ec2_flash_erase()
{
	BOOL r = TRUE;
	EC2BLOCK fe[] =
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

	ec2_reset();
	r &= txblock(fe);
	ec2_reset();
	
	// init after reset
	r &= write_port_ch( 0x55 );
	r &= write_port("\x00\x00\x00",3);
	r &= write_port("\x01\x03\x00",3);
	r &= write_port("\x06\x00\x00",3);
//	r &= txblock( &init[0] );
	init_ec2();
	return r;
}
///////////////////////////////////////////////////////////////////////////////
/// Internal helper functions                                               ///
///////////////////////////////////////////////////////////////////////////////

/** Send a block of characters tothe port and check for the correct reply
  */
static BOOL trx( char *txbuf, int txlen, char *rxexpect, int rxlen )
{
	char rxbuf[256];
	write_port( txbuf, txlen );
	if( read_port( rxbuf, rxlen ) )
		return memcmp( rxbuf, rxexpect, rxlen )==0 ? TRUE : FALSE;
	else
		return FALSE;
}

/** Reset the EC2 by turning off DTR for a short period
  */
void ec2_reset()
{
	usleep(100);
	DTR( FALSE );
	usleep(100);
	DTR( TRUE );
	usleep(10000);	// 10ms minimum appears to be about 8ms so play it safe
}

void init_ec2()
{
	EC2BLOCK init[] = {
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
	
	{ "",-1,"",-1 } };
	
	txblock( init );
}


BOOL ec2_target_go()
{
	write_port("\x0B\x02\x00\x00",4);
	if( read_port_ch()!=0x0D )
		return FALSE;
	write_port("\x09\x00",2);
	if( read_port_ch()!=0x0D )
		return FALSE;
	else
		return TRUE;
}

BOOL ec2_target_halt()
{
	int i;
	// FIXME: this is broken, need to understand the byte sequence more
	write_port("\x0B\x02\x01\x00",4);
	if( read_port_ch()!=0x0D )
		return FALSE;
	for( i=0; i<8;i++)
	{
		write_port("\x13\x00",2);
	}
	write_port("\x13\x00",2);
	if( read_port_ch()!=0x0D )		// currently we read 0x00 but that is because we can't halt
		return FALSE;
	else
		return TRUE;
}


BOOL txblock( EC2BLOCK *blk )
{
	int i = 0;
	while( blk[i].tlen != -1 )
	{
		trx(blk[i].tx, blk[i].tlen, blk[i].rx, blk[i].rlen );
		i++;
	}
}


///////////////////////////////////////////////////////////////////////////////
/// COM port control functions                                              ///
///////////////////////////////////////////////////////////////////////////////
static BOOL open_port( char *port )
{
	m_fd = open( port, O_RDWR | O_NOCTTY | O_NDELAY);
	if( m_fd == -1 )
	{
		/*
		* Could not open the port.
		*/
		printf("open_port: Unable to open %s\n", port );
		return FALSE;
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
	RTS(TRUE);
	DTR(TRUE);
	return TRUE;
}

static BOOL write_port_ch( char ch )
{
	return write_port( &ch, 1 );
}

static BOOL write_port( char *buf, int len )
{
	int i,j;
	tx_flush();
	rx_flush();
	// The EC2 dosen't like back to back writes!
	// have to slow things down a tad
	for( i=0; i<len; i++ )
	{
		write( m_fd, buf+i, 1);
		usleep(4000);			// 4ms inter character delay
	}
	return TRUE;
}

static int read_port_ch()
{
	char ch;
	if( read_port( &ch, 1 ) )
		return ch;
	else
		return -1;
}

static BOOL read_port( char *buf, int len )
{
	fd_set			input;
	struct timeval	timeout;
	
	// Initialize the input set
    FD_ZERO( &input );
    FD_SET( m_fd, &input );
	fcntl(m_fd, F_SETFL, 0);	// block if not enough characters available
	
	// Initialize the timeout structure
    timeout.tv_sec  = 2;		// 1 seconds timeout
    timeout.tv_usec = 0;
	
	char *cur_ptr = buf;
	int cnt=0, r, n;
	
	// Do the select
	n = select( m_fd+1, &input, NULL, NULL, &timeout );
	if (n < 0)
	{
		perror("select failed");
		exit(-1);
		return FALSE;
	}
	else if (n == 0)
	{
		puts("TIMEOUT");
		return -1;
	}
	else
	{
		r = read( m_fd, cur_ptr, len-cnt );
		return 1;
	}
}


static void rx_flush()
{
	tcflush( m_fd, TCIFLUSH );
}

static void tx_flush()
{
	tcflush( m_fd, TCOFLUSH );
}

static void close_port()
{
	close( m_fd );
}

static void DTR(BOOL on)
{
	int status;
	ioctl( m_fd, TIOCMGET, &status );
	if( on )
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;
	ioctl( m_fd, TIOCMSET, &status );
}

static void RTS(BOOL on)
{
	int status;
	ioctl( m_fd, TIOCMGET, &status );
	if( on )
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;
	ioctl( m_fd, TIOCMSET, &status );
}
