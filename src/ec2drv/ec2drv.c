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
#include "config.h"

#undef EC2TRACE				// define to enable tracing
#define MAJOR_VER 0
#define MINOR_VER 2

/** Retrieve the ec2drv library version
  * \returns the version.  upper byte is major version, lower byte is minor
  */
uint16_t ec2drv_version()
{
	return (MAJOR_VER<<8) | MINOR_VER;
}

static int m_fd;			///< file descriptor for com port

typedef struct
{
	char *tx;
	int tlen;
	char *rx;
	int rlen;
} EC2BLOCK;

// Foward declarations
static bp_flags;					// mirror of EC2 breakpoint byte
static uint16_t  bpaddr[4];			// breakpoint addresses

// Internal functions
static void init_ec2();
static BOOL txblock( EC2BLOCK *blk );
static BOOL trx( char *txbuf, int txlen, char *rxexpect, int rxlen );
static void print_buf( char *buf, int len );
static int resetBP(void);
static int getNextBPIdx(void);
static int getBP( uint16_t addr );
static BOOL setBpMask( int bp, BOOL active );
static uint8_t sfr_fixup( uint8_t addr );

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
  *
  * \param port name of the linux device the EC2 is connected to, eg "/dev/ttyS0"
  * \returns TRUE on success
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
	printf("EC2 firmware version = 0x%02x\n",ec2_sw_ver);
	if( ec2_sw_ver != 0x12 )
	{
		printf("Incompatible EC2 firmware version, version 0x12 required\n");
		return FALSE;
	}
//	init_ec2(); Does slightly more than ec2_target_reset() but dosen't seem necessary
	ec2_target_reset();	
}

/** disconnect from the EC2 releasing the serial port
  */
void ec2_disconnect()
{
	DTR(FALSE);
	close_port();
}


/** Translates certian special SFR addresses for read and write 
  *  reading or writing the sfr address as per datasheet returns incorrect
  * information.
  * These mappings seem necessary due to the way the hardware is implemented.
  *  The access is the same byte sequence as a normal SFR but the address is
  * much lower starting arround 0x20.
  */
static uint8_t sfr_fixup( uint8_t addr )
{
	switch( addr )
	{
		case 0xD0:	return 0x23;	// PSW
		case 0xE0:	return 0x22;	// ACC
		default:	return addr;
	}
}


/** SFR read command							<br>
  * T 02 02 addr len							<br>
  * len <= 0x0C									<br>
  * addr = SFR address 0x80 - 0xFF				<br>
  *
  * \param buf buffer to store the read data
  * \param addr address to read from, must be in SFR area, eg 0x80 - 0xFF
  */
void ec2_read_sfr( char *buf, uint8_t addr )
{
	assert( addr >= 0x80 );
	ec2_read_ram_sfr( buf, sfr_fixup( addr ), 1, TRUE );
}

/** write to a SFR (Special Function Register)
  * NOTE some SFR's appear to accept writes but do not take any action on the
  * heardware.  This seems to be the same SFRs that the SI labs IDE can't make
  * change either.
  *
  * One possible work arroud is to place a couple of bute program in the top of
  * flash and then the CPU state can be saved (via EC2) and then values poked 
  * into regs and this code stepped through.  This would mean we could change 
  * any sfr provided the user application can spare a few bytes of code memory
  * The SFR's that don';t write correctly are asubset of the bit addressable ones
  * for some of them the SI labs IDE uses a different command.
  * This function will add support for knowen alternative access methods as found.
  *
  * \param buf buffer containing data to write
  * \param addr sfr address to begin writing at, must be in SFR area, eg 0x80 - 0xFF
  * \param len Number of bytes to write.
  */
void ec2_write_sfr( char *buf, uint8_t addr )
{
	uint8_t i;
	char cmd[4];
	assert( addr >= 0x80 );
	
	cmd[0] = 0x03;
	cmd[1] = 0x02;
	cmd[2] = sfr_fixup(addr);
	cmd[3] = buf[0];
	trx(cmd,4,"\x0D",1);
}



