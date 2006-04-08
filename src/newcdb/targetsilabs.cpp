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
#include <iostream>
#include <pthread.h>
#include "targetsilabs.h"
#include "ec2drv.h"

using namespace std;

TargetSiLabs::TargetSiLabs()
	: Target(), running(false)
{
}


TargetSiLabs::~TargetSiLabs()
{
	running = false;
	pthread_join( run_thread, NULL );	// wait for thread to stop
}


bool TargetSiLabs::connect()
{
	if( ec2_connect( &obj, debugger_port.c_str() ) )
	{
		return true;
	}
	else
		return false;
}

bool TargetSiLabs::disconnect()
{
	ec2_disconnect( &obj );
}

bool TargetSiLabs::is_connected()
{
	return false;
}
		
string TargetSiLabs::port()
{
	return debugger_port;
}

bool TargetSiLabs::set_port( string port )
{
	debugger_port = port;
	return true;
}

string TargetSiLabs::target_name()
{
	return "SL51";
}

string TargetSiLabs::target_descr()
{
	return "Silicon Laboritories Debug adapter EC2 / EC3";
}

string TargetSiLabs::device()
{
	running = false;
	return	obj.dev ? obj.dev->name : "unknown";
}


///////////////////////////////////////////////////////////////////////////////
// Device control
///////////////////////////////////////////////////////////////////////////////

void TargetSiLabs::reset()
{
	ec2_target_reset( &obj );
}

uint16_t TargetSiLabs::step()
{
	return ec2_step( &obj );
}

bool TargetSiLabs::add_breakpoint(uint16_t addr)
{
	return ec2_addBreakpoint( &obj, addr );
}

bool TargetSiLabs::del_breakpoint(uint16_t addr)
{
	return ec2_removeBreakpoint( &obj, addr );
}

void TargetSiLabs::clear_all_breakpoints()
{
}

void *TargetSiLabs::run_thread_func( void *ptr )
{
	TargetSiLabs *t = (TargetSiLabs*)ptr;
//	while( *(bool*)ptr )
	while( t->running )
	{
		ec2_target_run_bp( &(t->obj), (BOOL*)&(t->running) );
		cout <<"run";
	}
	pthread_exit(0);
}

void TargetSiLabs::run_to_bp()
{
	cout << "starting a run now..."<<endl;
	running = true;
	pthread_create( &run_thread, NULL, (void*(*)(void*))&run_thread_func, this );
	
	pthread_join( run_thread, NULL );	// wait for thread to stop
	
#if 0
	ec2_target_run_bp( &obj, 0 );		///< \fixme we need a thread to handle the running task yet allow the command line to continue.
#endif
}

bool TargetSiLabs::is_running()
{
}

void TargetSiLabs::stop()
{
	running = false;
	pthread_join( run_thread, NULL );	// wait for thread to stop
}


///////////////////////////////////////////////////////////////////////////////
// Memory reads
///////////////////////////////////////////////////////////////////////////////

void TargetSiLabs::read_data( uint8_t addr, uint8_t len, unsigned char *buf )
{
	ec2_read_ram( &obj, (char*)buf, addr, len );
}

void TargetSiLabs::read_sfr( uint8_t addr, uint8_t len, unsigned char *buf )
{
	for( uint16_t offset=0; offset<len; offset++ )
		ec2_read_sfr( &obj, (char*)buf[offset], addr+offset );
}

void TargetSiLabs::read_xdata( uint16_t addr, uint16_t len, unsigned char *buf )
{
	ec2_read_xdata( &obj, (char*)buf, addr, len );
}

void TargetSiLabs::read_code( uint16_t addr, uint16_t len, unsigned char *buf )
{
	ec2_read_flash( &obj, (char*)buf, addr, len );
}

uint16_t TargetSiLabs::read_PC()
{
	ec2_read_pc( &obj );
}

///////////////////////////////////////////////////////////////////////////////
// Memory writes
///////////////////////////////////////////////////////////////////////////////
void TargetSiLabs::write_data( uint8_t addr, uint8_t len, unsigned char *buf )
{
	ec2_write_ram( &obj, (char*)buf, addr, len );
}

void TargetSiLabs::write_sfr( uint8_t addr, uint8_t len, unsigned char *buf )
{
	for( uint16_t offset=0; offset<len; offset++ )
		ec2_write_sfr( &obj, buf[offset], addr+offset );
}

void TargetSiLabs::write_xdata( uint16_t addr, uint16_t len, unsigned char *buf )
{
	ec2_write_xdata( &obj, (char*)buf, addr, len );
}

void TargetSiLabs::write_code( uint16_t addr, uint16_t len, unsigned char *buf )
{
	cout << "Writing to flash with auto erase as necessary" << endl;
	printf("\tWriting %i bytes at 0x%04x\n",len,addr);
	if( ec2_write_flash_auto_erase( &obj, (char*)buf, addr, len ) )
		cout << "Flash write successful." << endl;
	else
		cout << "ERROR: Flash write Failed." << endl;
}

void TargetSiLabs::write_PC( uint16_t addr )
{
	ec2_set_pc( &obj, addr );
}
