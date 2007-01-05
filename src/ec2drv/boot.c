/**	boot.c
	Provides routines for using the bootloader in the EC2 / EC3
	(C) Ricky White 2007
	Liscence GPL V2
 */
#include <string.h>
#include <stdint.h>
#include "boot.h"

/** Run the main debugger application.
	\returns the version of the firmware currently loaded into the debugger.
 */
uint8_t boot_run_app( EC2DRV *obj )
{
	DUMP_FUNC();
	uint8_t fw_ver;
	write_port(obj,"\x06\x00\x00",3);
	fw_ver = read_port_ch(obj);
	DUMP_FUNC_END();
	return fw_ver;
}

/** Get the version of the bootloader
	known versions are 2 for ec3 and 3 for ec2.
 */
uint8_t boot_get_version( EC2DRV *obj )
{
	DUMP_FUNC();
	write_port(obj,"\x00\x00\x00",3);
	obj->boot_ver = read_port_ch(obj);
	DUMP_FUNC_END();
	return obj->boot_ver;
}


/** Erase the currently addressed sector in the debugger
 */
void boot_erase_flash_page( EC2DRV *obj )
{
	DUMP_FUNC();
	trx(obj,"\x02\x00\x00",3,"\x00",1);
	DUMP_FUNC_END();
}


/** Select the active flash page
 */
void boot_select_flash_page( EC2DRV *obj, uint8_t page_num )
{
	DUMP_FUNC();
	char buf[3];
	buf[0] = 1;			// set sector
	buf[1] = page_num;
	buf[2] = 0;
	trx(obj,buf,3,"\x00",1);
	DUMP_FUNC_END();
}

/** Write a page of data into the debugger (active page)
	The buffer must contain full page data
 */
void boot_write_flash_page( EC2DRV *obj, uint8_t *buf, BOOL do_xor )
{
	DUMP_FUNC();
	const uint16_t page_size = 512;
	char out_buf[page_size];
	char tmp[2];
	trx(obj,"\x03\x00\x00",3,"\x00",1);
	
	if(do_xor)
	{
		int i;
		for(i=0;i<page_size;i++)
			out_buf[i] = buf[i] ^ 0x55;
	}
	else
		memcpy(out_buf,buf,page_size);
	
	if(obj->dbg_adaptor==EC3)
	{
//		write_port(obj,out_buf,512);
		// write the data block
		// 8 * 63 byte blocks
		// + 1 * 8 byte block
		int k;
		uint16_t offset=0;
		for(k=0; k<8; k++, offset+=63 )
			write_port( obj, (char*)buf+offset, 63 );
		// now the 8 left over bytes 
		write_port( obj, (char*)buf+offset, 8 );
		read_port(obj, tmp, 2);
		offset +=8;
	}
	else if(obj->dbg_adaptor==EC2)
	{
		write_port(obj,out_buf,512);
	}
	read_port_ch(obj);			// 0x00
	DUMP_FUNC_END();
}


/** read a byte from the debuggers code memory
	There sseems to be a problem once we have read 3d bytes.
 */
uint8_t boot_read_byte( EC2DRV *obj, uint16_t addr )
{
	DUMP_FUNC();
	char cmd[3];
	uint8_t c;
	cmd[0] = 0x05;	// read
	cmd[1] = addr & 0xff;
	cmd[2] = (addr>>8) & 0xff;
	write_port(obj,cmd,3);
	c = read_port_ch(obj);
	DUMP_FUNC_END();
	return c;
}


/** Calculate the CRC for the active page and return it
	this value is calculated in the debugger and can be compared to a local copy
	to ensure the firmware was transfered correctly.
*/
uint16_t boot_calc_page_cksum( EC2DRV *obj )
{
	DUMP_FUNC();
	char buf[2];
	uint16_t cksum;
	write_port(obj,"\x04\x00\x00",3);
	read_port(obj,buf,2);
	cksum = buf[0] | (buf[1]<<8);
	DUMP_FUNC();
	return cksum;
}
