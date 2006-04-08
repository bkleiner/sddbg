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
using namespace std;
#include "cmdbreakpoints.h"
#include "breakpointmgr.h"
#include "target.h"
extern Target *target;

bool CmdBreakpoints::show( string cmd )
{
	//cout <<"breakpoints are..."<<endl;
	return false;
}


bool CmdBreakpoints::info( string cmd )
{
	bp_mgr.dump();
	
	return true;
}


/** Add a breakpoint

	formats:
		break file.c:123		add breakpoint to file.c at line 123
		break main				add a breakpoint at the start of the main function
		break 0x1234			add abreakpoint at 0x1234

  user break point location specification can be of the following
       forms
       a) <nothing>        - break point at current location
       b) lineno           - number of the current module
       c) filename:lineno  - line number of the given file
       e) filename:function- function X in file Y (useful for static functions)
       f) function         - function entry point
       g) *addr            - break point at address 

*/
bool CmdBreak::direct( string cmd )
{
	return bp_mgr.set_breakpoint( cmd );
}

bool CmdBreak::directnoarg()
{
	cout <<"Adding a breakpoint at the current location"<<endl;
	return bp_mgr.set_bp( target->read_PC(), false );
}


bool CmdBreak::help( string cmd )
{
	if( cmd.length()==0)
	{
		cout<<"Set breakpoint at specified line or function.\n"
			"Argument may be line number, function name, or \"*\" and an address.\n"
			"If line number is specified, break at start of code for that line.\n"
			"If function is specified, break at start of code for that function.\n"
			"If an address is specified, break at that exact address.\n"
			"With no arg, uses current execution address of selected stack frame.\n"
			"This is useful for breaking on return to a stack frame.\n"
			"\n"
			"Multiple breakpoints at one place are permitted, and useful if conditional.\n"
			"\n"
			"Do \"help breakpoints\" for info on other commands dealing with breakpoints.\n"
			<<endl;
	}
	return true;
}


bool CmdTBreak::direct( string cmd )
{
	return bp_mgr.set_breakpoint( cmd, false );
}

bool CmdTBreak::directnoarg()
{
	cout <<"Adding a temporary breakpoint at the current location"<<endl;
	return bp_mgr.set_bp( target->read_PC(), true );
}

bool CmdTBreak::help( string cmd )
{
}

bool CmdClear::direct( string cmd )
{
	return bp_mgr.clear_breakpoint( cmd );
}

bool CmdClear::directnoarg()
{
	return bp_mgr.clear_breakpoint_addr( target->read_PC() );
}

bool CmdDelete::direct( string cmd )
{
//	if( cmd.find(" ")<0 )
//	{
		bp_mgr.clear_breakpoint_id( strtoul( cmd.c_str(), 0, 10 ) );
		return true;
//	}
	return false;
}


bool CmdEnable::direct( string cmd )
{
	bp_mgr.enable_bp( strtoul( cmd.c_str(), 0, 10 ) );
	return true;
	
}

bool CmdDisable::direct( string cmd )
{
	bp_mgr.disable_bp( strtoul( cmd.c_str(), 0, 10 ) );
	return true;
	
}

