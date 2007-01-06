#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "c2_mode.h"


/** Connect to a device using C2 mode.
*/
void c2_connect_target( EC2DRV *obj )
{
	trx(obj,"\x20",1,"\x0d",1);
}

void c2_disconnect_target( EC2DRV *obj )
{
	trx(obj,"\x21",1,"\x0d",1);
}


uint16_t c2_device_id( EC2DRV *obj )
{
	char buf[2];
	// this appeared in new versions of IDE but seems to have no effect for F310	
	// EC2 chokes on this!!!!		trx(obj,"\xfe\x08",2,"\x0d",1);
	write_port( obj,"\x22", 1 );	// request device id (C2 mode)
	read_port( obj, buf, 2 );
	return buf[0]<<8 | buf[1];
}

uint16_t c2_unique_device_id( EC2DRV *obj )
{
	char buf[3];
//	ec2_target_halt(obj);	// halt needed otherwise device may return garbage!
	write_port(obj,"\x23",1);
	read_port(obj,buf,3);
//	print_buf( buf,3);
//	ec2_target_halt(obj);	// halt needed otherwise device may return garbage!
	// test code
	trx(obj,"\x2E\x00\x00\x01",4,"\x02\x0D",2);
	trx(obj,"\x2E\xFF\x3D\x01",4,"xFF",1);
	return buf[1];
}


void c2_erase_flash( EC2DRV *obj )
{
	int i;
	// generic C2 erase entire device
	// works for EC2 and EC3
		
	// FIXME the disconnect / connect sequence dosen't work with the EC2 and C2 mode!
	if( obj->dbg_adaptor==EC3 )
	{
		ec2_disconnect( obj );
		ec2_connect( obj, obj->port );
	}
	write_port( obj, "\x3C",1);			// Erase entire device
	read_port_ch(obj);					// 0x0d
	if( obj->dbg_adaptor==EC3 )
	{
		ec2_disconnect( obj );
		ec2_connect( obj, obj->port );
	}
}

BOOL c2_erase_flash_sector( EC2DRV *obj, uint32_t sector_addr,
							BOOL scratchpad )
{
	char cmd[2];
	cmd[0] = 0x30;		// sector erase command
	cmd[1] = sector_addr/ obj->dev->flash_sector_size;
	return trx( obj, cmd, 2, "\x0d", 1 );
}

/*  C2 version of ec2_write_flash
*/
BOOL c2_write_flash( EC2DRV *obj, uint8_t *buf, uint32_t start_addr, int len )
{
	DUMP_FUNC();
//	if(!check_flash_range( obj, start_addr, len )) return FALSE;
	// preamble
	// ...
	// 2f connect breakdown:
	// T 2f 00 30 08 55 55 55 55 55 55 55 55		R 0d
	//    |  |  |  | +---+--+--+--+--+--+--+--- Data bytes to write
	//    |  |  |  +--------------------------- number of data bytes towrite (8 max, maxtotal cmd length 0x0c)
	//    |  |  +------------------------------ High byte of address to start write
	//    |  +--------------------------------- low byte of address
	//    +------------------------------------ write code memory command
	//
	// for some funny reason the IDE alternates between 8 byte writes and 
	// 4 byte writes, this means the total number of writes cycle is 0x0c the
	// exact same number as the JTAG mode, it looks like they economise code,
	// I this would complicate things, well just do 8 byte writes and then an
	// fragment at the end.  This will need testing throughlerly as it is
	// different to the IDE's action.
	unsigned int i, addr;
	char		 cmd[0x0c];

	cmd[0] = 0x2f;									// Write code/flash memory cmd	
	for( i=0; i<len; i+=8 )
	{
		addr = start_addr + i;
		cmd[1] = addr & 0xff;						// low byte
		cmd[2] = (addr>>8) & 0xff;					// high byte
		cmd[3] = (len-i)<8 ? (len-i) : 8;
		memcpy( &cmd[4], &buf[i], cmd[3] );
		if( !trx( obj, cmd, cmd[3]+4, "\x0d", 1 ) )
			return FALSE;							// Failure
	}
	return TRUE;
}


