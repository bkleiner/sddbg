/***************************************************************************
 *   Copyright (C) 2006 by Ricky White   *
 *   ricky@localhost.localdomain   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>			// File control definitions
#include <errno.h>			// Error number definitions
#include <termios.h>		// POSIX terminal control definitions
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
//FILE *simin ; /* stream for simulator input */
//FILE *simout; /* stream for simulator output */

#include "targets51.h"
#include "types.h"

TargetS51::TargetS51()
  : Target(),
	bConnected(false)
{
	sock = -1;
	simPid = -1;
}


TargetS51::~TargetS51()
{
}

bool TargetS51::connect()
{
	struct sockaddr_in sin;
	int retry = 0;
	int i ;
	if( !bConnected )
	{
	try_connect:
		sock = socket(AF_INET,SOCK_STREAM,0);
		memset(&sin,0,sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		sin.sin_port = htons(9756);
	
		// connect to the simulator
		if( ::connect(sock,(struct sockaddr *) &sin, sizeof(sin)) < 0 )
		{
		// if failed then wait 1 second & try again
		// do this for 10 secs only
		if(retry < 10)
		{
			if ( !retry )
			{
				/* fork and start the simulator as a subprocess */
				if( simPid = fork() )
				{
					printf("simi: simulator pid %d\n",(int) simPid);
				}
				else
				{
					// we are in the child process : start the simulator
					signal(SIGHUP , SIG_IGN );
					signal(SIGINT , SIG_IGN );
					signal(SIGABRT, SIG_IGN );
					signal(SIGCHLD, SIG_IGN );
					char *argv[] = {"s51","-Z9756","-tC52",0};
					if( execvp("s51",argv) < 0 )
					{
						perror("cannot exec simulator");
						exit(1);
					}
				}
			}
			retry ++;
			sleep (1);
			goto try_connect;
		}
		perror("connect failed :");
		exit(1);
		}
		cout<<"simulator started, waiting for prompt"<<endl;
	
		// go the socket now turn it into a file handle
		if( !(simin = fdopen(sock,"r")) )
		{
			fprintf(stderr,"cannot open socket for read\n");
			exit(1);
		}
		
		if( !(simout = fdopen(sock,"w")) )
		{
			fprintf(stderr,"cannot open socket for write\n");
			exit(1);
		}
		cout<<"Waiting for sim."<<endl;
		bConnected = true;	
		recvSim( 200 );
		cout<<"Ready."<<endl;
		return true;
	}
	else
		return false;
}

bool TargetS51::disconnect()
{
	if( bConnected )
	{
		bConnected = false;
		sendSim("quit\n");
		recvSim( 2000 );
		fclose (simin);
		fclose (simout);
		shutdown(sock,2);
		close(sock);
		sock = -1;
		if ( simPid > 0 )
			kill (simPid,SIGKILL);
	}	
	return true;
}

bool TargetS51::is_connected()
{
	return bConnected;
}

string TargetS51::port()
{
	return "local";
}

bool TargetS51::set_port( string port )
{
	return false;	// no port set for simulator
}

string TargetS51::target_name()
{
	return "S51";
}

string TargetS51::target_descr()
{
	return "S51 8051 Simulator";
}

string TargetS51::device()
{
	return "unset";
}

void TargetS51::sendSim( string cmd )
{
	if(	bConnected )
	{
		if ( ! simout ) 
			return;
		fputs(cmd.c_str(),simout);
		fflush(simout);
	}
}

string TargetS51::recvSim(int timeout_ms )
{
	int		i=0;
	char	ch;
	string	resp;
	fd_set	input;
	struct	timeval	timeout;
	if( bConnected )
	{
		// Initialize the input set
		FD_ZERO( &input );
		FD_SET( sock, &input );
		fcntl(sock, F_SETFL,0);	// block if not enough characters available
		
		// Initialize the timeout structure
		timeout.tv_sec  = 0;		// n seconds timeout
		timeout.tv_usec = timeout_ms*1000;
		
		//char *cur_ptr = buf;
		int cnt=0, r, n;
			
		while(1)
		{
			// Do the select
			n = select( sock+1, &input, NULL, NULL, &timeout );
			if (n < 0)
			{
	//			perror("select failed");
				exit(-1);
				return resp;
			}
			else if (n == 0)
			{
	//			puts("TIMEOUT");
				return resp;
			}
			else
			{
				r = read( sock, &ch, 1 );
				resp+=ch;
			}
		}
	}
	return resp;
}
#if 0
bool TargetS51::load_file( string name )
{
	string s;
	sendSim("file \""+name+"\"\n");
	s = recvSim( 500 );
	
	if( s.find("Can't open")==0 )
		return false;
	else if( s.find("0 words read from ")==0 )
		return false;	// wrong file type
	else if( s.find(" words read from ")>0 )
		return true;	// was an intel hex file so loaded correctly
	else
		return false;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Device control
///////////////////////////////////////////////////////////////////////////////

void TargetS51::reset()
{
	sendSim("reset\n");
	recvSim( 250 );
}

/** step over one assembly instruction
	\returns PC
*/
uint16_t TargetS51::step()
{
	sendSim("step\n");
	recvSim( 100 );
	return read_PC();
}

bool TargetS51::add_breakpoint(uint16_t addr)
{
	char cmd[16];
	snprintf(cmd,16,"break 0x%x\n",addr);
	sendSim(cmd);
	string r = recvSim(100);
	return true;
}

bool TargetS51::del_breakpoint(uint16_t addr)
{
	char cmd[16];
	snprintf(cmd,16,"clear 0x%x\n",addr);
	sendSim(cmd);
	string r = recvSim(100);
	if( r.find("No breakpoint at")==0 )
		return false;
	if( r.find("Breakpoint")==0 )
		return true;
//	cout <<"bool TargetS51::del_breakpoint(uint16_t addr) ERROR ["<<r<<"]"<<endl;
	// fixme we should test for the correct response here... return false;
	return true;
}

void TargetS51::clear_all_breakpoints()
{
	// for the simulator we need to clear all at the simulator level
	// in case we have connected to an already sued simulator
	// any other breakpoints will have been cleared by calling the BreakpointMgr
	sendSim("info breakpoints\n");
	string s = recvSim( 100 );

	// parse the table deleting as we go
	string line;
	int pos=0, epos, bpid;
	char cmd[16];
	while(1)
	{
		epos = s.find('\n',pos);
		if(pos>=epos)
			return;
		line = s.substr( pos, epos-pos );
		if(line[0]!='N')	// skip header
		{
			// first column is the breakpoint number
			bpid = strtoul(line.substr(0,line.find(' ')).c_str(),0,10);
			cout <<"["<<line<<"]"<<endl;
			snprintf(cmd,16,"delete %i\n",bpid);
			sendSim( cmd ); 
			cout << recvSim( 100 ) << endl;;
		}
		pos = epos+1;
	}
	
	
}

void TargetS51::run_to_bp(int ignore_cnt)
{
	sendSim("go\n");
	recvSim(100);
}

bool TargetS51::is_running()
{
}

void TargetS51::stop()
{
	sendSim("stop\n");
	recvSim(100);
}


///////////////////////////////////////////////////////////////////////////////
// Memory reads
///////////////////////////////////////////////////////////////////////////////

void TargetS51::read_data( uint8_t start, uint8_t len, unsigned char *buf )
{
	char cmd[16];
	snprintf(cmd,16,"di 0x%02x 0x%02x\n",start, (start+len-1) );
	sendSim( cmd );
	string s = recvSim( 250 );
	cout <<"got : "<<s<<endl;
	parse_mem_dump( s, buf, len );
}

void TargetS51::read_sfr( uint8_t addr, uint8_t len, unsigned char *buf )
{
	char cmd[16];
	snprintf(cmd,16,"ds 0x%02x 0x%02x\n",addr, (addr+len-1) );
	sendSim( cmd );
	parse_mem_dump( recvSim( 250 ), buf, len );
}

void TargetS51::read_xdata( uint16_t addr, uint16_t len, unsigned char *buf )
{
	char cmd[16];
	snprintf(cmd,16,"ds 0x%04x 0x%04x\n",addr, (addr+len-1) );
	sendSim( cmd );
	parse_mem_dump( recvSim( 250 ), buf, len );
}

void TargetS51::read_code( uint16_t addr, uint16_t len, unsigned char *buf )
{
	char cmd[16];
	snprintf(cmd,16,"dch 0x%04x 0x%04x\n",addr, (addr+len-1) );
	sendSim( cmd );
	parse_mem_dump( recvSim( 250 ), buf, len );
}

uint16_t TargetS51::read_PC()
{
	sendSim("pc\n");
	string r = recvSim( 250 );
	int pos = r.find("0x",0);
	int npos = r.find(' ',pos);
	return strtoul( r.substr( pos, npos-pos ).c_str(), 0, 16 );
}

///////////////////////////////////////////////////////////////////////////////
// Memory writes
///////////////////////////////////////////////////////////////////////////////

void TargetS51::write_data( uint8_t addr, uint8_t len, unsigned char *buf )
{
	write_mem("iram",addr,len,buf);
}

void TargetS51::write_sfr( uint8_t addr, uint8_t len, unsigned char *buf )
{
	write_mem("sfr",addr,len,buf);
}

void TargetS51::write_xdata( uint16_t addr, uint16_t len, unsigned char *buf )
{
	write_mem("xram",addr,len,buf);
} 

void TargetS51::write_code( uint16_t addr, uint16_t len, unsigned char *buf )
{
	write_mem("rom",addr,len,buf);
}

void TargetS51::write_PC( uint16_t addr )
{
	char cmd[16];
	snprintf(cmd,16,"pc 0x%04x",addr);
	sendSim(cmd);
	recvSim( 250 );
}



void print_buf( unsigned char *buf, int len )
{
	const int PerLine = 16;
	int i, addr;

	for( addr=0; addr<len; addr += PerLine )
	{
		printf("%04x\t",addr);
		// print each hex byte		
		for( i=0; i<PerLine; i++ )
			printf("%02x ",buf[addr+i]);
		printf("\t");
		for( i=0; i<PerLine; i++ )
			putchar( (buf[addr+i]>='0' && buf[addr+i]<='z') ? buf[addr+i] : '.' );
		putchar('\n');
	}
}

bool TargetS51::command( string cmd )
{
	unsigned char buf[256];
	if(cmd.compare("test")==0)
	{
		read_data(0,0x80,buf);
		print_buf(buf,0x80);
	}
	else
	{
		sendSim( cmd+"\n" );
		cout << recvSim( 250 )<<endl;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Helper routines
// simplify communications with the simulator
///////////////////////////////////////////////////////////////////////////////

/** Take a memory dump from the dimulator and parse it into the supplied buffer.
	Format: 
		0x08 00 bc d4 b6 3d 1c 3e 22 ....=.>"
		reads with less bytes will return a partial row,
		those with more will return multiple rows
	note the address can use more characters in the event of 16 bit so we must parse it
*/
void TargetS51::parse_mem_dump( string dump, unsigned char *buf, int len )
{
	int pos=0, epos=0;
	string line;
	int total_rows = (len%8==0) ? len/8 : len/8+1;
	int ofs=0;
	// split returned string up into lines and process independantly
	for( int row=0; row<total_rows; row++ )
	{
		epos = dump.find('\n',pos);
		line = dump.substr( pos, epos-pos );
		// parse the line
		ofs = line.find(' ',0) + 1;		// skip over address
		// todo handle lines without 8 requests
		for(int j=0; j<8; j++)
		{
			*buf++ = strtol( line.substr( ofs, 2).c_str(), 0, 16 );
			ofs +=3;
		}
		pos = epos+1;
	}
}

/** write to a specific memory area on the simulator.
	the area provided must match a memory area recognised by the simulator
	eg xram, rom, iram, sfr
*/
void TargetS51::write_mem( string area, uint16_t addr, uint16_t len,
						   unsigned char *buf )
{
	const int MAX_CHUNK = 16;
	string cmd;
	char s[32];

	for( ADDR offset=0; offset<len; offset+=MAX_CHUNK )
	{
		snprintf( s, 32, "set mem %s 0x%04X", area.c_str(), addr+offset  );
		cmd = s;
		for( int i=0; ( i<MAX_CHUNK && (offset+i)<len); i++ )
		{
			snprintf(s,32," 0x%02x", buf[offset+i] );
			cmd += s;
		}
		cmd += "\n";
		sendSim( cmd );
		recvSim( 250 );	// the sim sends back the result in memory, we ignore it
	}
}
