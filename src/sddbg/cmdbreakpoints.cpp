#include "cmdbreakpoints.h"

#include <iostream>
#include <stdlib.h>

#include "breakpoint_mgr.h"
#include "log.h"
#include "sddbg.h"
#include "target.h"

namespace debug {

  bool CmdBreakpoints::show(ParseCmd::Args cmd) {
    return false;
  }

  bool CmdBreakpoints::info(ParseCmd::Args cmd) {
    gSession.bpmgr()->dump();
    return true;
  }

  /** Add a breakpoint

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
  bool CmdBreak::direct(ParseCmd::Args cmd) {
    return gSession.bpmgr()->set_breakpoint(cmd.front()) != core::BP_ID_INVALID;
  }

  bool CmdBreak::directnoarg() {
    core::log::print("Adding a breakpoint at the current location\n");
    return gSession.bpmgr()->set_bp(gSession.target()->read_PC(), false) != core::BP_ID_INVALID;
  }

  bool CmdBreak::help(ParseCmd::Args cmd) {
    if (cmd.empty()) {
      core::log::print("Set breakpoint at specified line or function.\n");
      core::log::print("Argument may be line number, function name, or \"*\" and an address.\n");
      core::log::print("If line number is specified, break at start of code for that line.\n");
      core::log::print("If function is specified, break at start of code for that function.\n");
      core::log::print("If an address is specified, break at that exact address.\n");
      core::log::print("With no arg, uses current execution address of selected stack frame.\n");
      core::log::print("This is useful for breaking on return to a stack frame.\n");
      core::log::print("\n");
      core::log::print("Multiple breakpoints at one place are permitted, and useful if conditional.\n");
      core::log::print("\n");
      core::log::print("Do \"help breakpoints\" for info on other commands dealing with breakpoints.\n");
    }
    return true;
  }

  bool CmdTBreak::direct(ParseCmd::Args cmd) {
    return gSession.bpmgr()->set_breakpoint(cmd.front(), true) != core::BP_ID_INVALID;
  }

  bool CmdTBreak::directnoarg() {
    core::log::print("Adding a temporary breakpoint at the current location\n");
    return gSession.bpmgr()->set_bp(gSession.target()->read_PC(), true) != core::BP_ID_INVALID;
  }

  bool CmdTBreak::help(ParseCmd::Args cmd) {
    return false;
  }

  bool CmdClear::direct(ParseCmd::Args cmd) {
    return gSession.bpmgr()->clear_breakpoint(cmd.front());
  }

  bool CmdClear::directnoarg() {
    return gSession.bpmgr()->clear_breakpoint_addr(gSession.target()->read_PC());
  }

  bool CmdDelete::direct(ParseCmd::Args cmd) {
    if (cmd.size() == 0)
      return false;

    for (auto &c : cmd) {
      if (!gSession.bpmgr()->clear_breakpoint_id(strtoul(c.c_str(), 0, 10)))
        return false;
    }
    return true;
  }

  bool CmdEnable::direct(ParseCmd::Args cmd) {
    gSession.bpmgr()->enable_bp(strtoul(cmd.front().c_str(), 0, 10));
    return true;
  }

  bool CmdDisable::direct(ParseCmd::Args cmd) {
    gSession.bpmgr()->disable_bp(strtoul(cmd.front().c_str(), 0, 10));
    return true;
  }
} // namespace debug