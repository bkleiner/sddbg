#include "jtag_mode.h"



BOOL jtag_flashcon( EC2DRV *obj, uint8_t value)
{
	//T 0d 05 82 08 01 00 00	R 0d	code
	//			    81					scratchpad
	return jtag_write_IR( obj, 0x82, 8, (uint32_t)value );
}


BOOL jtag_halt( EC2DRV *obj )
{
	return trx( obj, "\x0B\x02\x01\x00", 4, "\x0d", 1 );
}


// These registers for the F120
extern SFRREG SFR_SFRPAGE;
extern SFRREG SFR_FLSCL;
extern SFRREG SFR_CCH0LC;
extern SFRREG SFR_OSCICN;
extern SFRREG SFR_CLKSEL;
extern SFRREG SFR_CCH0CN;

/** Write an entire flash sector.
	NOTE this function does not erase the sector first.
	\param obj	ec2drv object to act on
	\param buf	Buffer containing data to write, must contain a full sectors worth of data.
	\returns TRUE on success, FALSE on failure
*/
BOOL jtag_write_flash_sector( EC2DRV *obj, uint32_t sect_addr, uint8_t *buf,
							  BOOL scratchpad )
{
	DUMP_FUNC();
	uint8_t max_block_len = (obj->dbg_adaptor==EC2) ? 0x0c : 0x3F;
	BOOL result = TRUE;
//	printf("jtag_write_flash_sector(...)    addr = 0x%05x\n",sect_addr);
	jtag_flashcon( obj, 0x00 );
	trx( obj, "\x0b\x02\x01\x00",4,"\x0d",1);
	
	ec2_write_paged_sfr( obj, SFR_FLSCL, 0x80 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0xc0 );
	ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x3e );
	ec2_write_paged_sfr( obj, SFR_CCH0CN, 0x07 );	// Cache Lock
	
	trx( obj, "\x0b\x02\x04\x00",4,"\x0d",1);
	trx( obj, "\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);
	trx( obj, "\x0d\x05\xc2\x07\x47\x00\x00",7,"\x0d",1);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0B\x02\x01\x00", 4, "\x0D", 1 );	
	
//	printf("Cache control\n");
	// Cache control
	if( obj->dev->has_cache )
		ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x00 );	// Cache Lock
	
//	printf("Oscilator config\n");
	// oscilator config
	uint8_t cur_oscin = ec2_read_paged_sfr( obj, SFR_OSCICN,  0 );
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0x80 );
	
