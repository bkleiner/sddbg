/*-------------------------------------------------------------------------
  simi.c - source file for simulator interaction

	      Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1999)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   
   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!  
-------------------------------------------------------------------------*/
#include "sdcdb.h"
#include "simi.h"
#include <assert.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#else
//#error "Cannot build debugger without socket support"
#endif
FILE *simin ; /* stream for simulator input */
FILE *simout; /* stream for simulator output */

int sock = -1; /* socket descriptor to comm with simulator */
pid_t simPid = -1;
static char simibuff[MAX_SIM_BUFF];    /* sim buffer       */
static char regBuff[MAX_SIM_BUFF];
static char *sbp = simibuff;           /* simulator buffer pointer */
extern char **environ;
char simactive = 0;

static memcache_t memCache[NMEM_CACHE];

#include "ec2drv.h"
load_ihex( char *filename, char buf[0x10000] );

/*-----------------------------------------------------------------*/
/* get data from  memory cache/ load cache from simulator          */
/*-----------------------------------------------------------------*/
static char *getMemCache(unsigned int addr,int cachenum, int size)
{
    char *resp, *buf;
    unsigned int laddr;
    memcache_t *cache = &memCache[cachenum];
//	printf("static char *getMemCache( %i, %i, %i )\n", addr, cachenum, size );

    if ( cache->size <=   0 ||
         cache->addr > addr ||
         cache->addr + cache->size < addr + size )
    {
        if ( cachenum == IMEM_CACHE )
        {
/*
            sendSim("di 0x0 0xff\n");
*/
			// EC2 version
			ec2_read_ram( cache->buffer, 0x00, 0x100 );
			return cache->buffer + (addr - cache->addr)*3;
        }
        else if ( cachenum == SREG_CACHE )
        {
            sendSim("ds 0x80 0xff\n");
        }
        else
        {
            laddr = addr & 0xffffffc0;
            sprintf(cache->buffer,"dx 0x%x 0x%x\n",laddr,laddr+0xff );
            sendSim(cache->buffer);
        }
        waitForSim(100,NULL);
        resp = simResponse();
        cache->addr = strtol(resp,0,0);
        buf = cache->buffer;
        cache->size = 0;
        while ( *resp && *(resp+1) && *(resp+2))
        {
            /* cache is a stringbuffer with ascii data like
               " 00 00 00 00 00 00 00 00"
            */
            resp += 2;
            laddr = 0;
            /* skip thru the address part */
            while (isxdigit(*resp)) resp++;
            while ( *resp && *resp != '\n')
            {
                if ( laddr < 24 )
                {
                    laddr++ ;
                    *buf++ = *resp ;
                }
                resp++;
            }
            resp++ ;
            cache->size += 8;
        }
        *buf = '\0';
        if ( cache->addr > addr ||
             cache->addr + cache->size < addr + size )
            return NULL;
    }

    return cache->buffer + (addr - cache->addr)*3;
}

/*-----------------------------------------------------------------*/
/* invalidate memory cache                                         */
/*-----------------------------------------------------------------*/
static void invalidateCache( int cachenum )
{
    memCache[cachenum].size = 0;  
}

/*-----------------------------------------------------------------*/
/* waitForSim - wait till simulator is done its job                */
/*-----------------------------------------------------------------*/
void waitForSim(int timeout_ms, char *expect)
{
}

/*-----------------------------------------------------------------*/
/* openSimulator - connect to the EC2                              */
/*-----------------------------------------------------------------*/
void openSimulator (char **args, int nargs)
{
	ec2_connect("/dev/ttyS0");
	simactive = 1;	// tell the sdcdb core we are up
    Dprintf(D_simi, ("simi: openSimulator\n"));
    invalidateCache(XMEM_CACHE);
    invalidateCache(IMEM_CACHE);
    invalidateCache(SREG_CACHE);
}


