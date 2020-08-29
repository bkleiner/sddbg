#pragma once

#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {
  class context_mgr {
  public:
    enum MODE {
      ASM,
      C,
    };

    struct Context {
      std::string module;
      ADDR addr; // address of current c line
      ADDR asm_addr;
      LINE_NUM line; ///< @depreciated
      LINE_NUM c_line;
      LINE_NUM asm_line;
      MODE mode;
      BLOCK block;
      LEVEL level;
      std::string function;
    };

    context_mgr(dbg_session *session);
    ~context_mgr();
    void dump();
    void set_context(ADDR addr);
    Context get_current() { return cur_context; }

  protected:
    dbg_session *mSession;
    Context cur_context;
  };

} // namespace debug::core
