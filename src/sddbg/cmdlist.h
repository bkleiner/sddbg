#pragma once

#include <list>
#include <string>

#include "parsecmd.h"

namespace debug {

  class ParseCmdList {
  public:
    ParseCmdList();

    void add(ParseCmd *cmd) {
      cmds.push_back(cmd);
    }
    std::list<ParseCmd *> &get_cmds() {
      return cmds;
    }

    bool parse(std::string &cmd);

  private:
    std::list<ParseCmd *> cmds;
  };

  class CmdHelp : public CmdShowSetInfoHelp {
  public:
    CmdHelp(ParseCmdList *list)
        : list(list) {
      name = "help";
    }

    bool directnoarg();

  private:
    ParseCmdList *list;
  };

} // namespace debug