/** Read ram
  * Read data from the internal data memory
  * \param buf buffer to store the read data
  * \param start_addr address to begin reading from, 0x00 - 0xFF
  * \param len Number of bytes to read, 0x00 - 0xFF
  */
void ec2_read_ram( char *buf, int start_addr, int len )
{
 	char cmd[4], rbuf[2], tmp[2];
 	int i;
	
	ec2_read_ram_sfr(  buf, start_addr, len, FALSE );	
	if( start_addr <= 0x01 )
	{
		// special case for first 2 bytes of ram
		write_port("\x06\x02\x00\x02",4);
		read_port(tmp,2);
		write_port("\x02\x02\x24\x02",4);
		read_port(rbuf,2);
		write_port("\x02\x02\x26\x02",4);
		read_port(tmp,2);
		
		// poke bytes into buffer
		if( start_addr == 0x00 )
			memcpy(buf,rbuf, len<2 ? 1 : 2 );
		if( start_addr == 0x01 )
			buf[0] = rbuf[1];
	}
}


/** Read ram or sfr
  * Read data from the internal data memory or from the SFR region
  * \param buf buffer to store the read data
  * \param start_addr address to begin reading from, 0x00 - 0xFF
  * \param len Number of bytes to read, 0x00 - 0xFF
  * \param sfr TRUE if you want to read a special function register, FALSE to read RAM
  */
void ec2_read_ram_sfr(  char *buf, int start_addr, int len, BOOL sfr )
{
	int i;
	char cmd[4];
	assert( (int)start_addr+len-1 <= 0xFF );	// RW -1 to allow reading 1 byte at 0xFF

	memset( buf, 0xff, len );	
	cmd[0] = sfr ? 0x02 : 0x06;
	cmd[1] = 0x02;
	for( i = 0; i<len; i+=0x0C )
	{
		cmd[2] = start_addr+i;
		cmd[3] = len-i >= 0x0C ? 0x0C : len-i;
		write_port( cmd, 0x04 );
		read_port( buf+i, cmd[3] );
	}
}

/** Write data into the micros RAM					<br>
  * cmd  07 addr len a b							<br>
  * len is 1 or 2									<br>
  * addr is micros data ram location				<br>
  * a  = first databyte to write					<br>
  * b = second databyte to write					<br>
  *
  * \param buf buffer containing dsata to write to data ram
  * \param start_addr address to begin writing at, 0x00 - 0xFF
  * \param len Number of bytes to write, 0x00 - 0xFF
  *
  * \returns TRUE on success, otherwise FALSE
  */
BOOL ec2_write_ram( char *buf, int start_addr, int len )
{
	int i, blen;
	char cmd[5];
	assert( start_addr>=0 && start_addr<=0xFF );

	// special case the first 2 bytes of RAM
	i=0;
	while( (start_addr+i)<=0x01 && ((len-i)>1) )
	{
		cmd[0] = 0x03;
		cmd[1] = 0x02;
		cmd[2] = 0x24 + start_addr+i;
		cmd[3] = buf[i];
		trx( cmd, 4, "\x0D", 1 );
		i++;
	}
	for( ; i<len; i+=2 )
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
			// single byte write but ec2 only does 2 byte writes correctly.
			// we read the affected bytes and change the first to our desired value
			// then write back
			cmd[0] = 0x07;
			cmd[1] = start_addr + i;
			cmd[2] = 0x02;			// two bytes
			ec2_read_ram(&cmd[3],start_addr+i,2);
			cmd[3] = buf[i];		// poke in desired value
			write_port( cmd, 5 );
		}
	}
}

/** write to targets XDATA address space			<BR>
  * Preamble... trx("\x03\x02\x2D\x01",4,"\x0D",1);	<BR>
  * <BR>
  * Select page address:							<BR>
  * trx("\x03\x02\x32\x00",4,"\x0D",1);				<BR>
  * cmd: 03 02 32 addrH								<BR>
  * where addrH is the top 8 bits of the address	<BR>
  * cmd : 07 addrL len a b							<BR>
  * addrL is low byte of address					<BR>
  * len is 1 of 2									<BR>
  * a is first byte to write						<BR>
  * b is second byte to write						<BR>
  * <BR>
  * closing :										<BR>
  * cmd 03 02 2D 00									<BR>
  *
  * \param buf buffer containing data to write to XDATA
  * \param start_addr address to begin writing at, 0x00 - 0xFFFF
  * \param len Number of bytes to write, 0x00 - 0xFFFF
  *
  * \returns TRUE on success, otherwise FALSE
  */
