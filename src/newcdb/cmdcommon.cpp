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
#include <string.h>
#include "cmdcommon.h"
#include "target.h"
#include "symtab.h"
#include "cdbfile.h"
#include "breakpointmgr.h"
#include "linespec.h"

extern Target *target;
extern Target *target_drivers[];
using namespace std;

bool CmdVersion::show( string cmd )
{
	if(cmd.length()==0)
	{
		cout << "\nVersion 0.1 (jelly)\n"
			 << "Compiled on "<<__DATE__<<" at "<<__TIME__<<"\n"
			 << endl;
		return true;
	}
	return false;
}

bool CmdWarranty::show( string cmd )
{
	if(cmd.length()==0)
	{	
		cout << "                            NO WARRANTY\n"
		"\n"
		"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
		"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
		"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
		"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
		"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
		"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
		"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
		"REPAIR OR CORRECTION.\n"
		"\n"
		"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
		"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
		"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
		"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
		"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
		"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
		"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
		"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
		"POSSIBILITY OF SUCH DAMAGES.\n"
		"\n";
		return true;
	}
	return false;
}

bool CmdCopying::show( string cmd )
{
//	string s = getenv("_");
	if( cmd.length()==0 )
	{
		execl("/usr/bin/less","less","copying",NULL);
		return true;
	}
	return false;	
}

/** top level help
*/
bool CmdHelp::parse( string cmd )
{
	if( cmd.compare("help")==0 )
	{
		printf("Help\n\n");
		cout << "List of classes of commands:\n"
				"\n"
				"breakpoints -- Making program stop at certain points\n"
				"data -- Examining data\n"
				"files -- Specifying and examining files\n"
				"running -- Running the program\n"
				"stack -- Examining the stack\n"
				"status -- Status inquiries\n"
				<<endl;
		return true;
	}
	return false;
}


/** Redirect commands directly to the target driver
	Ideaslly not much should be done through this interface in the interest of
	portability to other targer devices
*/
bool CmdTarget::direct( string cmd )
{
	return target->command( cmd );
}

bool CmdTarget::set( string cmd )
{
	if( cmd.find("port ")==0 )
	{
		cout << "set port '" <<cmd.substr(5)<<"'"<< endl;
		target->set_port(cmd.substr(5));
		return true;
	}
	else if( cmd.find("device ")==0 )
	{
		cout << "set device"<<cmd<< endl;
		return true;
	}
	else if( cmd.compare("connect")==0 )
	{
		target->connect();
		return true;
	}
	else if( cmd.compare("disconnect")==0 )
	{
		target->disconnect();
		return true;
	}
	else
	{
		int i=0;
		while( target_drivers[i] )
		{
			if( target_drivers[i]->target_name()==cmd )
			{
				// disconnect from current target and select new one
				target->disconnect();
				target = target_drivers[i];
				// Don't connect yet, user probably needs to setup ports before cmmanding a connect
				return true;
			}
			i++;
		}
	}
	return false;
}

bool CmdTarget::info( string cmd )
{
	if( cmd.find("port")==0 )
	{
		cout <<"port = \""<<"/dev/ttyS0"<<"\"."<<endl;
		return true;
	}
	else if( cmd.find("device")==0 )
	{
		cout <<"device = \""<<"80C51"<<"\"."<<endl;
		return true;
	}
	else if( cmd.length()==0 )
	{
		cout <<"Target = '"<<target->target_name()<<"'\t"
				<<"'"<<target->target_descr()<<"'"<<endl;
		cout <<"Port = '"<<target->port()<<"'"<<endl;
		cout <<"Device = '"<<target->device()<<"'"<<endl;
		return true;
	}
	return false;
}

bool CmdTarget::show( string cmd )
{
	if( cmd.compare("connect")==0 )
	{
		cout << (target->is_connected() ? 
				"Connected." : "Disconnected.") << endl;
		return true;
	}
	return false;
}


/** cause the target to step one source level instruction
*/
bool CmdStep::directnoarg()
{
	// @TODO add loop and checks for hitting next cmd level instruction.
	//		If a breakpoint is available use it reather than rrepeated calls to step
	// 		if the instruction is an if using a breakpoint may be problematic
	// Can't fully implement "step" until the c file line / address mapping is complete
	int16_t addr = target->step();
	bp_mgr.stopped(addr);
	printf("PC = 0x%04x\n",target->read_PC());
	
	
	return true;
}

