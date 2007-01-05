/**	boot.c
	Provides routines for using the bootloader in the EC2 / EC3
	(C) Ricky White 2007
	Liscence GPL V2
 */
#include <string.h>
#include <stdint.h>
#include "boot.h"

/** Run the main debugger application
 */
void boot_run_app( EC2DRV *obj )
{
	trx(obj,"\x06\x00\x00",3,"\x00",1);
}

/** Get the version
 */
uint8_t boot_get_version( EC2DRV *obj )
{
	write_port(obj,"\x00\x00\x00",3);
	return read_port_ch(obj);
}


/** Erase the currently addressed sector in the debugger
 */
void boot_erase_flash_page( EC2DRV *obj )
{
	trx(obj,"\x02\x00\x00",3,"\x00",1);
}


/** Select the active flash page
 */
void boot_select_flash_page( EC2DRV *obj, uint8_t page_num )
{
	char buf[3];
	buf[0] = 1;			// set sector
	buf[1] = page_num;
	buf[2] = 0;
	trx(obj,buf,3,"\x00",1);
}

/** Write a page of data into the debugger (active page)
	The buffer must contain full page data
 */
void boot_write_flash_page( EC2DRV *obj, uint8_t *buf, BOOL do_xor )
{
	const uint16_t page_size = 512;
	char out_buf[page_size];
	trx(obj,"\x03\x00\x00",3,"\x00",1);
	
	if(do_xor)
	{
		int i;
		for(i=0;i<page_size;i++)
			out_buf[i] = buf[i] ^ 0x55;
	}
	else
		memcpy(out_buf,buf,page_size);
	write_port(obj,out_buf,512);
	read_port_ch(obj);			// 0x00
}


/** read a byte from the debuggers code memory
	There sseems to be a problem once we have read 3d bytes.
 */
uint8_t boot_read_byte( EC2DRV *obj, uint16_t addr )
{
	char cmd[3];
	cmd[0] = 0x05;	// read
	cmd[1] = addr & 0xff;
	cmd[2] = (addr>>8) & 0xff;
	write_port(obj,cmd,3);
	return read_port_ch(obj);
}


/** Calculate the CRC for the active page and return it
	this value is calculated in the debugger and can be compared to a local copy
	to ensure the firmware was transfered correctly.
*/
uint16_t boot_calc_page_cksum( EC2DRV *obj )
{
	char buf[2];
	write_port(obj,"\x04\x00\x00",3);
	read_port(obj,buf,2);
	return buf[0] | (buf[1]<<8);
}
