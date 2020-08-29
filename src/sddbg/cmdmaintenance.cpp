/***************************************************************************
 *   Copyright (C) 2005 by Ricky White   *
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
#include <iostream>

#include "cmdmaintenance.h"
#include "module.h"
#include "sddbg.h"
#include "symtab.h"
#include "symtypetree.h"

/** This command provides similar functionality to that of GDB
*/
bool CmdMaintenance::direct(ParseCmd::Args cmd) {
  if (cmd.size() == 0)
    return false;

  if (!match(cmd.front(), "dump")) {
    return false;
  }
  cmd.pop_front();

  if (cmd.empty()) {
    return false;
  }

  const std::string s = cmd.front();
  cmd.pop_front();

  if (cmd.empty()) {
    if (match(s, "modules")) {
      gSession.modulemgr()->dump();
      return true;
    }
    if (match(s, "symbols")) {
      gSession.symtab()->dump();
      return true;
    }
    if (match(s, "types")) {
      gSession.symtree()->dump();
      return true;
    }
    return false;
  }

  if (match(s, "module")) {
    std::cout << " dumping module '" << cmd.front() << "'" << std::endl;
    gSession.modulemgr()->module(cmd.front()).dump();
    return true;
  }

  if (match(s, "type")) {
    gSession.symtree()->dump(cmd.front());
    return true;
  }

  return false;
}
