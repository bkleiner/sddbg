#pragma once

#include "parsecmd.h"

namespace debug {

  class CmdShow : public ParseCmd {
  public:
    CmdShow();
    ~CmdShow();
    virtual bool parse(std::string cmd);

  protected:
    List cmdlist;
  };

  class CmdShowVersion : public ParseCmd {
  public:
    virtual bool parse(std::string cmd);
  };

  class CmdShowCopying : public ParseCmd {
  public:
    virtual bool parse(std::string cmd);
  };

  class CmdShowWarranty : public ParseCmd {
  public:
    virtual bool parse(std::string cmd);
  };

} // namespace debug