BOOL ec2_write_xdata( char *buf, int start_addr, int len )
{ 
	int addr, blen, page;
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len-1) >> 8 ) & 0xFF;
	unsigned int ofs=0;
	char start;
	unsigned int pg_start_addr, pg_end_addr;	// start and end addresses within page
	assert( start_addr>=0 && start_addr<=0xFFFF && start_addr+len<=0x10000 );
	
	for( page = start_page; page<=last_page; page++ )
	{
		pg_start_addr = (page==start_page) ? start_addr&0x00FF : 0x00;	
		pg_end_addr = (page==last_page) ? (start_addr+len-1)-(page<<8) : 0xff;
		blen = pg_end_addr - pg_start_addr + 1;	
//		printf("page = 0x%02x, start = 0x%04x, end = 0x%04x, len = %i, ofs=%04x\n", page,pg_start_addr, pg_end_addr,blen,ofs);
		ec2_write_xdata_page( buf+ofs, page, pg_start_addr, blen );
		ofs += blen;
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
	// up to 2 at a time
	for( i=0; i<len; i+=2 )
	{
		if( (len-i) > 1 )
		{
			cmd[0] = 0x07;
			cmd[1] = i+start;
			cmd[2] = 2;
			cmd[3] = (char)buf[i];
			cmd[4] = (char)buf[i+1];
			trx((char*)cmd,5,"\x0d",1);
		}
		else
		{
			// single byte write
			// although the EC2 responds correctly to 1 byte writes the SI labs
			// ide dosen't use them and attempting to use them does not cause a
			// write.  We fake a single byte write by reading in the byte that
			// will be overwitten and rewrite it 
			ec2_read_xdata(&cmd[3],(page<<8)+i+start,2);
			cmd[0] = 0x07;
			cmd[1] = i+start;
			cmd[2] = 2;					// length
			cmd[3] = (char)buf[i];		// overwrite first byte
			trx((char*)cmd,5,"\x0d",1);	// test
		}
	}
	trx("\x03\x02\x2D\x00",4,"\x0D",1);		// close xdata write session
	return TRUE;
}

/** Read len bytes of data from the target
  * starting at start_addr into buf
  *
  * T 03 02 2D 01  R 0D	<br>
  * T 03 02 32 addrH	<br>
  * T 06 02 addrL len	<br>
  * where len <= 0x0C	<br>
  *
  * \param buf buffer to recieve data read from XDATA
  * \param start_addr address to begin reading from, 0x00 - 0xFFFF
  * \param len Number of bytes to read, 0x00 - 0xFFFF
  */
void ec2_read_xdata( char *buf, int start_addr, int len )
{
	int addr, blen, page;
	char start_page	= ( start_addr >> 8 ) & 0xFF;
	char last_page	= ( (start_addr+len-1) >> 8 ) & 0xFF;
	unsigned int ofs=0;
	unsigned int pg_start_addr, pg_end_addr;	// start and end addresses within page
	
	assert( start_addr>=0 && start_addr<=0xFFFF && start_addr+len<=0x10000 );
	memset( buf, 0xff, len );
	for( page = start_page; page<=last_page; page++ )
	{
		pg_start_addr = (page==start_page) ? start_addr&0x00FF : 0x00;	
		pg_end_addr = (page==last_page) ? (start_addr+len-1)-(page<<8) : 0xff;
		blen = pg_end_addr - pg_start_addr + 1;	
//		printf("page = 0x%02x, start = 0x%04x, end = 0x%04x, len = %i\n", page,pg_start_addr, pg_end_addr,blen);
		ec2_read_xdata_page( buf+ofs, page, pg_start_addr, blen );
		ofs += blen;
	}
}

void ec2_read_xdata_page( char *buf, unsigned char page,
						  unsigned char start, int len )
{
	unsigned int i;
	unsigned char cmd[0x0C];

	memset( buf, 0xff, len );	
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
		buf += cmd[3];
	}
}