/*-----------------------------------------------------------------*/
/* simResponse - returns buffer to simulator's response            */
/*-----------------------------------------------------------------*/
char *simResponse()
{
	return "file \"test.ihx\"\n";
    return "NOT IMPLEMENTED: simResponse()\n";
}

/*-----------------------------------------------------------------*/
/* sendSim - sends a command to the simuator                 */
/*-----------------------------------------------------------------*/
void sendSim(char *s)
{
	printf("void sendSim(%s)\n",s);
#if 0
    if ( ! simout ) 
        return;

    Dprintf(D_simi, ("simi: sendSim-->%s", s));  // s has LF at end already
    fputs(s,simout);
    fflush(simout);
#endif
}


static int getMemString(char *buffer, char wrflag, 
                        unsigned int *addr, char mem, int size )
{
    int cachenr = NMEM_CACHE;
    char *prefix;
    char *cmd ;
//printf("getMemString(char *buffer, char wrflag, unsigned int *addr, char mem, int size )\n");
    if ( wrflag )
        cmd = "set mem";
    else
        cmd = "dump";
    buffer[0] = '\0' ;

    switch (mem) 
    {
        case 'A': /* External stack */
        case 'F': /* External ram */
            prefix = "xram";
            cachenr = XMEM_CACHE;
            break;
        case 'C': /* Code */
        case 'D': /* Code / static segment */
            prefix = "rom";
            break;
        case 'B': /* Internal stack */  
        case 'E': /* Internal ram (lower 128) bytes */
        case 'G': /* Internal ram */
            prefix = "iram";
            cachenr = IMEM_CACHE;
            break;
        case 'H': /* Bit addressable */
        case 'J': /* SBIT space */
            cachenr = BIT_CACHE;
            if ( wrflag )
            {
                cmd = "set bit";
            }
            sprintf(buffer,"%s 0x%x\n",cmd,*addr);
            return cachenr;
            break;
        case 'I': /* SFR space */
            prefix = "sfr" ;
            cachenr = SREG_CACHE;
            break;
        case 'R': /* Register space */ 
            prefix = "iram";
            /* get register bank */
            cachenr = simGetValue (0xd0,'I',1); 
            *addr  += cachenr & 0x18 ;
            cachenr = IMEM_CACHE;
            break;
        default: 
        case 'Z': /* undefined space code */
            return cachenr;
    }
    if ( wrflag )
        sprintf(buffer,"%s %s 0x%x\n",cmd,prefix,*addr);
    else
        sprintf(buffer,"%s %s 0x%x 0x%x\n",cmd,prefix,*addr,*addr+size-1);

    return cachenr;
}

void simSetPC( unsigned int addr )
{
	printf("ERROR: set PC not implemented addr=0x%04X\n",addr);
}

int simSetValue (unsigned int addr,char mem, int size, unsigned long val)
{
	printf("ERROR: simSetValue no implemented\n");
#if 0
    char cachenr, i;
    char buffer[40];
    char *s;

    if ( size <= 0 )
        return 0;

    cachenr = getMemString(buffer,1,&addr,mem,size);
    if ( cachenr < NMEM_CACHE )
    {
        invalidateCache(cachenr);
    }
    s = buffer + strlen(buffer) -1;
    for ( i = 0 ; i < size ; i++ )
    {
        sprintf(s," 0x%x", val & 0xff);
        s += strlen(s);
        val >>= 8;
    }
    sprintf(s,"\n");
    sendSim(buffer);
    waitForSim(100,NULL);
    simResponse();   
#endif
    return 0;
}


