#pragma once

#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {
  enum context_mode {
    ASM,
    C,
  };

  struct context {
    std::string module;
    ADDR addr; // address of current c line
    ADDR asm_addr;
    LINE_NUM line; ///< @depreciated
    LINE_NUM c_line;
    LINE_NUM asm_line;
    context_mode mode;
    BLOCK block;
    LEVEL level;
    std::string function;
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