/** Read from Flash memory (CODE memory)
  *
  * \param buf buffer to recieve data read from CODE memory
  * \param start_addr address to begin reading from, 0x00 - 0xFFFF
  * \param len Number of bytes to read, 0x00 - 0xFFFF
  * \returns TRUE on success, otherwise FALSE
  */
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
	acmd[4] = addr & 0xFF;				// Little endian
	acmd[5] = (addr>>8) & 0xFF;			// patch in actual address
	trx( (char*)acmd, 7, "\x0D", 1 );	// first address write

	trx("\x0D\x05\x82\x08\x01\x00\x00",7,"\x0D",1);	// 82 flash control reg

	memset( buf, 0xff, len );

	for( i=0; i<len; i+=0x0C )
	{
		addr = start_addr + i;
		acmd[4] = addr & 0xFF;				// Little endian, flash address
		acmd[5] = (addr>>8) & 0xFF;			// patch in actual address
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
  *
  * Writes to a location that already contains data will only be successfull
  * in changing 1's to 0's.
  *
  * \param buf buffer containing data to write to CODE
  * \param start_addr address to begin writing at, 0x00 - 0xFFFF
  * \param len Number of bytes to write, 0x00 - 0xFFFF
  *
  * \returns TRUE on success, otherwise FALSE
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
	printf("ec2_write_flash( buf, 0x%04x, 0x%04x )\n",start_addr,len);
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

/** This variant of writing to flash memory (CODE space) will erase sectors
  * before writing.
  *
  * \param buf buffer containing data to write to CODE
  * \param start_addr address to begin writing at, 0x00 - 0xFFFF
  * \param len Number of bytes to write, 0x00 - 0xFFFF
  *
  * \returns TRUE on success, otherwise FALSE
  */
BOOL ec2_write_flash_auto_erase( char *buf, int start_addr, int len )
{
	int i;
	int end_addr = start_addr + len;
	int sector_cnt = ((end_addr-start_addr) >> 9) + 1;
	int first_sector = start_addr & 0xFE00;		// 512 byte sectors

	// Erase sectors involved
	for( i=0; i<sector_cnt; i++ )
	{
		printf("erasing sector 0x%04x\n",first_sector + i*0x200);
		ec2_erase_flash_sector( first_sector + i*0x200  );
	}
	ec2_write_flash( buf, start_addr, len );
}

/** This variant of writing to flash memory (CODE space) will read all sector
  * content before erasing and will merge changes over the existing data
  * before writing.
  * This is slower than the other methods in that it requires a read of the
  * sector first.  also blank sectors will not be erased again
  *
  * \param buf buffer containing data to write to CODE
  * \param start_addr address to begin writing at, 0x00 - 0xFFFF
  * \param len Number of bytes to write, 0x00 - 0xFFFF
  *
  * \returns TRUE on success, otherwise FALSE
  */
BOOL ec2_write_flash_auto_keep( char *buf, int start_addr, int len )
{
	int i,j;
	int end_addr = start_addr + len;
	int sector_cnt = ((end_addr-start_addr) >> 9) + 1;
	int first_sector = start_addr & 0xFE00;		// 512 byte sectors
	char tbuf[0x10000];
	
	// read in all sectors that are affected
	ec2_read_flash( tbuf, first_sector, sector_cnt*0x200 );

	// erase nonblank sectors
	for( i=0; i<sector_cnt; i++)
	{
		j = 0;
		while( j<0x200 )
		{
			if( (unsigned char)tbuf[i*0x200+j] != 0xFF )
			{
				// not blank, erase it
				ec2_erase_flash_sector(i*0x200);
				break;
			}
			j++;
		}
	}

	// merge data
	memcpy( tbuf+(start_addr-first_sector), buf, len );

	// write it now
	return ec2_write_flash( tbuf, first_sector, sector_cnt*0x200 );
}


/** Erase all CODE memory flash in the device
  */
void ec2_erase_flash()
{
	BOOL r=TRUE;
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

/** Erase a single sector of flash memory
  * \param sect_addr base address of sector to erase.  
  * 				Does not necessarilly have to be the base addreres but any
  *					address within the sector is equally valid
  *
  */
void ec2_erase_flash_sector( int sect_addr )
{
	char cmd[8];
	assert( sect_addr>=0 && sect_addr<=0xFFFF );
	sect_addr &= 0xFE00;								// 512 byte sectors
	
	trx("\x02\x02\xB6\x01",4,"\x80",1);
	trx("\x02\x02\xB2\x01",4,"\x14",1);
	trx("\x03\x02\xB2\x04",4,"\x0D",1);
	trx("\x0B\x02\x04\x00",4,"\x0D",1);
	trx("\x0D\x05\x82\x08\x20\x00\x00",7,"\x0D",1);
	memcpy(cmd,"\x0D\x05\x84\x10\x00\x00\x00",7);		// Address 0x0000
	cmd[4] = sect_addr & 0xFF;							// fill in actual address
	cmd[5] = (sect_addr>>8) & 0xFF;						// little endian
	trx(cmd,7,"\x0D",1);
	trx("\x0F\x01\xA5",3,"\x0D",1);
	
	// cleanup
	trx("\x0B\x02\x01\x00",4,"\x0D",1);
	trx("\x03\x02\xB6\x80",4,"\x0D",1);
	trx("\x03\x02\xB2\x14",4,"\x0D",1);
}

/** read the currently active set of R0-R7
  * the first returned value is R0
  * \note This needs more testing, seems to corrupt R0
  * \param buf buffer to reciere results, must be at least 8 bytes only
  */
void read_active_regs( char *buf )
{
	char b[8];
	char psw;
	int addr;
	// read PSW
	ec2_read_sfr( &psw, 0xD0 );
	printf( "PSW = 0x%02x\n",psw );

	// determine correct address
	addr = ((psw&0x18)>>3) * 8;
	printf("address = 0x%02x\n",addr);
	ec2_read_ram( buf, addr, 8 );

	// R0-R1
	write_port("\x02\x02\x24\x02",4);
	read_port(&buf[0],2);
}

/** Read the targets program counter
  *
  * \returns current address of program counter (16-bits)
  */
uint16_t ec2_read_pc()
{
	uint16_t		addr;
	unsigned char	buf[2];
	write_port("\x02\x02\x20\x02",4);
	read_port(buf,2);
	addr = (buf[1]<<8) | buf[0];
	return addr;
}

void ec2_set_pc( uint16_t addr )
{
	char cmd[4];
	cmd[0] = 0x03;
	cmd[1] = 0x02;
	cmd[2] = 0x20;
	cmd[3] = addr&0xFF;
	trx(cmd,4,"\x0D",1);
	cmd[2] = 0x21;
	cmd[3] = (addr>>8)&0xFF;
	trx(cmd,4,"\x0D",1);
}


/** Cause the processor to step forward one instruction
  * The program counter must be setup to point to valid code before this is
  * called. Once that is done this function can be called repeatedly to step
  * through code.
  * It is likely that in most cases the debugger will request register dumps
  * etc between each step but this function provides just the raw step
  * interface.
  * 
  * \returns instruction address after the step operation
  */
uint16_t ec2_step()
{
	char buf[2];
	uint16_t addr;
	trx("\x09\x00",2,"\x0d",1);
	trx("\x13\x00",2,"\x01",1);		// very similar to 1/2 a target_halt command
	
	write_port("\x02\x02\x20\x02",4);
	read_port(buf,2);
	addr = buf[0] | (buf[1]<<8);
	return addr;
}

/** Start the target processor running from the current PC location
  *
  * \returns TRUE on success, FALSE otherwise
  */
BOOL ec2_target_go()
{
	if( !trx("\x0B\x02\x00\x00",4,"\x0D",1) )
		return FALSE;
	if( !trx("\x09\x00",2,"\x0D",1) )
		return FALSE;
	return TRUE;
}

/** Poll the target to determine if the processor has halted.
  * The halt may be caused by a breakpoint of the ec2_target_halt() command.
  *
  * For run to breakpoint it is necessary to call this function regularly to
  * determine when the processor has actually come accross a breakpoint and
  * stopped.
  *
  * Recommended polling rate every 250ms.
  *
  * \returns TRUE if processor has halted, FALSE otherwise
  */
BOOL ec2_target_halt_poll()
{
	write_port("\x13\x00",2);
	return read_port_ch()==0x01;	// 01h = stopped, 00h still running
}

/** Cause target to run until the next breakpoint is hit.
  * \note this function will not return until a breakpoint it hit.
  * 
  * \returns Adderess of breakpoint at which the target stopped
  */
uint16_t ec2_target_run_bp()
{
	int i;
	ec2_target_go();
	trx("\x0C\x02\xA0\x10",4,"\x00\x01\x00",3);
	trx("\x0C\x02\xA1\x10",4,"\x00\x00\x00",3);
	trx("\x0C\x02\xB0\x09",4,"\x00\x00\x01",3);
	trx("\x0C\x02\xB1\x09",4,"\x00\x00\x01",3);
	trx("\x0C\x02\xB2\x0B",4,"\x00\x00\x20",3);
	
	// dump current breakpoints for debugging
	for( i=0; i<4;i++)
	{
		if( getBP( bpaddr[i] )!=-1 )
			printf("bpaddr[%i] = 0x%04x\n",i,(unsigned int)bpaddr[i]);
	}
	
	while( !ec2_target_halt_poll() )
		usleep(250000);
	return ec2_read_pc();
}

/** Request the target processor to stop
  * the polling is necessary to determine when it has actually stopped
  */
BOOL ec2_target_halt()
{
	int i;
	char ch;
	
	if( !trx("\x0B\x02\x01\x00",4,"\x0d",1) )
		return FALSE;
	
	// loop allows upto 8 retries 
	// returns 0x01 of successful stop, 0x00 otherwise suchas already stopped	
	for( i=0; i<8; i++ )
	{
		if( ec2_target_halt_poll() )
			return TRUE;	// success
	}
	printf("ERROR: target would not stop after halt!\n");
	return FALSE;
}


/** Rest the target processor
  * This reset is a cut down form of the one used by the IDE which seems to 
  * read 2 64byte blocks from flash as well.
  * \todo investigate if the additional reads are necessary
  */
BOOL ec2_target_reset()
{
	BOOL r = TRUE;
	r &= trx("\x04",1,"\x0D",2);
	r &= trx("\x1A\x06\x00\x00\x00\x00\x00\x00",8,"\x0D",1);
	r &= trx("\x0B\x02\x02\x00",4,"\x0D",1);
	r &= trx("\x14\x02\x10\x00",4,"\x04",1);
	r &= trx("\x16\x02\x01\x20",4,"\x01\x00",2);
	r &= trx("\x14\x02\x10\x00",4,"\x04",1);
	r &= trx("\x16\x02\x81\x20",4,"\x01\x00",2);
	r &= trx("\x14\x02\x10\x00",4,"\x04",1);
	r &= trx("\x16\x02\x81\x30",4,"\x01\x00",2);
	r &= trx("\x15\x02\x08\x00",4,"\x04",1);
	r &= trx("\x16\x01\xE0",3,"\x00",1);
	r &= trx("\x0B\x02\x01\x00",4,"\x0D",1);
	r &= trx("\x13\x00",2,"\x01",1);
	r &= trx("\x03\x02\x00\x00",4,"\x0D",1);
	return r;
}



///////////////////////////////////////////////////////////////////////////////
// Breakpoint support                                                        //
///////////////////////////////////////////////////////////////////////////////

/** Reset all breakpoints
  */
static int resetBP(void)
{
	int bp;
	for( bp=0; bp<4; bp++ )
		setBpMask( bp, FALSE );
}

/** Determine if there is a free breakpoint and then returning its index
  * \returns the next available breakpoint index, -1 on failure
 */
static int getNextBPIdx(void)
{
	int i;
	
	for( i=0; i<4; i++ )
	{
		if( !(bp_flags>>i)&0x01 )
			return i;				// not used, well take it
	}
	return -1;						// no more available
}

/** Get the index of the breakpoint for the specified address
  * \returns index of breakpoint matching supplied address or -1 if not found
  */
static int getBP( uint16_t addr )
{
	int i;

	for( i=0; i<4; i++ )
		if( (bpaddr[i]==addr) && ((bp_flags>>i)&0x01) )
			return i;

	return -1;	// No active breakpoints with this address
}

// Modify the bp mask approprieatly and update EC2
/** Update both our local and the EC2 bp mask byte
  * \param bp		breakpoint number to update
  * \param active	TRUE set that bp active, FALSE to disable
  * \returns		TRUE = success, FALSE=failure
  */
static BOOL setBpMask( int bp, BOOL active )
{
	char cmd[7];

	if( active )
		bp_flags |= ( 1 << bp );
	else
		bp_flags &= ~( 1 << bp );
	cmd[0] = 0x0D;
	cmd[1] = 0x05;
	cmd[2] = 0x86;
	cmd[3] = 0x10;
	cmd[4] = bp_flags;
	cmd[5] = 0x00;
	cmd[6] = 0x00;
	if( trx(cmd,7,"\x0D",1) )	// inform EC2
		return TRUE;
	else
		return FALSE;
}

/** Add a new breakpoint using the first available breakpoint
  */
BOOL ec2_addBreakpoint( uint16_t addr )
{
	char cmd[7];
	int bp;
	if( getBP(addr)==-1 )	// check address doesn't already have a BP
	{
		bp = getNextBPIdx();
		if( bp!=-1 )
		{
			// set address
			bpaddr[bp] = addr;
			cmd[0] = 0x0D;
			cmd[1] = 0x05;
			cmd[2] = 0x90+bp;	// Breakpoint address register to write
			cmd[3] = 0x10;
			cmd[4] = addr & 0xFF;
			cmd[5] = (addr>>8) & 0xFF;
			cmd[6] = 0x00;
			if( !trx(cmd,7,"\x0D",1) )
				return FALSE;
			return setBpMask( bp, TRUE );
		}
		else
			return FALSE;
	}
	else
		return FALSE;
}

BOOL ec2_removeBreakpoint( uint16_t addr )
{
	int16_t bp = getBP( addr );
	if( bp != -1 )
		return setBpMask( bp, FALSE );
	else
		return FALSE;
}


/**  Write the data pointed to by image into the flash memory of the EC2
  * \param image	buffer containing the firmware image.
  * \param len		Length of the image in bytes (shoulden't ever change)
  */
BOOL ec2_write_firmware( char *image, uint16_t len )
{
	int i;
	char cmd[4];
	BOOL r;
	// defines order of captured blocks...
	const char block_order[] = 
	{ 
		0x0E,0x09,0x0D,0x05,0x06,0x0A,0x08,
		0x0C,0x0B,0x07,0x04,0x0F,0x02,0x03
	};
	
	ec2_reset();
	trx("\x55",1,"\x5A",1);
	for(i=0; i<14;i++)
	{
		cmd[0] = 0x01;
		cmd[1] = block_order[i];
		cmd[2] = 0x00;
		trx(cmd,3,"\x00",1);
		trx("\x02\x00\x00",3,"\x00",1);
		trx("\x03\x02\x00",3,"\x00",1);
		trx( image+(i*0x200), 0x200,"\x00",1);
		write_port("\x04\x00\x00",3);
		read_port(cmd,2);
//		printf("CRC = %02x%02x\n",(unsigned char)cmd[0],(unsigned char)cmd[1]);
	}
	ec2_reset();
	r = trx("\x55",1,"\x5a",1);
	ec2_reset();
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
	resetBP();
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
	// The EC2 doesn't like back to back writes!
	// have to slow things down a tad
	for( i=0; i<len; i++ )
	{
		write( m_fd, buf+i, 1);
		usleep(4000);			// 4ms inter character delay
	}
#ifdef EC2TRACE
	printf("TX: ");
	print_buf( buf, len );
#endif	
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
    timeout.tv_sec  = 2;		// n seconds timeout
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
#ifdef EC2TRACE
		printf("RX: ");
		print_buf( buf, len );
#endif	
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

static void print_buf( char *buf, int len )
{
	while( len-- !=0 )
		printf("%02x ",(unsigned char)*buf++);
	printf("\n");
}
