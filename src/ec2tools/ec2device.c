#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ec2drv.h"



int main(int argc, char *argv[])
{
	char buf[4096];
	int i;
	EC2DRV obj;
	
	if( argc != 2 )
	{
		printf("ec2device\nSyntax:\n\tec2device /dev/ttyS0\n");
		return EXIT_FAILURE;
	}
	
	obj.debug	= FALSE;
	obj.mode	= AUTO;
	if( ec2_connect( &obj, argv[1] ) )
	{
		printf("FOUND:\n");
		printf("device\t: %s\n", obj.dev->name);
		printf("mode\t: %s\n", obj.dev->mode==C2 ? "C2" : "JTAG");
		printf("\n");
	}
	else
	{
		printf("ERROR: coulden't communicate with the EC2 debug adaptor\n");
		exit(-1);
	}
	
	if( !obj.dev->tested )
	{
		printf("Support for this device has not been fully tested or may not be complete\n");
		printf("You can be of assistance, please visit:\n"); printf("http://sourceforge.net/tracker/?atid=775284&group_id=149627&func=browse\n");
		printf("Please feel free to report your succcess / failure.\n");
		printf("If there are any issues we can probably resolve them with your help since we don't have boards for every supported processor.\n");
	}
	else
	{
		printf("If the above does not match your processor exactly, please contact us at :\n"); printf("http://sourceforge.net/tracker/?atid=775284&group_id=149627&func=browse\n");
		printf("We need your help to discover if any sub device id's exsist in the silicon\n");
	}
	ec2_disconnect( &obj );
	exit(0);
}