BOOL c2_read_flash( EC2DRV *obj, uint8_t *buf, uint32_t start_addr, int len )
{
	int i;
	uint8_t cmd[10];
	uint32_t addr;
	// C2 mode is much simpler
	//
	// example command 0x2E 0x00 0x00 0x0C
	//				     |    |   |    |
	//					 |    |   |    +---- Length or read up to 0x0C bytes
	//					 |    |   +--------- High byte of address to start at
	//					 |    +----(len-i)--------- Low byte of address
	//					 +------------------ Flash read command
	cmd[0] = 0x2E;
	for( i=0; i<len; i+=0x0c )
	{
		addr = start_addr + i;
		cmd[1] = addr & 0xff;	// low byte
		cmd[2] = (addr >> 8) & 0xff;
		cmd[3] = (len-i) > 0x0c ? 0x0c : (len-i);
		write_port( obj, (char*)cmd, 4 );
		read_port( obj, (char*)buf+i, cmd[3] ); 
	}
	return TRUE;
}


/** Read from xdata memory on chips that have external memory interfaces and C2

	\param obj			Object to act on.
	\param buf			Buffer to recieve data read from XDATA
	\param start_addr	Address to begin reading from, 0x00 - 0xFFFF
	\param len			Number of bytes to read, 0x00 - 0xFFFF
	
	\returns			TRUE on success, otherwise FALSE
*/
BOOL c2_read_xdata_emif( EC2DRV *obj, char *buf, int start_addr, int len )
{
	// Command format
	//	T 3e LL HH NN
	// where
	//		LL = Low byte of address to start reading from
	//		HH = High byte of address to start reading from
	//		NN = Number of  bytes to read, max 3c for EC3, max 0C? for EC2
	assert( obj->mode==C2 );
	assert( obj->dev->has_external_bus );
	uint16_t block_len_max = obj->dbg_adaptor==EC2 ? 0x0C : 0x3C;
	// read  blocks of upto max block  len
	uint16_t addr = start_addr;
	uint16_t cnt = 0;
	uint16_t block_len;
	char cmd[4];
	while( cnt < len )
	{
		// request the block
		cmd[0] = 0x3e;					// Read EMIF
		cmd[1] = addr & 0xff;			// Low byte
		cmd[2] = (addr & 0xff00) >> 8;	// High byte
		block_len = (len-cnt)>block_len_max ? block_len_max : len-cnt;
		cmd[3] = block_len;
		write_port( obj, cmd, 4 );
		read_port( obj, buf+cnt, block_len+1 );	// +1 for 0x0d terminator ///@FIXME do we need to read one extra here for 0x0d?
		addr += block_len;
		cnt += block_len;
	}
	return TRUE;
}



/** write to targets XDATA address space (C2 with internal + external xdata).
	c2 xdata write with emif.

	\param obj			Object to act on.
	\param buf			Buffer containing data to write to XDATA
	\param start_addr	Address to begin writing at, 0x00 - 0xFFFF
	\param len			Number of bytes to write, 0x00 - 0xFFFF
	
	\returns			TRUE on success, otherwise FALSE
*/
BOOL c2_write_xdata_emif( EC2DRV *obj, char *buf, int start_addr, int len )
{
	// Command format
	// data upto 3C bytes, last byte is in a second USB transmission with its own length byte
	// 3f 00 00 3c 5a
	// 3f LL HH NN
	// where
	//		LL = Low byte of address to start writing at.
	//		HH = High byte of address to start writing at.
	//		NN = Number of  bytes to write, max 3c for EC3, max 0C? for EC2	
	
	// read back result of 0x0d
	assert( obj->mode==C2 );
	assert( obj->dev->has_external_bus );	
	uint16_t block_len_max = obj->dbg_adaptor==EC2 ? 0x0C : 0x3C;
	uint16_t block_len;
	uint16_t addr = start_addr;
	uint16_t cnt = 0;
	char cmd[64];
	const char cmd_len = 4;
	BOOL ok=TRUE;
	while( cnt<len )
	{
		cmd[0] = 0x3f;	// Write EMIF
		cmd[1] = addr&0xff;
		cmd[2] = (addr&0xff00)>>8;
		block_len = (len-cnt)>block_len_max ? block_len_max : len-cnt;
		cmd[3] = block_len;
		//memcpy( &cmd[4], buf+addr, block_len );
		memcpy( &cmd[4], buf+cnt, block_len );
		if( block_len==0x3c )
		{
			// split write over 2 USB writes
			write_port( obj, cmd, 0x3f );
			write_port( obj, &cmd[cmd_len+0x3b], 1 );
			ok |= (read_port_ch( obj)=='\x0d');
		}
		else
		{
			write_port( obj, cmd, block_len + cmd_len );
			ok |= (read_port_ch( obj)=='\x0d');
		}
		addr += block_len;
		cnt += block_len;
	}
	return ok;
}


