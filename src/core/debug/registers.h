#pragma once

#include <vector>

#include "types.h"

#include "dbg_session.h"

namespace debug::core {

  enum cpu_register_names {
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,

    SP,
    DPL0,
    DPH0,
    DPL1,
    DPH1,

    DPS,

    PSW,
    ACC,
  };

  struct cpu_register {
    cpu_register_names name;
    std::string str;
    ADDR addr;
  };

  class cpu_registers {
  public:
    cpu_registers(dbg_session *session);

    std::string print(cpu_register_names name);

    const std::vector<cpu_register> &get_registers() {
      return registers;
    }

  private:
    static const std::vector<cpu_register> registers;
    dbg_session *session;
  };

} // namespace debug::core