/*-----------------------------------------------------------------*/
/* simGetValue - get value @ address for mem space                 */
/*-----------------------------------------------------------------*/
unsigned long simGetValue (unsigned int addr,char mem, int size)
{
	char cachenr, i;
    char buffer[40];
    char *resp;
unsigned long r;	// rw added
	char tmp[4] = {0,0,0,0};
	if ( size <= 0 )
		return 0;
	cachenr = getMemString(buffer,0,&addr,mem,size);
//	printf("memstr ='%s'\n",buffer);

	// RW version here
	switch( mem )
	{
        case 'A': /* External stack */
        case 'F': /* External ram */
//            prefix = "xram";
            cachenr = XMEM_CACHE;
            break;
        case 'C': /* Code */
        case 'D': /* Code / static segment */
//            prefix = "rom";
            break;
        case 'B': /* Internal stack */  
        case 'E': /* Internal ram (lower 128) bytes */
        case 'G': /* Internal ram */
//            prefix = "iram";
//			printf("*** IRAM *** addr = 0x%02X, size = %i\n",addr,size);
//            cachenr = IMEM_CACHE;
				ec2_read_ram( tmp, addr, size );
				printf("[ %02X %02X %02X %02X  ]\n", (unsigned char)tmp[0], (unsigned char)tmp[1], (unsigned char)tmp[2], (unsigned char)tmp[3] );
// following taken out for now, we will ignore the cache
#if 0
			resp = getMemCache(addr,IMEM_CACHE,size);
			if( !resp )
			{
				// not in cache, go to hardware
				ec2_read_ram( &r, addr, size );
				printf("[ 0x%02X, 0x%02X ]\n",*((unsigned char*)&r),*((unsigned char*)&r+1) );
			}
			else
			{
				printf("[ 0x%02X, 0x%02X ]\n",resp[0],resp[1] );
			}
            break;
        case 'H': /* Bit addressable */
        case 'J': /* SBIT space */
            cachenr = BIT_CACHE;
//            if ( wrflag )
//            {
//                cmd = "set bit";
//            }
//            sprintf(buffer,"%s 0x%x\n",cmd,*addr);
            return cachenr;
#endif
            break;
        case 'I': /* SFR space */
//            prefix = "sfr" ;
            cachenr = SREG_CACHE;
			ec2_read_sfr( tmp, addr, size );
			printf("[ %02X %02X %02X %02X  ]\n", (unsigned char)tmp[0], (unsigned char)tmp[1], (unsigned char)tmp[2], (unsigned char)tmp[3] );
            break;
        case 'R': /* Register space */ 
//            prefix = "iram";
            /* get register bank */
            cachenr = simGetValue (0xd0,'I',1);
//            *addr  += cachenr & 0x18 ;
            cachenr = IMEM_CACHE;
            break;
        default: 
        case 'Z': /* undefined space code */
			break;
	}

	return tmp[0] | tmp[1] << 8 | tmp[2] << 16 | tmp[3] << 24 ;


#if 0	
	// get from cache if cache is valid
	cachenr = getMemString(buffer,0,&addr,mem,size);
	resp = NULL;
	if ( cachenr < NMEM_CACHE )
	{
		resp = getMemCache(addr,cachenr,size);
	}
	if ( !resp )
	{
		// send appropriete command to EC2

	}
#endif
#if 0
    unsigned int b[4] = {0,0,0,0}; /* can be a max of four bytes long */
    char cachenr, i;
    char buffer[40];
    char *resp;

    if ( size <= 0 )
        return 0;

    cachenr = getMemString(buffer,0,&addr,mem,size);

    resp = NULL;
    if ( cachenr < NMEM_CACHE )
    {
        resp = getMemCache(addr,cachenr,size);
    }
    if ( !resp )
    {
        /* create the simulator command */
        sendSim(buffer);
        waitForSim(100,NULL);
        resp = simResponse();

        /* got the response we need to parse it the response
           is of the form 
           [address] [v] [v] [v] ... special case in
           case of bit variables which case it becomes
           [address] [assembler bit address] [v] */
        /* first skip thru white space */
        while (isspace(*resp)) resp++ ;

        if (strncmp(resp, "0x",2) == 0)
            resp += 2;

        /* skip thru the address part */
        while (isxdigit(*resp)) resp++;

    }   
    /* make the branch for bit variables */
    if ( cachenr == BIT_CACHE) 
    {
        /* skip until newline */
        while (*resp && *resp != '\n' ) resp++ ;
        if ( *--resp != '0' )
            b[0] = 1;
    }
    else 
    {	
        for (i = 0 ; i < size ; i++ ) 
        {
            /* skip white space */
            while (isspace(*resp)) resp++ ;
	    
            b[i] = strtol(resp,&resp,16);
        }
    }

    return b[0] | b[1] << 8 | b[2] << 16 | b[3] << 24 ;
#endif

}

