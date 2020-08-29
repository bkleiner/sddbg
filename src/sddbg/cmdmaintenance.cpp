
#include "cmdmaintenance.h"

#include <iostream>

#include "module.h"
#include "sddbg.h"
#include "sym_tab.h"
#include "sym_type_tree.h"

namespace debug {

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

} // namespace debug