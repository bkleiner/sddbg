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
      : mSession(session) {
  }

  breakpoint_mgr::~breakpoint_mgr() {
  }

  /** set a normal breakpoint at the specified address
	\param addr	address to set breakpoint on
	\returns assigned bp id
*/
  BP_ID breakpoint_mgr::set_bp(ADDR addr, bool temporary) {
    if (add_target_bp(addr)) {
      BP_ENTRY ent;
      ent.id = next_id();
      ent.addr = addr;
      ent.bTemp = temporary;
      ent.bDisabled = false;
      bplist.push_back(ent);
      return ent.id;
    }
    return BP_ID_INVALID;
  }

  BP_ID breakpoint_mgr::set_bp(std::string file, LINE_NUM line) {
    // @TODO lookup address, try and findout what it is, if we can't find it we should fail, since we know which files are involved from the start (nothing is dynamic)
    if (add_target_bp(0x1234)) {
      BP_ENTRY ent;
      ent.id = next_id();
      ent.addr = 0x1234;
      ent.bTemp = false;
      ent.bDisabled = false;
      ent.file = file;
      ent.line = line;
      bplist.push_back(ent);
      return ent.id;
    }
    return BP_ID_INVALID;
  }

  // @FIXME: shoulden't we remove file and line from the entry and have a generic what field that can be set approprieatly when a breakpoint is created of modified
  /** Set a temporary breakpoint.
	Temporary breakpoints will be deleted after the target stops on them
	\param addr	address to set breakpoint on
	\returns assigned BP id, BP_ID_INVALID on failure.
*/
  BP_ID breakpoint_mgr::set_temp_bp(ADDR addr) {
    if (add_target_bp(addr)) {
      BP_ENTRY ent;
      ent.id = next_id();
      ent.addr = addr;
      ent.bTemp = true;
      ent.bDisabled = false;
      bplist.push_back(ent);
      return ent.id;
    }
    return BP_ID_INVALID;
  }

  /** Set a temporary breakpoint.
	Temporary breakpoints will be deleted after the target stops on them
	\param addr	address to set breakpoint on
	\returns assigned BP id, BP_ID_INVALID on failure.
*/
  BP_ID breakpoint_mgr::set_temp_bp(std::string file, unsigned int line) {
    // @TODO lookup address, try and findout what it is, if we can't find it we should fail, since we know which files are involved from the start (nothing is dynamic)
    if (add_target_bp(0x1234)) {
      BP_ENTRY ent;
      ent.id = next_id();
      ent.addr = 0x1234;
      ent.bTemp = true;
      ent.bDisabled = false;
      ent.file = file;
      ent.line = line;
      bplist.push_back(ent);
      return ent.id;
    }
    return BP_ID_INVALID;
  }

  /** Clear all breakpoints.
*/
  void breakpoint_mgr::clear_all() {
    std::cout << "Clearing all breakpoints." << std::endl;
    bplist.clear();
    std::cout << "Clearing all breakpoints in target." << std::endl;
    mSession->target()->clear_all_breakpoints();
  }

  void breakpoint_mgr::reload_all() {
    BP_LIST::iterator it;
    std::cout << "Reloading breakpoints into the target." << std::endl;

    std::vector<ADDR> loaded;
    std::vector<ADDR>::iterator lit;

    mSession->target()->clear_all_breakpoints();
    if (bplist.size() > 0) {
      for (it = bplist.begin(); it != bplist.end(); ++it) {
        // only reload if we haven't already loaded a bp at that address.
        //lit = loaded.find( (*it).addr );
        for (lit = loaded.begin(); lit != loaded.end(); ++lit) {
          if ((*it).addr == *(lit))
            break;
        }
        //lit = find(loaded.begin(),loaded.end),(*it).addr==addr);
        if (lit == loaded.end()) {
          mSession->target()->add_breakpoint((*it).addr);
          loaded.push_back((*it).addr);
        }
      }
    }
  }

  void breakpoint_mgr::dump() {
    BP_LIST::iterator it;

    if (bplist.size() > 0) {
      printf("Num Type           Disp Enb Address            What\n");
      for (it = bplist.begin(); it != bplist.end(); ++it) {
        printf("%-4ibreakpoint     %-5s%-4c0x%04x             %s\n",
               (*it).id,
               (*it).bTemp ? "del " : "keep",
               (*it).bDisabled ? 'n' : 'y',
               (*it).addr,
               (*it).what.c_str());
      }
    } else
      std::cout << "No breakpoints or watchpoints." << std::endl;
  }

  /** find the lowest unused breakpoint number and return it
	This version is ineffificent but it will work 
	for the numbe of breakpoints we expect.
*/
  int breakpoint_mgr::next_id() {
    int lowest;
    bool ok;

    BP_LIST::iterator it;
    for (lowest = 1; lowest < 0xffff; lowest++) {
      ok = true;
      for (it = bplist.begin(); it != bplist.end(); ++it) {
        if ((*it).id == lowest) {
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
  BP_ID breakpoint_mgr::set_breakpoint(std::string cmd, bool temporary) {
    BP_ENTRY ent;
    line_spec ls(mSession);
    if (cmd.length() == 0) {
      return set_bp(cur_addr); //  add breakpoint at current location
    } else if (ls.set(cmd)) {
      // valid linespec
      switch (ls.type()) {
      case line_spec::LINENO:
        //				ADDR addr = symtab.get_addr( ls.file(), ls.line() );
        if (ls.addr() != -1) {
          ent.addr = ls.addr();
          ent.what = cmd;
          ent.bTemp = temporary;
          ent.id = next_id();
          ent.bDisabled = false;
          if (add_target_bp(ent.addr)) {
            printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
                   ent.id,
                   ent.addr,
                   ls.file().c_str(),
                   ls.line());

            bplist.push_back(ent);
            return ent.id;
          }
        }
        return BP_ID_INVALID;
        //return true;	// don't print bad command, was correctly formatted just no addr
        break;
      case line_spec::FUNCTION:
        ent.addr = ls.addr();
        ent.what = cmd;
        ent.bTemp = temporary;
        ent.id = next_id();
        ent.bDisabled = false;
        if (add_target_bp(ent.addr)) {
          printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
                 ent.id,
                 ent.addr,
                 ls.file().c_str(),
                 ls.line());
          bplist.push_back(ent);
          return ent.id;
        }
        break;
      case line_spec::PLUS_OFFSET:
        ent.addr = ls.addr();
        ent.what = cmd;
        ent.bTemp = temporary;
        ent.id = next_id();
        ent.bDisabled = false;
        if (add_target_bp(ent.addr)) {
          printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
                 ent.id,
                 ent.addr,
                 ls.file().c_str(),
                 ls.line());
          bplist.push_back(ent);
          return ent.id;
        }
        break;
      case line_spec::MINUS_OFFSET:
        ent.addr = ls.addr();
        ent.what = cmd;
        ent.bTemp = temporary;
        ent.id = next_id();
        ent.bDisabled = false;
        if (add_target_bp(ent.addr)) {
          printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
                 ent.id,
                 ent.addr,
                 ls.file().c_str(),
                 ls.line());
          bplist.push_back(ent);
          return ent.id;
        }
        break;
      case line_spec::ADDRESS:
        ent.addr = ls.addr();
        ent.what = cmd;
        ent.bTemp = temporary;
        ent.id = next_id();
        ent.bDisabled = false;
        if (add_target_bp(ent.addr)) {
          printf("Breakpoint %i at 0x%04x: file %s, line %i.\n",
                 ent.id,
                 ent.addr,
                 ls.file().c_str(),
                 ls.line());
          bplist.push_back(ent);
          return ent.id;
        }
        break;
      default:
        return BP_ID_INVALID; // invalid linespec
      }
    }
    return BP_ID_INVALID;
  }

  /** call when execution stops to check of a temporary breakpoint caused it
	if so this will delete the breakpoint, freeing its resources up
	\param addr Address where the target stopped
 */
  void breakpoint_mgr::stopped(ADDR addr) {
    // scan for matching address in breakpoint list.
    BP_LIST::iterator it;

    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).addr == addr) {
        // inform user we stopped on a breakpoint
        std::cout << "Stopped on breakpoint #" << (*it).id << std::endl;
        if ((*it).bTemp) {
          // remove temporary breakpoints
          bplist.erase(it);
          it = bplist.begin(); // safer, we start again after deleting an object, slow but reliable
          del_target_bp(addr);
        }
      }
    }
  }

  bool breakpoint_mgr::clear_breakpoint_id(BP_ID id) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).id == id) {
        ADDR addr = (*it).addr;
        bplist.erase(it);
        del_target_bp(addr);
        return true;
      }
    }
    return false;
  }

  bool breakpoint_mgr::clear_breakpoint_addr(ADDR addr) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).addr == addr) {
        bplist.erase(it);
        del_target_bp(addr);
        return true;
      }
    }
    return false;
  }

  bool breakpoint_mgr::clear_breakpoint(std::string cmd) {
    line_spec ls(mSession);
    if (cmd.length() == 0) {
      return set_bp(cur_addr); //  add breakpoint at current location
    } else if (ls.set(cmd)) {
      // valid linespec
      switch (ls.type()) {
      case line_spec::LINENO:
      case line_spec::FUNCTION:
      case line_spec::PLUS_OFFSET:
      case line_spec::MINUS_OFFSET:
      case line_spec::ADDRESS:
        return clear_breakpoint_addr(ls.addr());
      default:
        return false;
      }
    }
    return false;
  }

  std::string breakpoint_mgr::current_file() {
    return "NOT IMPLEMENTED: breakpoint_mgr::current_file()";
  }

  bool breakpoint_mgr::enable_bp(BP_ID id) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).id == id) {
        if (add_target_bp((*it).addr)) {
          (*it).bDisabled = false;
          return true;
        } else {
          return false;
        }
      }
    }
    return false;
  }

  bool breakpoint_mgr::disable_bp(BP_ID id) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).id == id) {
        (*it).bDisabled = true;
        return del_target_bp((*it).addr);
      }
    }
    return false;
  }

  bool breakpoint_mgr::active_bp_at(ADDR addr) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).addr == addr &&
          !(*it).bDisabled)
        return true;
    }
    return false; // no breakpoint at the specified address.
  }

  // for this to work the bplist entry must be added after this call.
  bool breakpoint_mgr::add_target_bp(ADDR addr) {
    if (!active_bp_at(addr)) {
      return mSession->target()->add_breakpoint(addr);
    }
    printf("BP already active at this address\n");
    return true; // already a bp at this address in target.
  }

  // for this to work the bplist entry must be removed before this
  // function is called.
  bool breakpoint_mgr::del_target_bp(ADDR addr) {
    if (!active_bp_at(addr)) {
      return mSession->target()->del_breakpoint(addr);
    }
    return true; // already a bp at this address in target.
  }

  bool breakpoint_mgr::get_bp_file_line(BP_ID id, std::string &file, int &line) {
    BP_LIST::iterator it;
    for (it = bplist.begin(); it != bplist.end(); ++it) {
      if ((*it).id == id) {
        file = (*it).file;
        line = (*it).line;
        return true;
      }
    }
    return false;
  }
} // namespace debug::core