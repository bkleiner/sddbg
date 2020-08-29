#include "cmdlist.h"

#include <fmt/format.h>

#include "cmdbreakpoints.h"
#include "cmdcommon.h"
#include "cmddisassemble.h"
#include "cmdmaintenance.h"

ParseCmdList::ParseCmdList() {

  // add commands to list
  add(new CmdHelp(this));

  add(new CmdVersion());
  add(new CmdWarranty());
  add(new CmdCopying());

  add(new CmdPrompt());
  add(new CmdBreakpoints());
  add(new CmdBreak());
  add(new CmdTBreak());
  add(new CmdDelete());
  add(new CmdEnable());
  add(new CmdDisable());
  add(new CmdClear());
  add(new CmdTarget());
  add(new CmdStep());
  add(new CmdStepi());
  add(new CmdNext());
  add(new CmdNexti());
  add(new CmdContinue());
  add(new CmdDFile());
  add(new CmdFile());
  add(new CmdFiles());
  add(new CmdList());
  add(new CmdPWD());
  add(new CmdSource());
  add(new CmdSources());
  add(new CmdLine());
  add(new CmdRun());
  add(new CmdStop());
  add(new CmdFinish());
  add(new CmdDisassemble());
  add(new CmdX());
  add(new CmdChange());
  add(new CmdMaintenance());
  add(new CmdPrint());
  add(new CmdRegisters());
}

bool ParseCmdList::parse(std::string &cmd) {
  for (ParseCmd *c : cmds) {
    if (c->parse(cmd)) {
      return true;
    }
  }
  // anything left with length >0 is bad.
  return cmd.length() == 0;
}

/** top level help
*/
bool CmdHelp::directnoarg() {
  fmt::print("Help\n\n");
  fmt::print("List of commands:\n");

  for (auto c : list->get_cmds()) {
    fmt::print("  {}\n", c->get_name());
  }

  fmt::print("\n");
  return true;
}