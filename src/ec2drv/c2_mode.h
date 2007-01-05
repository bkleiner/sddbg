/**	Routines to provide support for the C2 mode.
	This is ued to communicate with processors like the
	C8051F310 and C8051F340 etc

	(C) Ricky White 2006
 */
#ifndef C2_MODE_H
#define C2_MODE_H
#include <stdint.h>
#include "ec2drv.h"
void c2_erase_flash( EC2DRV *obj );
BOOL c2_write_flash( EC2DRV *obj, uint8_t *buf, uint32_t start_addr, int len );
BOOL c2_read_flash( EC2DRV *obj, uint8_t *buf, uint32_t start_addr, int len );
void ec2_read_xdata_c2_emif( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL ec2_write_xdata_c2_emif( EC2DRV *obj, char *buf, int start_addr, int len );
void c2_read_ram( EC2DRV *obj, char *buf, int start_addr, int len );
void c2_read_ram_sfr( EC2DRV *obj, char *buf, int start_addr, int len, BOOL sfr );
BOOL c2_write_ram( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL c2_write_xdata( EC2DRV *obj, char *buf, int start_addr, int len );
BOOL c2_read_xdata( EC2DRV *obj, char *buf, int start_addr, int len );
void c2_write_sfr( EC2DRV *obj, uint8_t value, uint8_t addr );
#endif
