/***************************************************************************
 *   Copyright (C) 2006 by Ricky White                                     *
 *   rickyw@sourceforge.net                                                *
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <list>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include "cdbfile.h"
#include "parsecmd.h"
#include "cmdcommon.h"
#include "cmdbreakpoints.h"
#include "cmddisassemble.h"
#include "cmdmaintenance.h"
#include "targetsilabs.h"
#include "targets51.h"
using namespace std;

ParseCmd::List cmdlist;
Target *target;
Target *target_drivers[] = { new TargetS51(),  new TargetSiLabs, 0 };
string prompt;


void sig_int_handler(int)
{
	target->stop();
	cout << endl << prompt;
}


int main(int argc, char *argv[])
{
	sighandler_t	old_sig_int_handler;
	
	cout << "newcdb, new ec2cdb based on c++ source code" << endl;
	old_sig_int_handler = signal( SIGINT, sig_int_handler );
#if 0
	if( argc!=2 )
	{
		cout << "incorrect number of arguments"<<endl;
		return -1;
	}
#endif
	
	target = new TargetS51();
	target->connect();
			
//	SymTab symtab;
	CdbFile f(&symtab);
//	f.open( argv[1] );
//	f.open( "test.cdb" );
//	symtab.dump();

	FILE *badcmd = fopen("badcmd.log","w");
	
	// add commands to list
	cmdlist.push_back( new CmdShowSetInfoHelp() );
	cmdlist.push_back( new CmdVersion() );
	cmdlist.push_back( new CmdWarranty() );
	cmdlist.push_back( new CmdCopying() );
	cmdlist.push_back( new CmdHelp() );
	cmdlist.push_back( new CmdPrompt() );
	cmdlist.push_back( new CmdBreakpoints() );
	cmdlist.push_back( new CmdBreak() );
	cmdlist.push_back( new CmdTBreak() );
	cmdlist.push_back( new CmdDelete() );
	cmdlist.push_back( new CmdEnable() );
	cmdlist.push_back( new CmdDisable() );
	cmdlist.push_back( new CmdClear() );
	cmdlist.push_back( new CmdTarget() );
	cmdlist.push_back( new CmdStep() );
	cmdlist.push_back( new CmdStepi() );
	cmdlist.push_back( new CmdNext() );
	cmdlist.push_back( new CmdNexti() );
	cmdlist.push_back( new CmdContinue() );
	cmdlist.push_back( new CmdFile() );
	cmdlist.push_back( new CmdFiles() );
	cmdlist.push_back( new CmdList() );
	cmdlist.push_back( new CmdPWD() );
	cmdlist.push_back( new CmdSource() );
	cmdlist.push_back( new CmdSources() );
	cmdlist.push_back( new CmdLine() );
	cmdlist.push_back( new CmdRun() );
	cmdlist.push_back( new CmdStop() );
	cmdlist.push_back( new CmdFinish() );
	cmdlist.push_back( new CmdDisassemble() );
	cmdlist.push_back( new CmdX() );
	cmdlist.push_back( new CmdMaintenance() );
	cmdlist.push_back( new CmdPrint() );
	cmdlist.push_back( new CmdRegisters() );
	string ln;
	prompt = "(newcdb) ";
	while(1)
	{
		bool ok=false;
		char *line = readline( prompt.c_str() );
		add_history(line);
		ln = line;
		free(line);
		fwrite((ln+'\n').c_str(),1,ln.length()+1, badcmd);
		if( ln.compare("quit")==0 )
		{
			signal( SIGINT, old_sig_int_handler );
			target->disconnect();
			fclose(badcmd);
			return 0;
		}
		ParseCmd::List::iterator it;
		for( it=cmdlist.begin(); it!=cmdlist.end(); ++it)
		{
			if( (*it)->parse(ln) )
			{
				ok = true;
				break;
			}
		}
		if( !ok && ln.length()>0)
		{
			fwrite(("BAD: "+ln+'\n').c_str(),1,ln.length()+1, badcmd);
			cout <<"bad command ["<<ln<<"]"<<endl;
		}
		
		
	}
	fclose(badcmd);
	return EXIT_SUCCESS;
}


