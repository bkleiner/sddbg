#pragma once

#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {

  struct context {
    std::string module;

    ADDR addr;

    LINE_NUM c_line;
    LINE_NUM asm_line;

    std::string function;
    BLOCK block;
    LEVEL level;
  };

  class context_mgr {
  public:
    context_mgr(dbg_session *session);

    void dump();
    void set_context(ADDR addr);
    context get_current() { return cur_context; }

  protected:
    dbg_session *mSession;
    context cur_context;
  };

} // namespace debug::core
