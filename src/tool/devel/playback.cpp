/***************************************************************************
 *   Copyright (C) 2006 by Ricky White   *
 *   rickyw@neatstuff.co.nz   *
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

#include "playback-comms.h"
#include <cstdlib>
#include <getopt.h>
#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "ec2emulator" << std::endl;
  std::string port, file;
  int help_flag;
  static struct option long_options[] =
      {
          {"help", no_argument, &help_flag, 'h'},
          {"port", required_argument, 0, 'p'},
          {"file", required_argument, 0, 'f'},
          {0, 0, 0, 0}};
  int option_index = 0;
  int c;
  while (1) {
    c = getopt_long(argc, argv, "", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 0: // set a flag, nothing to do
      break;
    case 'p': // port
      port = optarg;
      break;
    case 'f': // file
      file = optarg;
      break;
    default:
      printf("unexpected option\n");
      break;
    }
  };

  std::cout << "load file " << file << std::endl;
  Ec2Sim sim;
  sim.loadFile(file);
  sim.open_port(port);
  sim.go();

  return EXIT_SUCCESS;
}
