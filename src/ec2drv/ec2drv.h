/** EC2DRV Driver Library
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
#ifndef EC2_H
#define EC2_H
#include <stdint.h>
#if !BOOL
	typedef uint8_t BOOL;
	#define TRUE	1
	#define FALSE	0
#endif
#include "devices.h"

/**	Object for an EC2.
	Create one of these for every EC you wish to use
*/
typedef struct
{
	int			fd;				///< file descriptor for com port
	uint8_t		bp_flags;		///< mirror of EC2 breakpoint byte
	uint16_t	bpaddr[4];		///< breakpoint addresses
	EC2_MODE	mode;			///< Communication method used to communicate with the target chip.
	EC2_DEVICE	*dev; 
	BOOL		debug;			///< true to enable debugging on an object, false otherwise
	uint8_t		progress;		///< percentage complete, check from an alternative thread or use callback
	void (*progress_cbk)(uint8_t percent);	///< called on significant progress update intervale
} EC2DRV;


uint16_t ec2drv_version();
BOOL ec2_connect( EC2DRV *obj, char *port );
void ec2_disconnect( EC2DRV *obj );
void ec2_reset( EC2DRV *obj );
void ec2_read_sfr( EC2DRV *obj, char *buf, uint8_t addr );
void ec2_write_sfr( EC2DRV *obj, char *buf, uint8_t addr );
void ec2_read_ram( EC2DRV *obj, char *buf, int start_addr, int len );
void ec2_read_ram_sfr( EC2DRV *obj, char *buf, int start_addr, int len, BOOL sfr );
BOOL ec2_write_ram( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_write_xdata( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_write_xdata_page( EC2DRV *obj, char *buf, unsigned char page,
						   unsigned char start, int len );
void ec2_read_xdata( EC2DRV *obj, char *buf, int start_addr, int len );
void ec2_read_xdata_page( EC2DRV *obj, char *buf, unsigned char page,
						  unsigned char start, int len );
BOOL ec2_read_flash( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_read_flash_scratchpad( EC2DRV *obj, char *buf, int start_addr, int len );

BOOL ec2_write_flash( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_write_flash_auto_erase( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_write_flash_scratchpad( EC2DRV *obj, char *buf, int start_addr, int len );
void ec2_write_flash_scratchpad_merge( EC2DRV *obj, char *buf, int start_addr, int len );
void ec2_erase_flash_scratchpad( EC2DRV *obj );
void ec2_erase_flash_sector( EC2DRV *obj, int sector_addr );
void ec2_erase_flash( EC2DRV *obj );
BOOL ec2_target_go( EC2DRV *obj );
uint16_t ec2_target_run_bp( EC2DRV *obj );
BOOL ec2_target_halt( EC2DRV *obj );
BOOL ec2_target_halt_poll( EC2DRV *obj );
BOOL ec2_target_reset( EC2DRV *obj );
uint16_t ec2_step( EC2DRV *obj );
void read_active_regs( EC2DRV *obj, char *buf );
uint16_t ec2_read_pc( EC2DRV *obj );
void ec2_set_pc( EC2DRV *obj, uint16_t addr );
BOOL ec2_addBreakpoint( EC2DRV *obj, uint16_t addr );
BOOL ec2_removeBreakpoint( EC2DRV *obj, uint16_t addr );
BOOL ec2_write_firmware( EC2DRV *obj, char *image, uint16_t len);

#endif
