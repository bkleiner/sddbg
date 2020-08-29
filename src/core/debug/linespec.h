#pragma once

#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {
  class line_spec {
  public:
    line_spec(dbg_session *session);

    typedef enum {
      LINENO,
      FUNCTION,
      PLUS_OFFSET,
      MINUS_OFFSET,
      ADDRESS,
      INVALID
    } TYPE;
    bool set(std::string linespec);

    TYPE type() { return spec_type; }
    std::string file() { return filename; }
    LINE_NUM line() { return line_num; }
    std::string func() { return function; }
    ADDR addr() { return address; }
    ADDR end_addr() { return endaddress; }

  protected:
    dbg_session *mSession;
    ADDR address;    ///< -1 = invalid, +ve or 0 is an address
    ADDR endaddress; ///< -1 = invalid, +ve or 0 is an address
    std::string filename;
    std::string function;
    LINE_NUM line_num;
    TYPE spec_type;
  };

} // namespace debug::core
