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
#ifndef EC2_H
#define EC2_H
#include <stdint.h>
#if !BOOL
	typedef uint8_t BOOL;
	#define TRUE	1
	#define FALSE	0
#endif
BOOL ec2_connect( char *port );
void ec2_disconnect();
void ec2_reset();
void ec2_read_sfr( char *buf, uint8_t len, uint8_t addr  );
void ec2_read_ram( char *buf, int start_addr, int len );
BOOL ec2_write_ram( char *buf, int start_addr, int len );
BOOL ec2_write_xdata( char *buf, int start_addr, int len );
BOOL ec2_write_xdata_page( char *buf, unsigned char page,
						   unsigned char start, int len );
void ec2_read_xdata( char *buf, int start_addr, int len );
void ec2_read_xdata_page( char *buf, unsigned char page,
						  unsigned char start, int len );
BOOL ec2_read_flash( char *buf, int start_addr, int len );
BOOL ec2_write_flash( char *buf, int start_addr, int len );
BOOL ec2_write_flash_auto_erase( char *buf, int start_addr, int len );
void ec2_erase_flash_sector( int sector_addr );
void ec2_erase_flash();
BOOL ec2_target_go();
uint16_t ec2_target_run_bp();
BOOL ec2_target_halt();
BOOL ec2_target_halt_poll();
BOOL ec2_target_reset();
int ec2_step();
void read_active_regs( char *buf );
uint16_t ec2_read_pc();

BOOL ec2_addBreakpoint( uint16_t addr );
BOOL ec2_removeBreakpoint( uint16_t addr );

#endif