void c2_read_ram( EC2DRV *obj, char *buf, int start_addr, int len )
{
	char tmp[4];
	assert(obj->mode==C2);
	
	ec2_read_ram_sfr( obj, buf, start_addr, len, FALSE );
		/// \TODO we need to do similar to above, need to check out if there is a generic way to handle
		/// the first 2 locations,  should be since the same reason the the special case and sfr reads
		/// are already JTAG / C2 aware.
		// special case, read first 3 bytes of ram
		//T 28 24 02		R 7C 00
		//T 28 26 02		R 00 00
	write_port( obj,"\x28\x24\x02",3 );
	read_port( obj, &tmp[0], 2);
	write_port( obj,"\x28\x26\x02",3 );
	read_port( obj, &tmp[2], 2);
	if( start_addr<3 )
	{
		memcpy( &buf[0], &tmp[start_addr], 3-start_addr );
	}
}


void c2_read_ram_sfr( EC2DRV *obj, char *buf, int start_addr, int len, BOOL sfr )
{
	int i;
	uint8_t cmd[40];
	char block_len = obj->dbg_adaptor==EC2 ? 0x0c : 0x3b;
	memset( buf, 0xff, len );
	cmd[0] = sfr ? 0x28 : 0x2A;		// SFR read or RAM read
	for( i = 0; i<len; i+=block_len )
	{
		cmd[1] = start_addr+i;
		cmd[2] = len-i >= block_len ? block_len : len-i;
		write_port( obj, (char*)cmd, 3 );
		read_port( obj, buf+i, cmd[2] );
	}
}


/** Write data into the micros DATA RAM (C2).
	\param obj			Object to act on.	
	\param buf			Buffer containing dsata to write to data ram
	\param start_addr	Address to begin writing at, 0x00 - 0xFF
	\param len			Number of bytes to write, 0x00 - 0xFF
	
	\returns 			TRUE on success, otherwise FALSE
 */
BOOL c2_write_ram( EC2DRV *obj, char *buf, int start_addr, int len )
{
	// special case for first 2 bytes, related to R0 / R1 I think
	// special case the first 2 bytes of RAM
	// looks like it should be the first 3 bytes
	assert(obj->mode==C2);
	int i=0;
	char cmd[5], tmp[2];
	
	while( (start_addr+i)<=0x02 && ((len-i)>=1) )
	{
		cmd[0] = 0x29;
		cmd[1] = 0x24+start_addr+i;
		cmd[2] = 0x01;	// len
		cmd[3] = buf[i];
		trx( obj, cmd, 4, "\x0D", 1 );
		i++;
	}
		// normal writes
		// 0x2b 0x03 0x02 0x55 0x55
		//  |    |    |    |    |
		//	|    |    |    |    +--- Seconds data byte
		//	|    |    |    +-------- Seconds data byte
		//	|    |    +------------- Number of bytes, must use 2 using 1 or >2 won't work!
		//	|    +------------------ Start address
		//	+----------------------- Data ram write command
	for( ; i<len; i+=2 )
	{
		cmd[0] = 0x2b;				// write data RAM
		cmd[1] = start_addr + i;	// address
		int blen = len-i;
		if( blen>=2 )
		{
			cmd[2] = 0x02;			// two bytes
			cmd[3] = buf[i];
			cmd[4] = buf[i+1];
			trx( obj, cmd, 5, "\x0d", 1 );
		}
		else
		{
				// read back, poke in byte and call write for 2 bytes
			if( (start_addr+i) == 0xFF )
			{
				// must use previous byte
				ec2_read_ram( obj, tmp, start_addr+i-1, 2 );
				tmp[1] = buf[i];	// databyte to write at 0xFF
				ec2_write_ram( obj, tmp, start_addr+i-1, 2 );
			}
			else
			{
					// use following byte
				ec2_read_ram( obj, tmp, start_addr+i, 2 );
				tmp[0] = buf[i];	// databyte to write
				ec2_write_ram( obj, tmp, start_addr+i, 2 );
			}
		}
	}
	return TRUE;
}


