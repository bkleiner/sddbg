#include <stdint.h>
#include "ec2drv.h"
#ifndef EC2_DEVICES
#define EC2_DEVICES


typedef enum { AUTO, JTAG, C2 } EC2_MODE;

typedef struct
{
	char		name[32];
	uint8_t 	id;
	uint8_t 	rev;		// -1 is any matching device id
	EC2_MODE	mode;
	
	uint16_t	internal_xram_size;
	uint16_t	flash_size;
	uint8_t		flash_sector_size;
	BOOL		tested;		// TRUE if ec2dev developers are happy the device support has been tested and is complete
} EC2_DEVICE;

EC2_DEVICE *getDevice( uint8_t id, uint8_t rev );

#endif
