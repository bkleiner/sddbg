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
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <list>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "cdbfile.h"
#include "cmdlist.h"
#include "dap_server.h"
#include "parsecmd.h"
#include "sddbg.h"
#include "targets51.h"
#include "targetsilabs.h"

#ifdef HAVE_LIBREADLINE
#if defined(HAVE_READLINE_READLINE_H)
#include <readline/readline.h>
#elif defined(HAVE_READLINE_H)
#include <readline.h>
#else  /* !defined(HAVE_READLINE_H) */
extern char *readline();
#endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
/* no readline */
#warning no readline found, using simple substute:
char *readline(const char *prompt) {
  const size_t size = 255;
  char *line = (char *)malloc(size);
  printf(prompt);
  if (line) {
    fgets(line, size, stdin);
    // strip off any line ending
    int p = strlen(line) - 1;
    while (line[p] == '\n' || line[p] == '\r')
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
#else  /* !defined(HAVE_HISTORY_H) */
extern void add_history();
extern int write_history();
extern int read_history();
#endif /* defined(HAVE_READLINE_HISTORY_H) */
#else
/* no history */
#warning No history support
void add_history(const char *str) {}
int write_history() { return 0; }
int read_history() { return 0; }
#endif /* HAVE_READLINE_HISTORY */

std::string prompt = "(sddbg) ";

ParseCmdList cmdlist;
DbgSession gSession;

void sig_int_handler(int) {
  gSession.target()->stop();
  std::cout << std::endl
            << prompt;
}

void quit() {
  gSession.target()->stop();
  gSession.target()->disconnect();
}

bool parse_cmd(std::string ln) {
  if (ln.compare("quit") == 0) {
    gSession.target()->disconnect();
    exit(0);
  }
  return cmdlist.parse(ln);
}

bool process_cmd_file(std::string filename) {
  std::ifstream cmdfile(filename.c_str());
  if (!cmdfile.is_open())
    return false; // failed to open command list file

  std::string line;
  while (!cmdfile.eof()) {
    std::getline(cmdfile, line);
    std::cout << line << std::endl;
    if (!parse_cmd(line))
      std::cout << "Bad Command" << std::endl;
  }

  cmdfile.close();
  return true;
}

int main(int argc, char *argv[]) {
  void (*old_sig_int_handler)(int);

  old_sig_int_handler = signal(SIGINT, sig_int_handler);
  atexit(quit);
  //	target = new TargetS51();
  //	target->connect();

  CdbFile f(&gSession);

  std::fstream badcmd;

  int quiet_flag = 0;
  int help_flag = 0;
  int dap_flag = 0;

  // command line option parsing
  static struct option long_options[] = {
      {"command", required_argument, 0, 'c'},
      {"ex", required_argument, 0, 'e'},
      {"dbg-badcmd", required_argument, 0, 'b'},
      {"dap", no_argument, &dap_flag, 1},
      {"q", no_argument, &quiet_flag, 1},
      {"help", no_argument, &help_flag, 1},
      {0, 0, 0, 0},
  };

  while (1) {
    /* getopt_long stores the option index here. */
    int option_index = 0;
#ifdef __GLIBC__
    int c = getopt_long_only(argc, argv, "", long_options, &option_index);
#else
    int c = getopt_long(argc, argv, "", long_options, &option_index);
#endif
    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;
    case 'b':
      if (badcmd.is_open()) {
        break;
      }
      badcmd.open(optarg);
      if (!badcmd.is_open()) {
        std::cerr << "ERROR: Failed to open " << optarg
                  << "for bad command logging." << std::endl;
      }
      break;
    case 'c':
      // Command file
      std::cout << "Processing command file '" << optarg << "'" << std::endl;
      if (process_cmd_file(optarg))
        std::cout << "ERROR coulden't open command file" << std::endl;
      break;
    case 'e':
      // Command file
      std::cout << "Executing command " << optarg << std::endl;
      add_history(optarg);
      if (!parse_cmd(optarg))
        printf("Bad Command.");
      break;
    default:
      abort();
    }
  }

  if (help_flag) {
    std::cout << "sddbg, new ec2cdb based on c++ source code" << std::endl
              << "Help:" << std::endl
              << "\t-command=<file>   Execute the commands listed in the supplied\n"
              << "\t                  file in ordser top to bottom.\n"
              << "\t-ex=<command>     Execute the command as if it were typed on \n"
              << "\t                  the command line of sddbg once sddbg\n"
              << "\t                  starts.  You can have multiple -ex options\n"
              << "\t                  and they will be executed in order left to\n"
              << "\t                  right.\n"
              << "\t-fullname         Sets the line number output format to two `\\032'\n"
              << "\t                  characters, followed by the file name, line number\n"
              << "\t                  and character position separated by colons, and a newline.\n"
              << "\t-q                Suppress the startup banner\n"
              << "\t--dbg-badcmd=file Log all bad commands to file\n"
              << "\t--help            Display this help"
              << std::endl
              << std::endl;
    exit(0);
  }

  if (dap_flag) {
    DapServer server;
    server.start();
    return server.run();
  }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    while (optind < argc)
      parse_cmd(std::string("file ") + argv[optind++]);
  }

  if (!quiet_flag) {
    std::cout << "sddbg, new ec2cdb based on c++ source code" << std::endl;
  }

  while (1) {
    std::string ln = readline(prompt.c_str());

    if (!ln.empty()) {
      add_history(ln.c_str());
    }

    if (badcmd.is_open()) {
      badcmd << ln << "\n";
    }

    if (ln.compare("quit") == 0) {
      signal(SIGINT, old_sig_int_handler);
      gSession.target()->disconnect();
      return 0;
    }

    const bool ok = cmdlist.parse(ln);
    if (!ok && (ln.length() > 0)) {
      std::cout << "bad command [" << ln << "]" << std::endl;
      if (badcmd.is_open()) {
        badcmd << "BAD: " << ln << "\n"
               << std::flush;
      }
    }
  }

  return EXIT_SUCCESS;
}
