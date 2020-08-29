#pragma once

#include <ctype.h>
#include <stdint.h>

#include "types.h"

namespace debug::core {

  struct target_addr {
    enum target_addr_space {
      AS_XSTACK,      ///< External stack
      AS_ISTACK,      ///< Internal stack
      AS_CODE,        ///< Code memory
      AS_CODE_STATIC, ///< Code memory, static segment
      AS_IRAM_LOW,    ///< Internal RAM (lower 128 bytes)
      AS_EXT_RAM,     ///< External data RAM
      AS_INT_RAM,     ///< Internal data RAM
      AS_BIT,         ///< Bit addressable area
      AS_SFR,         ///< SFR space
      AS_SBIT,        ///< SBIT space
      AS_REGISTER,    ///< Register space
      AS_UNDEF        ///< Used for function records, or any undefined space code
    };
    static constexpr const char addr_space_map[] = {
        'A', // AS_XSTACK
        'B', // AS_ISTACK
        'C', // AS_CODE
        'D', // AS_CODE_STATIC
        'E', // AS_IRAM_LOW
        'F', // AS_EXT_RAM
        'G', // AS_INT_RAM
        'H', // AS_BIT
        'I', // AS_SFR
        'J', // AS_SBIT
        'R', // AS_REGISTER
        'Z', // AS_UNDEF
    };

    target_addr();
    target_addr(target_addr_space space, ADDR addr);

    static target_addr from_name(char name, ADDR addr);

    char space_name() {
      return addr_space_map[space];
    }

    operator ADDR() const {
      return addr;
    }

    target_addr operator+(ADDR rhs) const {
      return {space, addr + rhs};
    }

    target_addr_space space;
    ADDR addr;
  };

  class mem_remap {
  public:
    /*
      Memory remapping

      Special addresses to allow easy debugging with interfaces designed for gdb

      0x00000000 - 0x00000000	Code memory ( support for processor bank switch and possible sw bank switch)
      0x20000000 - 0x2FFFFFFF	xdata + bank switch
      0x40000000 - 0x400000FF	data ram
      0x40000100 - 0x400001FF	i data ram
      0x80000080 - 0x800000FF	sfr
      0xFFFFFFFF				Invalid address
      
      @TODO add other areas for 8051 regs etc...
      
      @TODO consider how this can be applied to other processors.
    */
    static target_addr target(FLAT_ADDR flat_addr);
    static FLAT_ADDR flat(target_addr addr);
  };

} // namespace debug::core