/** cause the target to step one assembly level instruction
 */
bool CmdStepi::directnoarg()
{
	printf("PC = 0x%04x\n",target->step());
	return true;
}

/**	Continue execution from the current address
	if there is a breakpoint on the current address it is ignored.
	optional parameter specifies a further number of breakpoints to ignore
*/
bool CmdContinue::direct( string cmd )
{
	return false;
}

/**	Continue execution from the current address and stop at next breakpoint
*/
bool CmdContinue::directnoarg()
{
	printf("Continuing.\n");
	target->run_to_bp();
	bp_mgr.stopped_at( target->read_PC() );
	return true;
}



/** open a new cdb file for debugging
	all associated files must be in the same directory
*/
bool CmdFile::direct( string cmd)
{
	symtab.clear();
	CdbFile cdbfile(&symtab);
	cdbfile.open( cmd+".cdb" );
	bp_mgr.clear_all();
	return target->load_file(cmd+".ihx");
}


/** list a section of the program
	list linenum
list function
list
list -
list linespec
list first,last
list ,last
list first,
list +
list -

linespec:
	number
	+offset
	-offset
	filename:number 
	function
	filename:function 
	*address
*/
bool CmdList::direct( string cmd )
{
	cout <<"NOT implemented ["<<cmd<<"]"<<endl;
	return true;
}

bool CmdList::directnoarg()
{
	cout <<"NOT implemented"<<endl;
	return true;
}


bool CmdPWD::directnoarg()
{
	printf("Working directory %s.\n","dir here");	// @TODO replace "dir here with current path"
	return true;
}


/** info files and info target are synonymous; both print the current target
*/
bool CmdFiles::info( string cmd )
{
	cout <<"Symbols from \"/home/ricky/projects/ec2cdb/debug/src/test\"."<<endl;	// @TODO put correct pathe in here
	return true;
}


bool CmdSource::info( string cmd )
{
	if(cmd.length()==0)
	{
		cout << "Source files for which symbols have been read in:"<<endl<<endl;
		cout <<"test.c, test.asm"<<endl;
		return true;
	}
	return false;
}

bool CmdSources::info( string cmd )
{
	if(cmd.length()==0)
	{
		cout <<"Current source file is test.c"<<endl;
		cout <<"Located in test.c"<<endl;
		cout <<"Contains 11 lines."<<endl;
		cout <<"Source language is c."<<endl;
		return true;
	}
	return false;
}


/**  	
	Examples

	(gdb) info line m4_changequote
	Line 895 of "builtin.c" starts at pc 0x634c and ends at 0x6350.
	
	We can also inquire (using *addr as the form for linespec) what source line covers a particular address:

	(gdb) info line *0x63ff
	Line 926 of "builtin.c" starts at pc 0x63e4 and ends at 0x6404.
*/
bool CmdLine::info( string cmd )
{
//	if( cmd.find(' ')>=0 || cmd.length()==0 )
//		return false;	// cmd must be just one word
	if( cmd.length()==0 )
		return false;
	LineSpec ls;
	
	if( ls.set( cmd ) )
	{
		printf("Line %i of \"%s\" starts at pc 0x%04x and ends at 0x%04x.\n",
				ls.line(),
				ls.file().c_str(),
				ls.addr(),
				ls.end_addr()
			  );
		// test.c:19:1:beg:0x000000f8
		printf("  %s:%i:%i:beg:0x%08x\n",
			   ls.file().c_str(),
			   ls.line(),
			   1,				// what should this be?
			   ls.addr()
			  );
		return true;
	}
	return false;
}


extern string prompt;
bool CmdPrompt::set( string cmd )
{
	prompt = cmd[cmd.length()-1]==' ' ? cmd : cmd+" ";
	return true;
}

bool CmdRun::directnoarg()
{
	cout << "Starting program" << endl;
	if(!bp_mgr.set_breakpoint("main",true))
	{
		cout <<" failed to set main breakpoint!"<<endl;
	}
	target->run_to_bp();
	return true;
}

bool CmdStop::directnoarg()
{
	cout << "Starting target" << endl;
	target->stop();
	return true;
}

bool CmdFinish::directnoarg()
{
	cout << "Finishing current function" << endl;
	// @fixme set a breakpoint at the end of the current function
	//bp_mgr.set_breakpoint(
	return true;
}

