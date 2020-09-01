#include "registers.h"

#include <fmt/format.h>

#include "target.h"

namespace debug::core {

  const std::vector<cpu_register> cpu_registers::registers = {
      {R0, "R0", 0x0},
      {R1, "R1", 0x1},
      {R2, "R2", 0x2},
      {R3, "R3", 0x3},
      {R4, "R4", 0x4},
      {R5, "R5", 0x5},
      {R6, "R6", 0x6},
      {R7, "R7", 0x7},

      {SP, "SP", 0x81},
      {DPL0, "DPL0", 0x82},
      {DPH0, "DPH0", 0x83},
      {DPL1, "DPL1", 0x84},
      {DPH1, "DPH1", 0x85},

      {DPS, "DPS", 0x92},

      {PSW, "PSW", 0xD0},
      {ACC, "ACC", 0xE0},
  };

  cpu_registers::cpu_registers(dbg_session *session)
      : session(session) {}

  uint8_t cpu_registers::read(cpu_register_names name) {
    target_addr addr = {};
    if (name <= R7) {
      addr.space = target_addr::AS_REGISTER;
      addr.addr = registers[name].addr;
    } else {
      addr.space = target_addr::AS_SFR;
      addr.addr = registers[name].addr;
    }
    uint8_t value = 0;
    session->target()->read_memory(addr, 1, &value);
    return value;
  }

  std::string cpu_registers::print(cpu_register_names name) {
    return fmt::format("{:#x}", read(name));
  }

} // namespace debug::core
