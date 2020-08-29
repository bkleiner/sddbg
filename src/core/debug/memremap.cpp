#include "memremap.h"

namespace debug::core {

  constexpr const char target_addr::addr_space_map[];

  target_addr::target_addr()
      : space(AS_UNDEF)
      , addr(INVALID_ADDR) {}

  target_addr::target_addr(target_addr_space space, ADDR addr)
      : space(space)
      , addr(addr) {}

  target_addr target_addr::from_name(char name, ADDR addr) {
    target_addr_space space = AS_UNDEF;

    for (int i = 0; i < sizeof(addr_space_map); i++) {
      if (addr_space_map[i] == name) {
        space = target_addr_space(i);
        break;
      }
    }

    return {space, addr};
  }

  /*
	0x00000000 - 0x00000000	Code memory ( support for processor bank switch and possible sw bank switch)
	0x20000000 - 0x2FFFFFFF	xdata + bank switch
	0x40000000 - 0x400000FF	data ram
	0x40000100 - 0x400001FF	i data ram
	0x80000080 - 0x800000FF	sfr
	0xFFFFFFFF				Invalid address
*/

  target_addr mem_remap::target(uint32_t flat_addr) {
    if (flat_addr < 0x20000000) {
      return {target_addr::AS_CODE, ADDR(flat_addr)};
    }
    if (flat_addr < 0x2FFFFFFF) {
      return {target_addr::AS_EXT_RAM, ADDR(flat_addr & 0x0FFFFFFF)};
    }
    if (flat_addr >= 0x40000000 && flat_addr <= 0x400000FF) {
      return {target_addr::AS_IRAM_LOW, ADDR(flat_addr & 0xFF)};
    }
    if (flat_addr >= 0x40000100 && flat_addr <= 0x400001FF) {
      return {target_addr::AS_INT_RAM, ADDR(flat_addr & 0xFF)};
    }
    if (flat_addr >= 0x80000080 && flat_addr <= 0x80000FFF) {
      return {target_addr::AS_SFR, ADDR(flat_addr & 0xFF)};
    }
    return {target_addr::AS_UNDEF, INVALID_ADDR};
  }

  uint32_t mem_remap::flat(target_addr addr) {
    switch (addr.space) {
    case target_addr::AS_CODE:
      return addr.addr;
    case target_addr::AS_EXT_RAM:
      return addr.addr | 0x20000000;
    case target_addr::AS_IRAM_LOW:
      return addr.addr | 0x40000000;
    case target_addr::AS_INT_RAM:
      return addr.addr | 0x40000100;
    case target_addr::AS_SFR:
      return addr.addr | 0x80000000;
    default:
      return INVALID_FLAT_ADDR;
    }
  }
} // namespace debug::core