//	printf("ClkSel\n");
	uint8_t cur_clksel = ec2_read_paged_sfr( obj, SFR_CLKSEL, 0 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_core_suspend(obj);
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x90 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0xD6 );	

	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	uint32_t sect_start_addr;
	uint32_t sect_end_addr;
	uint16_t sector_size;
	
	if(!scratchpad)
	{
		// Normal case write to code memory
		
		// F120 does this... T 0d 05 80 12 02 28 00	R 0d	// what is JTAG IR = 0x80?
		trx(obj,"\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);	// without this writes fail on F120
		
		sector_size = obj->dev->flash_sector_size;
		sect_start_addr = sect_addr / obj->dev->flash_sector_size;
		sect_start_addr *= obj->dev->flash_sector_size;
		sect_end_addr = sect_start_addr+obj->dev->flash_sector_size-1;
	}
	else
	{
		// F120 does this... T 0d 05 80 12 02 28 00	R 0d	// what is JTAG IR = 0x80?
		trx(obj,"\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);	// without this writes fail on F120
		
		sector_size = obj->dev->scratchpad_sector_size;
		sect_start_addr = sect_addr / obj->dev->scratchpad_sector_size;
		sect_start_addr *= obj->dev->scratchpad_sector_size;
		sect_end_addr = sect_start_addr+obj->dev->scratchpad_sector_size-1;
	}
	
	// begin actual write now
//	printf("sector = [0x%05x,0x%05x]\n",sect_start_addr, sect_end_addr );
	if(scratchpad)
		jtag_flashcon( obj, 0xa0 );
	else
		jtag_flashcon( obj, 0x20 );
	
	set_flash_addr_jtag( obj, sect_start_addr );	
	// erase flash
	trx( obj, "\x0f\x01\xa5", 3, "\x0d", 1 );
	if(scratchpad)
		jtag_flashcon( obj, 0x82 );
	else
		jtag_flashcon( obj, 0x02 );
	trx( obj, "\x0e\x00", 2, "\xa5", 1 );	// ???
	trx( obj, "\x0e\x00", 2, "\xff", 1 );	// ???
	
	if(scratchpad)
		jtag_flashcon( obj, 0x90 );
	else
		jtag_flashcon( obj, 0x10 );
	set_flash_addr_jtag( obj, sect_start_addr );
	
	uint32_t addr = sect_start_addr;
	uint8_t tbuf[0xff];
	if( obj->dbg_adaptor==EC2 )
	{
		while( (sect_end_addr-addr) >= max_block_len )
		{
//			printf("Fragment start_addr = 0x%05x, len = 0x%04x\n",addr,max_block_len );
			memcpy(tbuf,"\x12\x02\x0c\x00",4);
			memcpy(tbuf+4,buf,max_block_len);
			write_port(obj,tbuf,max_block_len+4);
			result |= read_port_ch(obj)==0x0d;
			addr += max_block_len;
			buf += max_block_len;
		}
		if( (sect_end_addr-addr)>0 )
		{
			uint8_t len = (sect_end_addr-addr)+1;
//			printf("Fragment start_addr = 0x%05x, len = 0x%04x\n",addr,len );
			// mop up wats left, it had better be a multiple of 4 for the F120 and start on the correct offset
			tbuf[0] = 0x12;
			tbuf[1] = 0x02;
			tbuf[2] = len;
			tbuf[3] = 0x00;
			memcpy(&tbuf[4],buf,len);
			write_port(obj, tbuf,4+len);
			result &= read_port_ch( obj )==0x0d;
		}
	}
	else
	{
		// EC3 does it differently.
		// tell the EC2 a sectors worth of data is comming
		tbuf[0] = 0x12;
		tbuf[1] = 0x02;
		tbuf[2] = sector_size & 0xff;
		tbuf[3] = sector_size>>8 & 0xff;
		result &= trx(obj,tbuf,4,"\x0d",1);
		// write in the chunks now
		uint16_t offset=0;
		offset=0;
		for( offset=0; offset<sector_size; offset+=max_block_len )
		{
			uint16_t blk = sector_size-offset>max_block_len ? max_block_len
				: sector_size-offset;
			write_port(obj,buf+offset,blk);
		}
		result &= read_port_ch(obj)==0x0d;
		
	}
	jtag_flashcon( obj, 0x00 );
	trx( obj, "\x0b\x02\x01\x00", 4, "\x0d", 1 );
	trx( obj, "\x03\x02\xB6\x80", 4, "\x0d", 1 );
	trx( obj, "\x03\x02\xB2\x14", 4, "\x0d", 1 );
	return result;
}


/** Read a sector of flash memory.
	Conveniance function, read exactly one sector of flash memory starting at 
	the specified address.
	This function fill convert the supplied address back to a sector start if it
	not exact.

	\param obj	ec2drv object to act on
	\param buf	must be big enough to hold an entire flash sector to be read
	\returns TRUE on success, FALSE on failure
*/
BOOL jtag_read_flash_sector( EC2DRV *obj, uint32_t sect_addr, uint8_t *buf,
							 BOOL scratchpad )
{
	uint32_t sector_size = scratchpad ? 
			obj->dev->scratchpad_sector_size : obj->dev->flash_sector_size;
	uint32_t sect_start_addr = (sect_addr/sector_size)*sector_size;
	uint32_t sect_end_addr = sect_start_addr+sector_size-1;
	uint32_t len = sector_size;
	
	return ec2_read_flash_jtag( obj, buf,sect_start_addr, len, scratchpad );
}


/** Read data from the code memory (Flash) of a JTAG connected processor.
	Note that this function contains JTAG operations that are not fully 
	understood but aseem necessary to tead the flash from the F120 but don't
	seem to impact the F020.

	\param obj			ec2drv object to cact on.
	\param buf			Buffer to recieve the read bytes
	\param start_addr	Address to start reading from.
						( banks are treated as a flast address range )
	\param len			Number of bytes to read.
	\returns			TRUE on success, FALSE otherwise.
 */
BOOL ec2_read_flash_jtag( EC2DRV *obj, char *buf,
						  uint32_t start_addr, int len, BOOL scratchpad )
{
	BOOL result = TRUE;
	//ec2_target_halt(obj);
	jtag_halt(obj);		// trx(obj,"\x0B\x02\x01\x00",4,"\x0D ",1);			// suspend processor?
	ec2_write_paged_sfr( obj, SFR_FLSCL, 0x80 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0xc0 );
	
	if( obj->dev->has_cache )
		ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x3e );	// Cache Lock
	
	//====== flash read starts here
	uint8_t cur_page	= ec2_read_paged_sfr( obj, SFR_SFRPAGE, 0 );
	uint8_t cur_flscl	= ec2_read_paged_sfr( obj, SFR_FLSCL, 0 );
	
	// Cache control
	if( obj->dev->has_cache )
		ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x00 );	// Cache Lock
	
	// oscilator config
	uint8_t cur_oscin = ec2_read_paged_sfr( obj, SFR_OSCICN,  0 );
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0x80 );
	
	uint8_t cur_clksel = ec2_read_paged_sfr( obj, SFR_CLKSEL, 0 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	//ec2_write_paged_sfr( obj, SFR_SFRPAGE, 0 );		// page 0
	ec2_write_raw_sfr( obj, SFR_SFRPAGE.addr, 0 );			// page 0
	
	trx(obj,"\x0B\x02\x04\x00",4,"\x0D",1); 			//ec2_core_suspend(obj);
	
	// F120 does this... T 0d 05 80 12 02 28 00	R 0d	// what is JTAG IR = 0x80?
	trx(obj,"\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);
	
	//--------------------------------------------------------------------------
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x90 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0xD6 );	

	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
//	usleep(10000);
	//--------------------------------------------------------------------------
	
	// setup for readthe actual read
	set_flash_addr_jtag( obj, start_addr );
	
	// FLASH Control
	//	T 0d 05 82 08 01 00 00	R 0d			// FLASHCON = 0x01?	8 = 8 bits
	// normal program memory
	// 82 flash control reg
	if( scratchpad )
		jtag_flashcon(obj,0x81);
	else
		jtag_flashcon(obj,0x01);
			
	int			i;
	uint32_t	addr;
	char cmd[7];	//,acmd[7];
	memset( buf, 0xff, len );
	
	uint16_t block_size = obj->dbg_adaptor==EC3 ? 0x3C : 0x0C;
	uint8_t tmp_buf[0x3d];	// room for terminator
//	printf("block size = 0x%04x\n",block_size);
	for( i=0; i<len; i+=block_size )
	{
		addr = start_addr + i;
//		printf("addr=0x%05x\n",addr);
		set_flash_addr_jtag( obj, addr );
		cmd[0] = 0x11;
		cmd[1] = 0x02;
		cmd[2] = (len-i)>=block_size ? block_size : (len-i);
		cmd[3] = 0x00;
		write_port( obj, (char*)cmd, 4 );
		read_port( obj, tmp_buf, cmd[2]+1 );	// +1 for 0x0d terminator
		memcpy(buf+i,tmp_buf,cmd[2]);
		result &= tmp_buf[cmd[2]]==0x0d;
		//read_port( obj, buf+i, cmd[2]+1 ); 
	//	result &= read_port_ch( obj )==0x0d;	// requires ec2 ver 0x13 or newer
	}
	
	jtag_flashcon( obj, 0x00 );				//trx( obj, "\x0D\x05\x82\x08\x00\x00\x00", 7, "\x0D", 1 );
	trx( obj, "\x0B\x02\x01\x00", 4, "\x0D", 1 );	// state ctrl = halt
	trx( obj, "\x03\x02\xB6\x80", 4, "\x0D", 1 );
	trx( obj, "\x03\x02\xB2\x14", 4, "\x0D", 1 );
	return result;
}


