#pragma once

#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {
  struct line_spec {
    enum line_spec_type {
      INVALID,
      LINE_NUMBER,
      FUNCTION,
      PLUS_OFFSET,
      MINUS_OFFSET,
      ADDRESS,
    };

    line_spec(line_spec_type spec_type, ADDR addr = INVALID_ADDR);

    static line_spec create(dbg_session *session, std::string linespec);

    line_spec_type spec_type;

    ADDR addr;
    ADDR end_addr;

    std::string file;
    LINE_NUM line;
    std::string function;

    bool valid() const {
      return spec_type != INVALID && addr != INVALID_ADDR;
    }
  };

} // namespace debug::core