/** simSetBP - set break point for a given address
  * be careful the 8051 simulator had unlimited breakpoints, we only have 4!
  */
void simSetBP( unsigned int addr )
{
	printf("SETTING SIM BP\n");
	if( ec2_addBreakpoint( addr ) == FALSE )
		printf("error setting breakpoint at 0x%04X\n", addr );
}

/*-----------------------------------------------------------------*/
/* simClearBP - clear a break point                                */
/*-----------------------------------------------------------------*/
void simClearBP (unsigned int addr)
{
	printf("CLEARING SIM BP\n");
	if( ec2_removeBreakpoint( addr ) == FALSE )
		printf("error removing breakpoint at 0x%04X\n", addr );
}

/*-----------------------------------------------------------------*/
/* simLoadFile - load the simulator file                           */
/*-----------------------------------------------------------------*/
void simLoadFile (char *s)
{
	char buf[0x10000];
	load_ihex( s, buf );
//	printf("writing to flash\n");
    printf("file \"%s\"\n",s);

	//ec2_write_flash_auto_erase( buf, 0x0000, 0x10000 );
//	ec2_erase_flash();
//	ec2_write_flash( buf, 0x0000, 0x01000 );	// HACK: small test
#if 0
    char buff[128];

    sprintf(buff,"file \"%s\"\n",s);
    printf(buff);
    sendSim(buff);
    waitForSim(500,NULL);
#endif
}

/*-----------------------------------------------------------------*/
/* simGoTillBp - send 'go' to simulator till a bp then return addr */
/*-----------------------------------------------------------------*/
unsigned int simGoTillBp ( unsigned int gaddr)
{
	invalidateCache(XMEM_CACHE);
	invalidateCache(IMEM_CACHE);
	invalidateCache(SREG_CACHE);

	switch( gaddr)
	{
		case 0:		// initial start, start & stop from address 0
			printf("reset hardware\n");			
			//ec2_target_reset();	// this might be broken,  continue after this dosen't work...
			break;
		case -1:	// resume
//			printf("resume\n");
			gaddr = ec2_target_run_bp();
			break;
		case 1:		// nexti or next
			printf("nexti or next\n");
			break;
		case 2:		// stepi or step
			printf("stepi\n");
			gaddr = ec2_step();
			break;
		default:
			printf("Error, simGoTillBp > 0!\n");
      		exit(1);
	}

#if 0
    char *sr;
    unsigned addr ; 
    char *sfmt;
    int wait_ms = 1000;

    invalidateCache(XMEM_CACHE);
    invalidateCache(IMEM_CACHE);
    invalidateCache(SREG_CACHE);
    if (gaddr == 0) {
      /* initial start, start & stop from address 0 */
      //char buf[20];

         // this program is setting up a bunch of breakpoints automatically
         // at key places.  Like at startup & main() and other function
         // entry points.  So we don't need to setup one here..
      //sendSim("break 0x0\n");
      //sleep(1);
      //waitForSim();

      sendSim("reset\n");
      waitForSim(wait_ms, NULL);
      sendSim("run 0x0\n");
    } else	if (gaddr == -1) { /* resume */
      sendSim ("run\n");
      wait_ms = 100;
    }
    else 	if (gaddr == 1 ) { /* nexti or next */
      sendSim ("next\n");
      wait_ms = 100;
    }
    else 	if (gaddr == 2 ) { /* stepi or step */
      sendSim ("step\n");
      wait_ms = 100;
    }
    else  { 
      printf("Error, simGoTillBp > 0!\n");
      exit(1);
    }

    waitForSim(wait_ms, NULL);
    
    /* get the simulator response */
    sr = simResponse();
    /* check for errors */
    while ( *sr )
    {
        while ( *sr && *sr != 'E' ) sr++ ;
        if ( !*sr )
            break;
        if ( ! strncmp(sr,"Error:",6))
        {
            fputs(sr,stdout);
            break;
        }
        sr++ ;
    }

    nointerrupt = 1;
    /* get answer of stop command */
    if ( userinterrupt )
        waitForSim(wait_ms, NULL);

    /* better solution: ask pc */
    sendSim ("pc\n");
    waitForSim(100, NULL);
    sr = simResponse();
    nointerrupt = 0;

    gaddr = strtol(sr+3,0,0);
#endif
    return gaddr;
}