/** Write a block of data to flash.
	This routine will erase and rewrite all affected sectors.
	@FIXME this function is doing things to the wrong memory area and sizer when trying to use scratchpad.
	\param save	TRUE causes exsisting data arround that written to be saved, FALSE dosen't save unmodified bytes within the sector
*/
BOOL jtag_write_flash_block( EC2DRV *obj, uint32_t addr,
							 uint8_t *buf, uint32_t len,
							 BOOL save, BOOL scratchpad)
{
	BOOL result = TRUE;
	
	uint16_t sector_size;
	if(scratchpad)
		sector_size = obj->dev->scratchpad_sector_size;
	else
		sector_size = obj->dev->flash_sector_size;
	
	uint32_t end_addr = addr + len - 1;
	uint32_t first_sect_addr = (addr/sector_size)*sector_size;
	uint32_t last_sect_addr = (end_addr/sector_size)*sector_size;
	
	uint16_t num_sects = (last_sect_addr/sector_size) -
						 (first_sect_addr/sector_size);
//	printf("jtag_write_flash_block    addr = 0x%05x, len=0x%05x\n",addr,len);
	// allocate buffer to hold out mirror image
	uint8_t *mirror = malloc( sector_size );
	if(!mirror)
		return FALSE;
	
	uint32_t cur_addr,cur_sect_addr;
	uint32_t cur_sect_end_addr;
	cur_addr = addr;
	uint32_t offset = 0;
	
	// for each sector...
	for( cur_sect_addr = first_sect_addr;
		cur_sect_addr <= last_sect_addr;
		cur_sect_addr += sector_size )
	{
//		printf("Modifying sector @ 0x%05x\n",cur_sect_addr);
		cur_sect_end_addr = cur_sect_addr + sector_size - 1;
		
		if(save)
		{
			// read current sector
			result &= jtag_read_flash_sector( obj, cur_sect_addr, mirror, scratchpad );
		}
		else
			memset(mirror,0xff,sector_size);
		
		// modify with new data
		// must be careful as data write might not be at start of sector
		
		for( cur_addr = cur_sect_addr; cur_addr<=cur_sect_end_addr; cur_addr++ )
		{
//			printf("cur_addr = 0x%05x, addr=0x%05x, value=0x%05x\n",cur_addr, addr,buf[offset]);
			if( (cur_addr>=addr) && (cur_addr<=end_addr) )
			{
//				printf("will write 0x%05x\n",cur_addr-cur_sect_addr);
				mirror[cur_addr-cur_sect_addr] = buf[offset++];
			}
		}
		// mirror should now contan the correct sector data
		// write out the new sector
		result &= jtag_write_flash_sector( obj, cur_sect_addr,mirror,
										   scratchpad );
	}
	free(mirror);
	return result;
}

