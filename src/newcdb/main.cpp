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
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <list>
#include <signal.h>
#include <getopt.h>
#include <iostream>
#include <fstream>

#include "cdbfile.h"
#include "parsecmd.h"
#include "cmdcommon.h"
#include "cmdbreakpoints.h"
#include "cmddisassemble.h"
#include "cmdmaintenance.h"
#include "targetsilabs.h"
#include "targets51.h"


#ifdef HAVE_LIBREADLINE
	#if defined(HAVE_READLINE_READLINE_H)
		#include <readline/readline.h>
	#elif defined(HAVE_READLINE_H)
		#include <readline.h>
	#else /* !defined(HAVE_READLINE_H) */
		extern char *readline ();
	#endif /* !defined(HAVE_READLINE_H) */
	char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
    /* no readline */
	#warning no readline found, using simple substute:
	char *readline( const char *prompt )
	{
		const size_t size = 255;
		char *line = (char*)malloc(size);
		printf(prompt);
		if(line)
		{
			fgets(line, size, stdin);
			// strip off any line ending
			int p = strlen(line)-1;
			while( line[p]=='\n' || line[p]=='\r' )
				line[p--] = 0;
		}
		return line;
	}
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
	#if defined(HAVE_READLINE_HISTORY_H)
		#include <readline/history.h>
	#elif defined(HAVE_HISTORY_H)
		#include <history.h>
	#else /* !defined(HAVE_HISTORY_H) */
		extern void add_history();
		extern int write_history();
		extern int read_history();
	#endif /* defined(HAVE_READLINE_HISTORY_H) */
#else
    /* no history */
	#warning No history support
	void add_history( const char *string ) {}
	int write_history()	{return 0;}
	int read_history()	{return 0;}
#endif /* HAVE_READLINE_HISTORY */


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

void quit()
{
	target->stop();
	target->disconnect();
}


bool parse_cmd( string ln )
{
	if( ln.compare("quit")==0 )
	{
		target->disconnect();
		exit(0);
	}
	ParseCmd::List::iterator it;
	for( it=cmdlist.begin(); it!=cmdlist.end(); ++it)
	{
		if( (*it)->parse(ln) )
			return true;
	}
	return ln.length()==0;	// anything left with length >0 is bad.
}

bool process_cmd_file( string filename )
{
	string line;
	ifstream cmdlist( filename.c_str() );
	if( !cmdlist.is_open() )
		return false;	// failed to open command list file

	while( !cmdlist.eof() )
	{
		std::getline(cmdlist, line);
		cout << line << endl;
		if( !parse_cmd(line) )
			cout << "Bad Command" << endl;
	}
	cmdlist.close();
	return true;
}



int main(int argc, char *argv[])
{
	sighandler_t	old_sig_int_handler;


	cout << "newcdb, new ec2cdb based on c++ source code" << endl;
	old_sig_int_handler = signal( SIGINT, sig_int_handler );
	atexit(quit);
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





	while (1)
	{
		// command line option parsing
		static struct option long_options[] =
		{
			{"command", required_argument, 0, 'c'},
			{"ex", required_argument, 0, 'e'},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;
		int c = getopt_long_only( argc, argv, "", long_options, &option_index);
		/* Detect the end of the options. */
		if( c == -1 )
			break;

		switch (c)
		{
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case 'c':
				// Command file
				cout << "Processing command file '" << optarg << "'" << endl;
				if( process_cmd_file(optarg) )
					cout << "ERROR coulden't open command file" << endl;
				break;
			case 'e':
				// Command file
				cout << "Executing command " << optarg << endl;
				add_history(optarg);
				if( !parse_cmd(optarg) )
					printf("Bad Command.");
				break;
			default:
				abort();
		}
	}
	/* Print any remaining command line arguments (not options). */
	if( optind < argc )
	{
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		putchar ('\n');
	}


	while(1)
	{
		bool ok=false;
		char *line = readline( prompt.c_str() );
		if(*line!=0)
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


