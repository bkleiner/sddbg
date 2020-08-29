#include "breakpointmgr.h"

#include <iostream>
#include <stdio.h>
#include <vector>

#include "dbgsession.h"
#include "linespec.h"
#include "symtab.h"
#include "target.h"

namespace debug::core {

  breakpoint_mgr::breakpoint_mgr(dbg_session *session)
      : session(session) {
  }

  bp_id breakpoint_mgr::set_bp(ADDR addr, bool temporary) {
    if (!add_target_bp(addr)) {
      return BP_ID_INVALID;
    }

    breakpoint ent = {
        next_id(),
        addr,
        temporary,
        false,
    };

    bplist.push_back(ent);
    return ent.id;
  }

  void breakpoint_mgr::clear_all() {
    std::cout << "Clearing all breakpoints." << std::endl;
    bplist.clear();
    std::cout << "Clearing all breakpoints in target." << std::endl;
    session->target()->clear_all_breakpoints();
  }

  void breakpoint_mgr::reload_all() {
    std::cout << "Reloading breakpoints into the target." << std::endl;

    session->target()->clear_all_breakpoints();

    std::vector<ADDR> loaded;
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      std::vector<ADDR>::iterator lit;
      for (lit = loaded.begin(); lit != loaded.end(); ++lit) {
        if (it->addr == *(lit))
          break;
      }
      if (lit == loaded.end()) {
        session->target()->add_breakpoint(it->addr);
        loaded.push_back(it->addr);
      }
    }
  }

  void breakpoint_mgr::dump() {
    if (bplist.empty()) {
      std::cout << "No breakpoints or watchpoints." << std::endl;
      return;
    }

    printf("Num Type           Disp Enb Address            What\n");
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      printf("%-4ibreakpoint     %-5s%-4c0x%04x             %s\n",
             it->id,
             it->temporary ? "del " : "keep",
             it->disabled ? 'n' : 'y',
             it->addr,
             it->what.c_str());
    }
  }

  int breakpoint_mgr::next_id() {
    int lowest;
    bool ok;

    for (lowest = 1; lowest < 0xffff; lowest++) {
      ok = true;
      for (auto &bp : bplist) {
        if (bp.id == lowest) {
          ok = false;
          break;
        }
      }
      if (ok)
        return lowest;
    }
    return 1;
  }

  /** Sets a breakpoint based on a what field
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
  bp_id breakpoint_mgr::set_breakpoint(std::string cmd, bool temporary) {
    line_spec ls(session);
    if (cmd.length() == 0) {
      const auto ctx = session->contextmgr()->get_current();
      return set_bp(ctx.addr); //  add breakpoint at current location
    }

    if (!ls.set(cmd) || ls.addr() == -1) {
      return BP_ID_INVALID;
    }

    breakpoint ent = {
        next_id(),
        ls.addr(),
        temporary,
        false,
        cmd,
    };
    if (!add_target_bp(ent.addr)) {
      return BP_ID_INVALID;
    }

    printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
           ent.id,
           ent.addr,
           ls.file().c_str(),
           ls.line());

    bplist.push_back(ent);
    return ent.id;
  }

  void breakpoint_mgr::stopped(ADDR addr) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->addr != addr) {
        continue;
      }

      // inform user we stopped on a breakpoint
      std::cout << "Stopped on breakpoint #" << it->id << std::endl;
      if (it->temporary) {
        // remove temporary breakpoints
        bplist.erase(it);
        it = bplist.begin(); // safer, we start again after deleting an object, slow but reliable
        del_target_bp(addr);
      }
    }
  }

  bool breakpoint_mgr::clear_breakpoint_id(bp_id id) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->id == id) {
        ADDR addr = it->addr;
        bplist.erase(it);
        del_target_bp(addr);
        return true;
      }
    }
    return false;
  }

  bool breakpoint_mgr::clear_breakpoint_addr(ADDR addr) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->addr == addr) {
        bplist.erase(it);
        del_target_bp(addr);
        return true;
      }
    }
    return false;
  }

  bool breakpoint_mgr::clear_breakpoint(std::string cmd) {
    line_spec ls(session);
    if (cmd.length() == 0) {
      const auto ctx = session->contextmgr()->get_current();
      return set_bp(ctx.addr); //  add breakpoint at current location
    }

    if (!ls.set(cmd) || ls.addr() == -1) {
      return BP_ID_INVALID;
    }

    return clear_breakpoint_addr(ls.addr());
  }

  bool breakpoint_mgr::enable_bp(bp_id id) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->id == id) {
        if (add_target_bp(it->addr)) {
          it->disabled = false;
          return true;
        } else {
          return false;
        }
      }
    }
    return false;
  }

  bool breakpoint_mgr::disable_bp(bp_id id) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->id == id) {
        it->disabled = true;
        return del_target_bp(it->addr);
      }
    }
    return false;
  }

  bool breakpoint_mgr::active_bp_at(ADDR addr) {
    for (auto it = bplist.begin(); it != bplist.end(); ++it) {
      if (it->addr == addr && !it->disabled)
        return true;
    }
    return false; // no breakpoint at the specified address.
  }

  bool breakpoint_mgr::add_target_bp(ADDR addr) {
    if (!active_bp_at(addr)) {
      return session->target()->add_breakpoint(addr);
    }
    printf("BP already active at this address\n");
    return true; // already a bp at this address in target.
  }

  bool breakpoint_mgr::del_target_bp(ADDR addr) {
    if (!active_bp_at(addr)) {
      return session->target()->del_breakpoint(addr);
    }
    return true; // already a bp at this address in target.
  }

} // namespace debug::core