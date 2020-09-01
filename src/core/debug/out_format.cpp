#include "out_format.h"

#include <assert.h>
#include <cstring>
#include <iostream>
#include <sstream>

#include "mem_remap.h"
#include "sym_tab.h"
#include "target.h"

namespace debug::core {
  out_format::out_format(dbg_session *session)
      : mTargetEndian(ENDIAN_LITTLE)
      , session(session) {
  }

  std::string out_format::print(char fmt, target_addr addr, uint32_t size) {
    std::ostringstream out;
    using std::endl;
    using std::hex;
    using std::oct;
    uint32_t i, j;
    bool active, flag, one;

    switch (fmt) {
    case 'x':
      out << std::showbase << hex << get_uint(addr, size);
      break;
    case 'd':
      out << (int32_t)get_int(addr, size);
      break;
    case 'u':
      out << std::dec << get_uint(addr, size);
      break;
    case 'o':
      out << std::showbase << oct << get_uint(addr, size);
      break;
    case 't':
      // integer in binary. The letter `t' stands for "two"
      // strips leading zeros
      j = get_uint(addr, size);
      active = false;
      for (i = 0; i < size; i++) {
        one = j & (1 << (size - i - 1));
        active |= one;
        out << (one && active) ? '1' : '0';
      }
      break;
    case 'a':
      // Address
      // prints the address and the nearest preceding symbol
      // (gdb) p/a 0x54320
      // $3 = 0x54320 <_initialize_vx+396>
      out << hex << mem_remap::flat(addr);
      break; // Print as an address, both absolute in hexadecimal and as an offset from the nearest preceding symbol. You can use this format used to discover where (in what function) an unknown address is located:
    case 'c':
      // Regard as an integer and print it as a character constant.
      // This prints both the numerical value and its character
      // representation. The character representation is replaced with
      // the octal escape `\nnn' for characters outside the 7-bit ASCII
      // range.
      j = get_uint(addr, size);
      out << j << " '";
      if (j < 0x20 || j > 0x7e) {
        // non printable, use \nnn format
        out << '\\' << std::showbase << oct << j;
      } else
        out << char(j);
      out << "'";
      break; // Regard as an integer and print it as a character constant.
    case 'f':
      // print as floating point
      if (size == 4) {
        i = get_uint(addr, 4);
        out << *((float *)&i);
      } else
        out << "float not supported for this data type!";
      break;
    case 0:
      // Default format specifier for type
      out << "?";
      break;
    case 's': // sddbg specific format, std::string
      j = get_uint(addr, size);
      if ((j < 0x20 || j > 0x7e) && j != 0)
        out << '\\' << std::showbase << oct << j; // use \nnn format
      else
        out << char(j);
      break;
    default:
      out << "ERROR Unknown format specifier.";
    }
    return out.str();
  }

  uint32_t out_format::get_uint(target_addr addr, char size) {
    if (size == -1 || !addr.valid()) {
      return 0;
    }

    uint8_t buf[size];
    memset(buf, 0, size);
    session->target()->read_memory(addr, size, buf);

    uint32_t result = 0;
    if (mTargetEndian == ENDIAN_LITTLE) {
      for (int i = 0; i < size; i++)
        result = (result << 8) | buf[size - i - 1];
    } else if (mTargetEndian == ENDIAN_BIG) {
      for (int i = 0; i < size; i++)
        result = (result << 8) | buf[i];
    } else
      assert(1 == 0); // unsupported endian type
    return result;
  }

  int32_t out_format::get_int(target_addr addr, char size) {
    uint32_t v = get_uint(addr, size); // raw bit pattern
    // Sign extend
    int32_t mask = 1 << (size * 8 - 1);
    return -(v & mask) | v;
  }

} // namespace debug::core