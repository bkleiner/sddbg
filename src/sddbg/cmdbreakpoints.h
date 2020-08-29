#pragma once

#include "parsecmd.h"

namespace debug {

  class CmdBreakpoints : public CmdShowSetInfoHelp {
  public:
    CmdBreakpoints() { name = "BREAKPoints"; }

    bool show(ParseCmd::Args cmd) override;
    bool info(ParseCmd::Args cmd) override;
  };

  class CmdBreak : public CmdShowSetInfoHelp {
  public:
    CmdBreak() { name = "BReak"; }
    bool direct(ParseCmd::Args cmd) override;
    bool directnoarg() override;
    bool help(ParseCmd::Args cmd) override;
  };

  class CmdTBreak : public CmdShowSetInfoHelp {
  public:
    CmdTBreak() { name = "TBreak"; }
    bool direct(ParseCmd::Args cmd) override;
    bool directnoarg() override;
    bool help(ParseCmd::Args cmd) override;
  };

  class CmdClear : public CmdShowSetInfoHelp {
  public:
    CmdClear() { name = "CLear"; }
    bool direct(ParseCmd::Args cmd) override;
    bool directnoarg() override;
  };

  class CmdDelete : public CmdShowSetInfoHelp {
  public:
    CmdDelete() { name = "DElete"; }
    bool direct(ParseCmd::Args cmd) override;
  };

  class CmdDisable : public CmdShowSetInfoHelp {
  public:
    CmdDisable() { name = "DIsable"; }
    bool direct(ParseCmd::Args cmd) override;
  };

  class CmdEnable : public CmdShowSetInfoHelp {
  public:
    CmdEnable() { name = "ENable"; }
    bool direct(ParseCmd::Args cmd) override;
  };

} // namespace debug
