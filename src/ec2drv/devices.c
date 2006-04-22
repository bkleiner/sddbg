#include <stdio.h>
#include "devices.h"


EC2_DEVICE devices[] =
{
	{
		"C8051F310",
		0x08,			// id
		255,			// revision (255 = any)
		C2,				// debug mode
		1024,			// XRAM size
		0x3FFF,			// Flash size
		512,			// Flash sector size
		TRUE			// tested
	},
	{
		"C8051F320",
		0x09,			// id
		255,			// revision (255 = any)
		C2,				// debug mode
		1024,			// XRAM size
		0x3FFF,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F020",
		0x03,			// id
		255,			// revision (255 = any)
		JTAG,			// debug mode
		4096,			// XRAM size
		0xFFFF,			// Flash size
		512,			// Flash sector size
		TRUE			// tested
	},
	// untested devices below this point
	{
		"C8051F330",	// F330 - F335
		0x0A,			// id
		255,			// revision (255 = any)
		C2,				// debug mode
		512,			// XRAM size
		8196,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F018",	// F018-019
		0x02,			// id
		255,			// revision (255 = any)
		JTAG,				// debug mode
		1024,			// XRAM size
		0x3FFF,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F040",	// F040-F047
		0x05,			// id
		255,			// revision (255 = any)
		JTAG,			// debug mode
		4096,			// XRAM size
		0xFFFF,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F060",	// F060-F067
		0x06,			// id
		255,			// revision (255 = any)
		JTAG,			// debug mode
		4096,			// XRAM size
		0xFFFF,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F120",	// F120-F127, F130-F137
		0x07,			// id
		255,			// revision (255 = any)
		JTAG,			// debug mode
		8196,			// XRAM size
		0xFFFF,			// Flash size, actually bank switched to 128K, how do we handle this?
		1024,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F2XX",	// F206 F220/1/6 F230/1/6
		0x01,			// id
		255,			// revision (255 = any)
		JTAG,			// debug mode
		1024,			// XRAM size
		8196,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F300",	// F300-F305
		0x04,			// id
		255,			// revision (255 = any)
		C2,				// debug mode
		0,				// XRAM size
		8196,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{
		"C8051F350",	// F300-F305
		0x0B,			// id
		255,			// revision (255 = any)
		C2,				// debug mode
		512,			// XRAM size
		8196,			// Flash size
		512,			// Flash sector size
		FALSE			// tested
	},
	{ 0,0,0,0 }			// end marker
};

static EC2_DEVICE unknown_dev = { "Unknown",-1,255,AUTO,0,8196,512,FALSE };

// Pick the closest device and return it.
EC2_DEVICE *getDevice( uint8_t id, uint8_t rev )
{
	int i = 0;
	do
	{
		if( (devices[i].id==id)&&(devices[i].rev==255) )
			return &devices[i];
		else if( (devices[i].id==id)&&(devices[i].rev==rev) )
			return &devices[i];
	} while( devices[++i].name[0] != 0 );
	printf("id=0x%02x, rev=%02x\n",id, rev);
	return &unknown_dev;
}