/** write to targets XDATA address space (C2 with only internal xdata).
	Normal c2 xdata write without emif.

	\param obj			Object to act on.
	\param buf			Buffer containing data to write to XDATA
	\param start_addr	Address to begin writing at, 0x00 - 0xFFFF
	\param len			Number of bytes to write, 0x00 - 0xFFFF
	
	\returns			TRUE on success, otherwise FALSE
*/
BOOL c2_write_xdata( EC2DRV *obj, char *buf, int start_addr, int len )
{
	assert( obj->mode==C2);
	if( obj->dev->has_external_bus )
		return c2_write_xdata_emif( obj, buf, start_addr, len );
	
	// T 29 ad 01 00	R 0d
	// T 29 c7 01 00	R 0d
	// T 29 84 01 55	R 0d	// write 1 byte (0x55) at the curr addr then inc that addr
	char cmd[4];
	unsigned int i;
	
	// low byte of start address
	cmd[0] = 0x29;
	cmd[1] = 0xad;
	cmd[2] = 0x01;		// length
	cmd[3] = start_addr & 0xff;
	trx( obj, cmd, 4, "\x0d", 1 );
	
	// high byte of start address
	cmd[0] = 0x29;
	cmd[1] = 0xc7;
	cmd[2] = 0x01;		// length
	cmd[3] = (start_addr >> 8)&0xff;
	trx( obj, cmd, 4, "\x0d", 1 );
		
	// setup write command
	cmd[0] = 0x29;
	cmd[1] = 0x84;
	cmd[2] = 0x01;	// len, only use 1
	for(i=0; i<len; i++)
	{
		cmd[3] = buf[i];
		if( !trx( obj, cmd, 4, "\x0d", 1 ) )
			return FALSE;	// failure
	}
	return TRUE;
}


/** Read len bytes of data from the target (C2)

	\param obj			Object to act on.
	\param buf			Buffer to recieve data read from XDATA
	\param start_addr	Address to begin reading from, 0x00 - 0xFFFF
	\param len			Number of bytes to read, 0x00 - 0xFFFF
	
	\returns			TRUE on success, otherwise FALSE
*/
BOOL c2_read_xdata( EC2DRV *obj, char *buf, int start_addr, int len )
{
	// C2 dosen't seem to need any paging like jtag mode does
	char cmd[4];
	unsigned int i;
	
	if( obj->dev->has_external_bus )
		return c2_read_xdata_emif( obj, buf, start_addr, len );
	
	// code below is for C2 devices that don't have externam memory.
	// T 29 ad 01 10	R 0d	.// low byte of address 10 ( last byte of cmd)
	// T 29 c7 01 01	R 0d	//  high byte of address 01 ( last byte of cmd)
	// T 28 84 01		R 00	// read next byte (once for every byte to be read)

	// low byte of start address
	cmd[0] = 0x29;
	cmd[1] = 0xad;
	cmd[2] = 0x01;		// length
	cmd[3] = start_addr & 0xff;
	trx( obj, cmd, 4, "\x0d", 1 );
		// high byte of start address
	cmd[0] = 0x29;
	cmd[1] = 0xc7;
	cmd[2] = 0x01;		// length
	cmd[3] = (start_addr >> 8)&0xff;
	trx( obj, cmd, 4, "\x0d", 1 );
		
		// setup read command
	cmd[0] = 0x28;
	cmd[1] = 0x84;
	cmd[2] = 0x01;
	for(i=0; i<len; i++)
	{
		write_port( obj, cmd, 3 );
		buf[i] = read_port_ch( obj ); 
		read_port_ch( obj );	// look for 0x0d terminator
	}
	return TRUE;
}



void c2_write_sfr( EC2DRV *obj, uint8_t value, uint8_t addr )
{
	char cmd[4];

	cmd[0] = 0x29;
	cmd[1] = addr;
	cmd[2] = 0x01;
	cmd[3] = value;
	trx( obj,cmd,4,"\x0D",1 );
}