/*-----------------------------------------------------------------*/
/* simReset - reset the simulator                                  */
/*-----------------------------------------------------------------*/
void simReset ()
{
    invalidateCache(XMEM_CACHE);
    invalidateCache(IMEM_CACHE);
    invalidateCache(SREG_CACHE);
	//ec2_reset_target();
}


/*-----------------------------------------------------------------*/
/* closeSimulator - close connection to simulator                  */
/*-----------------------------------------------------------------*/
void closeSimulator ()
{
	ec2_disconnect();
}






// RW added EC2 support code below this point
///////////////////////////////////////////////////////////////////////
int do_line( char *line, char buf[0x10000] );
#define MAX_LINE 255
load_ihex( char *filename, char buf[0x10000] )
{
	FILE *in;
	char line[MAX_LINE];
	
	printf("loading file '%s'\n", filename);
	in = fopen( filename, "r" );
	if( in )
	{
		// read in lines
		while( fgets( line, MAX_LINE, in ) )
		if( !do_line( line, buf ) )
		{
			printf("Corrupt ihex file\n");
			return;
		}
		close(in);
	}
	else
		printf("CRAP");
}

// read in an ASCII-HEX pair and return the corresponding byte
char get_byte( char *p )
{
	char c;
	c = (p[0] <= '9' ? p[0] - '0' : p[0] - 'A' + 0x0A) << 4;
	c |= (p[1] <= '9' ? p[1] - '0' : p[1] - 'A' + 0x0A);
	return c;
}


// return 0 on failure, 1 on success
int do_line( char *line, char buf[0x10000] )
{
	int i;
	uint8_t bcnt, cnt, type, c;
	char csum, acsum;
	uint16_t addr;
	i = 0;
	csum = 0;
	if( line[i++] != ':' )
		return 1;
	
	// Byte count
	bcnt = get_byte( line+i );
	csum += bcnt;
	i+= 2;
	
	// Address
	c = get_byte( line+i );
	csum += c;
	addr = c<<8;
	c = get_byte( line+i );
	csum += c;
	addr |= c;
	i+= 4;
	
	// type
	type = get_byte( line+i );
	csum += type;
	i+= 2;
	if(type != 0x00)
		return 1;
	
	// data bytes
	for( cnt = 0; cnt<bcnt; cnt++ )
	{
		buf[ addr+cnt ] = get_byte( line+i );
		csum += buf[ addr+cnt ];
		i+= 2;
	}
	return 1;
	// checksum
	acsum = get_byte( line+i );
	csum = ~(csum&0xFF)+1;
	if( acsum==csum )
	{
		printf("Y\n");
		return 1;	// success
	}
	else
	{
		printf("N\n");
		printf("line='%s' csum = 0x%02x, ascsum = 0x%02x\n",line, (unsigned char)csum,(unsigned char)acsum);
		return 0;	// Failure
	}
}