/** Erase all user code locations in the device including the ock locations.
	\param obj	EC2DRV object to act on.
	\returns	TRUE on success, FALSE otherwise.
*/
BOOL jtag_erase_flash( EC2DRV *obj )
{
	jtag_halt(obj);
//	ec2_disconnect( obj );
//	ec2_connect( obj, obj->port );
//=== Expirimental code to erase F120 ==========================================
	jtag_flashcon( obj, 0x00 );
	trx( obj, "\x0b\x02\x01\x00",4,"\x0d",1);

	
	ec2_write_paged_sfr( obj, SFR_FLSCL, 0x80 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0xc0 );
	ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x3e );
	ec2_write_paged_sfr( obj, SFR_CCH0CN, 0x07 );	// Cache Lock
	
	trx( obj, "\x0b\x02\x04\x00",4,"\x0d",1);
	trx( obj, "\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);
	trx( obj, "\x0d\x05\xc2\x07\x47\x00\x00",7,"\x0d",1);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0B\x02\x01\x00", 4, "\x0D", 1 );	
	
//		printf("Cache control\n");
	// Cache control
	if( obj->dev->has_cache )
		ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x00 );	// Cache Lock
	
//		printf("Oscilator config\n");
	// oscilator config
	uint8_t cur_oscin = ec2_read_paged_sfr( obj, SFR_OSCICN,  0 );
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0x80 );
	
//		printf("ClkSel\n");
	uint8_t cur_clksel = ec2_read_paged_sfr( obj, SFR_CLKSEL, 0 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_core_suspend(obj);
	
	///  test from read code
	/// It workes, Yes! nice and simple the same code is needed as for the read!
	//--------------------------------------------------------------------------
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x90 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0xD6 );	

	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
