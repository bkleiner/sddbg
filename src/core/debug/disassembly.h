#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "types.h"

namespace debug::core {

  struct instruction {
    uint64_t code;
    char branch;
    uint8_t length;
    std::string mnemonic;
    bool is_call;
  };

  struct disassembly_line {
    disassembly_line()
        : start_addr(INVALID_ADDR)
        , end_addr(INVALID_ADDR) {}

    disassembly_line(
        ADDR start_addr,
        ADDR end_addr,
        instruction instr)
        : start_addr(start_addr)
        , end_addr(end_addr)
        , instr(instr) {}

    ADDR start_addr;
    ADDR end_addr;
    instruction instr;
  };

  class disassembly {
  public:
    void load_file(std::string filename);

    std::string get_source();
    LINE_NUM get_line_number(ADDR addr);

  private:
    std::vector<disassembly_line> lines;

    void dissasemble(uint8_t *buf, size_t size);
  };

} // namespace debug::core