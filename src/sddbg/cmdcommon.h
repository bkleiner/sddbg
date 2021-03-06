#pragma once

#include "parsecmd.h"

namespace debug {

  class CmdVersion : public CmdShowSetInfoHelp {
  public:
    CmdVersion() { name = "version"; }
    bool show(ParseCmd::Args cmd) override;
  };

  class CmdWarranty : public CmdShowSetInfoHelp {
  public:
    CmdWarranty() { name = "warranty"; }
    bool show(ParseCmd::Args cmd) override;
    bool info(ParseCmd::Args cmd) override { return show(cmd); }
  };

  class CmdCopying : public CmdShowSetInfoHelp {
  public:
    CmdCopying() { name = "COPying"; }
    bool show(ParseCmd::Args cmd) override;
    bool info(ParseCmd::Args cmd) override { return show(cmd); }
  };

  /** Provide a gateway to communicate with the target driver.
	for the simulator this allows execution of simulator commands
	for Si51 isallows resetting the emulator etc
*/
  class CmdTarget : public CmdShowSetInfoHelp {
  public:
    CmdTarget() { name = "target"; }
    bool direct(ParseCmd::Args cmd) override;
    bool set(ParseCmd::Args cmd) override;
    bool info(ParseCmd::Args cmd) override;
    bool show(ParseCmd::Args cmd) override;
  };

  class CmdPrompt : public CmdShowSetInfoHelp {
  public:
    CmdPrompt() { name = "PRompt"; }
    bool set(ParseCmd::Args cmd) override;
  };

  class CmdStep : public CmdShowSetInfoHelp {
  public:
    CmdStep() { name = "Step"; }
    bool directnoarg();
  };

  class CmdStepi : public CmdShowSetInfoHelp {
  public:
    CmdStepi() { name = "STEPI"; }
    bool directnoarg();
  };

  class CmdNext : public CmdShowSetInfoHelp {
  public:
    CmdNext() { name = "Next"; }
    bool directnoarg();
  };

  class CmdNexti : public CmdShowSetInfoHelp {
  public:
    CmdNexti() { name = "NEXTI"; }
    bool directnoarg();
  };

  class CmdContinue : public CmdShowSetInfoHelp {
  public:
    CmdContinue() { name = "Continue"; }
    bool direct(ParseCmd::Args cmd) override;
    bool directnoarg();
  };

  class CmdFile : public CmdShowSetInfoHelp {
  public:
    CmdFile() { name = "file"; }
    bool direct(ParseCmd::Args cmd) override;
  };

  class CmdDFile : public CmdShowSetInfoHelp {
  public:
    CmdDFile() { name = "dfile"; }
    bool direct(ParseCmd::Args cmd) override;
  };

  class CmdList : public CmdShowSetInfoHelp {
  public:
    CmdList() { name = "list"; }
    bool direct(ParseCmd::Args cmd) override;
    bool directnoarg();
  };

  class CmdPWD : public CmdShowSetInfoHelp {
  public:
    CmdPWD() { name = "pwd"; }
    bool directnoarg();
  };

  class CmdFiles : public CmdShowSetInfoHelp {
  public:
    CmdFiles() { name = "files"; }
    bool info(ParseCmd::Args cmd);
  };

  class CmdSource : public CmdShowSetInfoHelp {
  public:
    CmdSource() { name = "source"; }
    bool info(ParseCmd::Args cmd);
  };

  class CmdSources : public CmdShowSetInfoHelp {
  public:
    CmdSources() { name = "sources"; }
    bool info(ParseCmd::Args cmd);
  };

  class CmdLine : public CmdShowSetInfoHelp {
  public:
    CmdLine() { name = "Line"; }
    bool info(ParseCmd::Args cmd);
  };

  class CmdRun : public CmdShowSetInfoHelp {
  public:
    CmdRun() { name = "Run"; }
    bool directnoarg();
  };

  class CmdStop : public CmdShowSetInfoHelp {
  public:
    CmdStop() { name = "Stop"; }
    bool directnoarg();
  };

  class CmdFinish : public CmdShowSetInfoHelp {
  public:
    CmdFinish() { name = "Finish"; }
    bool directnoarg();
  };

  class CmdPrint : public CmdShowSetInfoHelp {
  public:
    CmdPrint() { name = "Print"; }
    bool direct(ParseCmd::Args cmd) override;
  };

  class CmdRegisters : public CmdShowSetInfoHelp {
  public:
    CmdRegisters() { name = "Registers"; }
    bool info(ParseCmd::Args cmd) override;
  };
} // namespace debug