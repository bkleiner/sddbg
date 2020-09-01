#pragma once

#include <string>

#include "dbg_session.h"
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

    bool in_interrupt_handler;
  };

  class context_mgr {
  public:
    context_mgr(dbg_session *session);

    void dump();
    context update_context();
    context set_context(ADDR addr);
    context get_current() {
      if (stack.empty()) {
        return {};
      }
      return stack[0];
    }
    std::vector<context> get_stack() {
      return stack;
    }

  protected:
    dbg_session *session;
    std::vector<context> stack;

    context build_context(ADDR addr);
  };

} // namespace debug::core
