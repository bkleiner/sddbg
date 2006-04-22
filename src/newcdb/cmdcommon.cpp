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
#include "types.h"
#include "cmdcommon.h"
#include "target.h"
#include "module.h"
#include "cdbfile.h"
#include "breakpointmgr.h"
#include "linespec.h"
#include "contextmgr.h"

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
		printf("PC = 0x%04x\n",target->read_PC());
		
		context_mgr.dump();
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
	string module;
	LINE_NUM line;
	ADDR addr;
	// keep stepping over asm instructions until we hit another c function
	do
	{
		addr = target->step();
		bp_mgr.stopped(addr);
		context_mgr.set_context(addr);
	}
	while( !mod_mgr.get_c_addr( addr, module, line ) && 
			!target->check_stop_forced() );
	context_mgr.dump();
	return true;
}

/** cause the target to step one assembly level instruction
 */
bool CmdStepi::directnoarg()
{
	ADDR addr = target->step();
	bp_mgr.stopped(addr);
	context_mgr.set_context(addr);
	context_mgr.dump();
	return true;
}

/** Continue to the next source line in the current (innermost) stack frame.
	This is similar to step, but function calls that appear within the line of
	code are executed without stopping.
	Execution stops when control reaches a different line of code at the
	original stack level that was executing when you gave the next command.
	This command is abbreviated n.

	@FIXME: change from step implementation to proper next
*/
bool CmdNext::directnoarg()
{
	string module;
	LINE_NUM line;
	ADDR addr;
	// keep stepping over asm instructions until we hit another c function
	do
	{
		addr = target->step();
		bp_mgr.stopped(addr);
		context_mgr.set_context(addr);
	}
	while( !mod_mgr.get_c_addr( addr, module, line ) );
	context_mgr.dump();
	return true;
}

/** Execute one machine instruction, but if it is a function call, proceed until
	the function returns.

	@FIXME: change from stepi implementation to proper nexti
*/
bool CmdNexti::directnoarg()
{
	ADDR addr = target->step();
	bp_mgr.stopped(addr);
	context_mgr.set_context(addr);
	context_mgr.dump();
	return true;
}


/**	Continue execution from the current address
	if there is a breakpoint on the current address it is ignored.
	optional parameter specifies a further number of breakpoints to ignore
*/
bool CmdContinue::direct( string cmd )
{
	printf("Continuing.\n");
	int i = strtoul( cmd.c_str(), 0, 0);
	
	target->run_to_bp(i);
	bp_mgr.stopped(target->read_PC());
	context_mgr.set_context( target->read_PC() );
	context_mgr.dump();
	return true;
}

/**	Continue execution from the current address and stop at next breakpoint
*/
bool CmdContinue::directnoarg()
{
	printf("Continuing.\n");
	target->run_to_bp();
//	bp_mgr.stopped( target->read_PC() );
	context_mgr.set_context( target->read_PC() );
	context_mgr.dump();
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
	if( cmd.empty() )
	{
		/// @FIXME need a current context for this one...
		return true;
	}
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
		printf("\032\032%s:%i:%i:beg:0x%08x\n",
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
	target->reset();
	bp_mgr.reload_all();
	
	if(!bp_mgr.set_breakpoint("main",true))
		cout <<" failed to set main breakpoint!"<<endl;

	target->run_to_bp();
	ADDR addr = target->read_PC();
	bp_mgr.stopped(addr);
	context_mgr.set_context(addr);
	context_mgr.dump();
	return true;
}

bool CmdStop::directnoarg()
{
	cout << "Stopping target" << endl;
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

/** `print EXPR'
	`print /F EXPR'
		EXPR is an expression (in the source language).  By default the
		value of EXPR is printed in a format appropriate to its data type;
		you can choose a different format by specifying `/F', where F is a
		letter specifying the format; see  Output formats Output
		Formats.

	`print'
	`print /F'
		If you omit EXPR, GDB displays the last value again (from the
		"value history";  Value history Value History.).  This
		allows you to conveniently inspect the same value in an
		alternative format.
	
		A more low-level way of examining data is with the `x' command.  It
	examines data in memory at a specified address and prints it in a
	specified format.   Examining memory Memory.
	
		If you are interested in information about types, or about how the
	fields of a struct or a class are declared, use the `ptype EXP' command
	rather than `print'.   Examining the Symbol Table Symbols.

	\param expr	expression to display
*/
bool CmdPrint::direct( string expr )
{
	// figure out where we are
//	context_mgr.
	SymTab::SYMLIST::iterator it;
	Symbol::SCOPE scope;
	
	ContextMgr::Context c = context_mgr.get_current();
	cout << "current context :"<<endl;
	context_mgr.dump();
	
	cout << "displaythe variable now"<<endl;
	/// @FIXME scope and file etc need to come from current context
	symtab.getSymbol( "file", scope, expr,it );
	
	return true;
}

