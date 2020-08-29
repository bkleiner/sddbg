#pragma once

#include <string>

#include "parsecmd.h"

namespace debug {

  class CmdMaintenance : public CmdShowSetInfoHelp {
  public:
    CmdMaintenance() { name = "Maintenance"; }
    bool direct(ParseCmd::Args cmd) override;
  };

} // namespace debug