//=============================================================================
		
		
		
	trx( obj,"\x0b\x02\x03\x00",4,"\x0d",1);
//		trx( obj,"\x0B\x02\x04\x00",4,"\x0D",1);	// CPU core suspend
	trx( obj,"\x0D\x05\x85\x08\x00\x00\x00",7,"\x0D",1);
	trx( obj,"\x0D\x05\x82\x08\x20\x00\x00",7,"\x0D",1);	// erase mode
		
		// we do need the following lines because some processor families like the F04x have
		// both 64K and 32K variants and no distinguishing device id, just a whole family id
	if( obj->dev->lock_type==FLT_RW_ALT )
		set_flash_addr_jtag( obj, obj->dev->lock );	// alternate lock byte families

	if( obj->dev->lock_type==FLT_RW || obj->dev->lock_type==FLT_RW_ALT )
	{
		set_flash_addr_jtag( obj, obj->dev->read_lock );
	}
	else
		set_flash_addr_jtag( obj, obj->dev->lock );
	trx( obj,"\x0F\x01\xA5",3,"\x0D",1);		// erase sector
//	ec2_disconnect( obj );
//	ec2_connect( obj, obj->port );
	trx( obj, "\x0b\x02\x02\x00",4,"\x0d",1);
}

/** Erase a single sector of flash code or scratchpad
*/
BOOL jtag_erase_flash_sector( EC2DRV *obj, uint32_t sector_addr, BOOL scratchpad )
{
	jtag_halt(obj);
//=== Expirimental code to erase F120 ==========================================
	jtag_flashcon( obj, 0x00 );
	trx( obj, "\x0b\x02\x01\x00",4,"\x0d",1);
	
	ec2_write_paged_sfr( obj, SFR_FLSCL, 0x80 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0xc0 );
	ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x3e );
	ec2_write_paged_sfr( obj, SFR_CCH0CN, 0x07 );	// Cache Lock
	
	trx( obj, "\x0b\x02\x04\x00",4,"\x0d",1);
	trx( obj, "\x0d\x05\x80\x12\x02\x28\x00",7,"\x0d",1);
	trx( obj, "\x0d\x05\xc2\x07\x47\x00\x00",7,"\x0d",1);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0c\x02\xc3\x10",4,"\x00\xff\xff\x0d",4);
	trx( obj, "\x0B\x02\x01\x00", 4, "\x0D", 1 );	
	
//		printf("Cache control\n");
	// Cache control
	if( obj->dev->has_cache )
		ec2_write_paged_sfr( obj, SFR_CCH0LC, 0x00 );	// Cache Lock
	
//		printf("Oscilator config\n");
	// oscilator config
	uint8_t cur_oscin = ec2_read_paged_sfr( obj, SFR_OSCICN,  0 );
	ec2_write_paged_sfr( obj, SFR_OSCICN, 0x80 );
	
//		printf("ClkSel\n");
	uint8_t cur_clksel = ec2_read_paged_sfr( obj, SFR_CLKSEL, 0 );
	ec2_write_paged_sfr( obj, SFR_CLKSEL, 0x00 );
	
	ec2_core_suspend(obj);
	
	///  test from read code
	/// It workes, Yes! nice and simple the same code is needed as for the read!
	//--------------------------------------------------------------------------
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x90 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x00, 0x00 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0xD6 );	

	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x20, 0xe6, 0x16 );	
	
	JTAG_0x14( obj, 0x10, 0x00 );
	JTAG_0x16_Len2( obj, 0xc1, 0x40 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0xD0 );
	
	JTAG_unknown_op_0x15(obj);
	JTAG_0x16_Len3( obj, 0x00, 0x80, 0x10 );
	
//=============================================================================
		
		
		
	trx( obj,"\x0b\x02\x03\x00",4,"\x0d",1);
//		trx( obj,"\x0B\x02\x04\x00",4,"\x0D",1);	// CPU core suspend
	trx( obj,"\x0D\x05\x85\x08\x00\x00\x00",7,"\x0D",1);
	
	if(scratchpad)
		jtag_flashcon(obj,0xA0);	// erase mode scratchpad
	else
		jtag_flashcon(obj,0x20);	// erase mode code
		
	if( obj->dev->lock_type==FLT_RW || obj->dev->lock_type==FLT_RW_ALT )
	{
		set_flash_addr_jtag( obj, obj->dev->read_lock );
	}
	else
		set_flash_addr_jtag( obj, obj->dev->lock );
	trx( obj,"\x0F\x01\xA5",3,"\x0D",1);		// erase sector
	//ec2_disconnect( obj );
	//ec2_connect( obj, obj->port );
	trx( obj, "\x0b\x02\x02\x00",4,"\x0d",1);
}



