#include <stdint.h>
#include "ec2drv.h"
#ifndef EC2_DEVICES
#define EC2_DEVICES


typedef enum { AUTO, JTAG, C2 } EC2_MODE;
typedef enum
{
	FLT_SINGLE,		///< single lock as is F310 etc, uses lock
	FLT_SINGLE_ALT,	///< single lock as is F310 etc, uses lock, alternate is in read_lock
	FLT_RW,			///<Read and write locks eg F020, uses read_lock and write_lock
	FLT_RW_ALT		///<Read and write Locks eg F040, uses read_lock and write_lock additionally lock holds an alternate readlock location since some devices have smaller flash but the same device id!
} FLASH_LOCK_TYPE;

typedef struct
{
	char		name[32];
	uint8_t 	id;
	uint8_t 	rev;		// -1 is any matching device id
	EC2_MODE	mode;
	
	uint16_t	internal_xram_size;
	uint16_t	flash_size;
	uint16_t	flash_sector_size;
	FLASH_LOCK_TYPE	lock_type;
	uint16_t	lock;
	uint16_t	read_lock;
	uint16_t	write_lock;
	
	BOOL		tested;		// TRUE if ec2dev developers are happy the device support has been tested and is complete
} EC2_DEVICE;

EC2_DEVICE *getDevice( uint8_t id, uint8_t rev );

#endif