/** Set flash address register, Internal helper function,
	\param obj ec2 object to act on.
	\param 24 bit address to set the flash pointer to
	Note that flash preamble must be used before this can be used successfully
 */
void set_flash_addr_jtag( EC2DRV *obj, uint32_t addr )
{
	DUMP_FUNC();
	char cmd[7];
	cmd[0] = 0x0d;
	cmd[1] = 0x05;
	cmd[2] = 0x84;				// write flash address
//	cmd[3] = 0x10;				// for F020
//	cmd[3] = 0x11;				// for F120
	// @FIXME detect fproper processor here andadjust
	if( obj->dev->flash_size > 0x10000 )
		cmd[3] = 0x11;					// number of bits 0x11 (17) for F120, 0x10(16) for others
	else
		cmd[3] = 0x10;
	cmd[4] = addr & 0xFF;			// addr low
	cmd[5] = (addr >> 8) & 0xFF;	// addr middle
	cmd[6] = (addr >> 16) & 0xFF;	// addr top
	trx( obj, cmd, 7, "\x0D", 1 );
}

////////////////////////////////////////////////////////////////////////////////
// Low Level JTAG helper routines
////////////////////////////////////////////////////////////////////////////////


/** Write to the devices JTAG IR Register
	\param obj	ec2drv object to operate on
	\param reg IR register address to write to
	\param num_bits	number of bits of data to use (starting at lsb)
*/
BOOL jtag_write_IR( EC2DRV *obj, uint16_t reg, uint8_t num_bits, uint32_t data )
{
	uint8_t cmd[7];
	cmd[0] = 0x0d;
	cmd[1] = 0x05;	// num bytes
	cmd[2] = reg;
	cmd[3] = num_bits;
	cmd[4] = data & 0xff;
	cmd[5] = (data>>8) & 0xff;
	cmd[6] = (data>>16) & 0xff;
	return trx( obj, cmd, 7, "\x0d", 1 );
}

/** Unknown JTAG operation 0x14
	CMD:
		\x14\x2\x??\x??
		reply  1 byte and the 0x0d end of responce byte

	\returns the 1 byte of data returned from the hardware
*/
uint8_t JTAG_0x14( EC2DRV *obj, uint8_t a, uint8_t b )
{
	uint8_t cmd[4];
	cmd[0] = 0x14;
	cmd[1] = 0x02;
	cmd[2] = a;
	cmd[3] = b;
	write_port( obj, cmd, 4 ); 
	read_port( obj, cmd, 2 );
	return cmd[0];
}


/** An unknown opperation that occurs frequently.
	Seems to always read 4 then the terminating 0x0d
*/
uint8_t JTAG_unknown_op_0x15( EC2DRV *obj )
{
	uint8_t buf[2];
	write_port( obj, "\x15\x02\x18\x00",4);
	read_port( obj, buf, 2);
	return buf[0];
}


uint16_t JTAG_0x16_Len2( EC2DRV *obj, uint8_t a, uint8_t b )
{
	uint8_t cmd[10];
	cmd[0] = 0x16;
	cmd[1] = 0x02;
	cmd[2] = a;
	cmd[3] = b;
	write_port( obj, cmd, 4);
	read_port( obj, cmd, 3);
	return cmd[0] | (cmd[1]<<8);
}

uint32_t JTAG_0x16_Len3( EC2DRV *obj, uint8_t a, uint8_t b, uint8_t c )
{
	uint8_t cmd[10];
	cmd[0] = 0x16;
	cmd[1] = 0x03;
	cmd[2] = a;
	cmd[3] = b;
	cmd[4] = c;
	write_port( obj, cmd, 5);
	read_port( obj, cmd, 4);
	return cmd[0] | (cmd[1]<<8) | (cmd[1]<<16);
}
