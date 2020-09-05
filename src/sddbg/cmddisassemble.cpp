#include "cmddisassemble.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "line_spec.h"
#include "log.h"
#include "mem_remap.h"
#include "module.h"
#include "sddbg.h"
#include "sym_tab.h"
#include "target.h"
#include "types.h"

namespace debug {

  static bool print_asm_line(core::ADDR start, core::ADDR end, std::string function);

  /** Disassemble commend
    	disassemble [startaddr [endaddress]]
    */
  bool CmdDisassemble::direct(ParseCmd::Args cmd) {
    if (cmd.size() == 1) {
      // start only
      core::ADDR start = strtoul(cmd.front().c_str(), 0, 0);
      /// @FIXME: need a way to get a symbols address, given the symbol and module and vice versa, give an address and get a symbol
      std::string file, func;
      gSession.symtab()->get_c_function(start, file, func);
      core::log::print("Dump of assembler code for function {}:\n", func);
      print_asm_line(start, -1, func);
      core::log::print("End of assembler dump.\n");
      return true;
    }

    if (cmd.size() == 2) {
      // start and end
      core::ADDR start = strtoul(cmd[0].c_str(), 0, 0);
      core::ADDR end = strtoul(cmd[1].c_str(), 0, 0);

      //		printf("start=0x%04x, end=0x%04x\n",start,end);
      std::string file, func;
      gSession.symtab()->get_c_function(start, file, func);
      core::log::print("Dump of assembler code for function {}:\n", func);
      print_asm_line(start, end, func);
      core::log::print("End of assembler dump.\n");
      return true;
    }

    return false;
  }

  static bool print_asm_line(core::ADDR start, core::ADDR end, std::string function) {
    uint32_t asm_lines;
    core::ADDR delta;
    core::ADDR sym_addr;
    core::ADDR last_addr;
    std::string sym_name;
    bool printedLine = false;

    std::string module;
    core::LINE_NUM line;
    gSession.modulemgr()->get_asm_addr(start, module, line);
    core::module &m = gSession.modulemgr()->module(module);

    asm_lines = m.get_asm_num_lines();
    last_addr = start + 1;
    sym_addr = start;
    sym_name.clear();

    sym_addr = start;
    sym_name = function;
    int32_t i, j;
    for (j = 0, i = 1; i <= asm_lines; i++) {
      if (start >= 0 && m.get_asm_addr(i) < start) {
        continue;
      }
      if (end >= 0 && m.get_asm_addr(i) > end) {
        continue;
      }
      if (!function.empty()) {
        core::ADDR sfunc, efunc;
        gSession.symtab()->get_addr(function, sfunc, efunc);
        if (m.get_asm_addr(i) < sfunc ||
            m.get_asm_addr(i) > efunc)
          continue;
      }
      delta = m.get_asm_addr(i) - sym_addr;
      if (delta >= 0) {
        j++;
        last_addr = m.get_asm_addr(i);
        core::log::printf("0x%08x <%s", last_addr, sym_name.c_str());
        core::log::printf("+%5d", delta);
        core::log::printf(">:\t%s\n", m.get_asm_src(i).c_str());
        printedLine = true;
      }
    }
    return printedLine;
  }

  /** examine command imlementation.
	
	Examine memory: x/FMT ADDRESS.
	ADDRESS is an expression for the memory address to examine.
	FMT is a repeat count followed by a format letter and a size letter.
	Format letters are o(octal), x(hex), d(decimal), u(unsigned decimal),
	t(binary), f(float), a(address), i(instruction), c(char) and s(std::string).
	Size letters are b(byte), h(halfword), w(word), g(giant, 8 bytes).
	The specified number of objects of the specified size are printed
	according to the format.
	
	Defaults for format and size letters are those previously used.
	Default count is 1.  Default address is following last thing printed
	with this command or "print".

	example format std::strings





	@TOD add support for $sp $pc $fp and $ps
*/
  bool CmdX::direct(ParseCmd::Args cmd) {
    if (cmd.size() < 1)
      return false;

    uint32_t flat_addr;
    if (cmd[0][0] != '/') {
      // no format or size information, use defaults.
      flat_addr = strtoul(cmd[0].c_str(), 0, 0);
    } else if (parseFormat(cmd[0]) && (cmd.size() > 1)) {
      flat_addr = strtoul(cmd[1].c_str(), 0, 0);
    } else
      return false;

    for (int num = 0; num < num_units; num++) {
      char area;
      core::target_addr addr = core::mem_remap::target(flat_addr);

      switch (format) {
      case 'i': // instruction
        if (area != 'c') {
          core::log::printf("ERROR: can't print out in instruction format for non code memory areas\n");
        } else {
          if (print_asm_line(addr.addr, addr.addr + (num_units - num) * unit_size, std::string())) {
            // return, since print_asm_line prints all relevant lines
            return true;
          }
        }
        break;
      case 'x': // hex
        // read memory in one big chunk to reduce communication
        //  overhead on large reads
        unsigned int readByteLength;

        readByteLength = unit_size * num_units;
        unsigned char readValues[1000];

        (void)readMem(flat_addr, readByteLength, readValues);
        for (int i = 0; i < num_units; i++) {
          core::log::printf("0x");
          for (int j = 0; j < unit_size; j++) {
            //core::log::printf ("%d %d %d %d",i,j,num_units,unit_size);
            core::log::printf("%02x", readValues[i * unit_size + j]);
          }
          core::log::printf("\n");
        }
        return true;
      case 's': // std::string
        core::log::printf("std::string here\n");
        break;
      }
      flat_addr += unit_size;
    }
    return true;
  }

  /** parse the /nfu token
	must begin with '/' or it isn't a format specifier and we return false.

	n = number of unit outputs to show
	f = 's' / 'i' / 'x'	or not present for default
	u = 'b' / 'h' / 'w' / 'g'	number of bytes in word, optional
*/
  bool CmdX::parseFormat(std::string token) {
    int repeatNumber = 0;
    int numberOfDigits = 0;
    bool parsedLetter = false;

    token = token.substr(1);
    //std::cout << "tt" << token << std::endl;
    for (int i = 0; i < token.size(); i++) {
      // for each char
      if (isdigit(token[i]) && (parsedLetter == false)) {
        if (numberOfDigits < 4) {
          repeatNumber = repeatNumber * 10 + (token[i] - '0');
          numberOfDigits++;
        }
        // error if number is too big
        else {
          return false;
        }
      } else {
        parsedLetter = true;
        switch (token[i]) {
        case 's':
        case 'i':
        case 'x':
          format = token[i];
          break;
        case 'b':
          unit_size = 1;
          break;
        case 'h':
          unit_size = 2;
          break;
        case 'w':
          unit_size = 4;
          break;
        case 'g':
          unit_size = 8;
          break;
        default:
          return false; // invalid format
        }
      }
    }
    if (repeatNumber) {
      num_units = repeatNumber;
    }
    return true;
  }

  bool CmdX::readMem(uint32_t flat_addr, unsigned int readByteLength, unsigned char *returnPointer) {
    unsigned char b;
    uint8_t pageNumber = 0;
    core::target_addr addr = core::mem_remap::target(flat_addr);
    switch (addr.space) {

    case core::target_addr::AS_CODE:
    case core::target_addr::AS_CODE_STATIC:
      gSession.target()->read_code(addr.addr, readByteLength, returnPointer);
      return true;

    case core::target_addr::AS_ISTACK:
    case core::target_addr::AS_IRAM_LOW:
    case core::target_addr::AS_INT_RAM:
      gSession.target()->read_data(addr.addr, readByteLength, returnPointer);
      return true;

    case core::target_addr::AS_XSTACK:
    case core::target_addr::AS_EXT_RAM:
      gSession.target()->read_xdata(addr.addr, readByteLength, returnPointer);
      return true;

    case core::target_addr::AS_SFR:
      // extract the SFR page number from the flat address
      // @FIXME: should properly implement in memremap...
      pageNumber = ((flat_addr & 0xFF00) >> 8);
      gSession.target()->read_sfr(addr.addr, pageNumber, readByteLength, returnPointer);
      return true;
    default:
      core::log::printf("ERROR: invalid memory area '%c'\n", addr.space);
      return false;
    }
  }

  bool CmdChange::direct(ParseCmd::Args cmd) {
    uint32_t flat_addr;
    uint16_t intValue;
    unsigned char charValue;

    if ((cmd.size() != 3) || (strcmp("=", cmd[1].c_str()))) {
      core::log::printf("ERROR: format must be $register/memory = value\n");
      return false;
    }
    // figure out value to assign
    intValue = (uint16_t)strtoul(cmd[2].c_str(), NULL, 0);
    charValue = (unsigned char)strtoul(cmd[2].c_str(), NULL, 0);

    // check if the target is a register
    if (cmd[0][0] == '$') {
      if (strcmp(cmd[0].c_str(), "$a") == 0) {
        core::log::printf("setting acc to %d\n", charValue);
        gSession.target()->write_sfr(0xe0, 0, 1, &charValue);
        return true;
      } else if (strcmp(cmd[0].c_str(), "$pc") == 0) {
        core::log::printf("setting pc to %d\n", intValue);
        gSession.target()->write_PC(intValue);
        return true;
      } else if (strcmp(cmd[0].c_str(), "$dptr") == 0) {
        core::log::printf("setting dptr to %d\n", intValue);
        // set DPL
        charValue = intValue % 256;
        gSession.target()->write_sfr(0x82, 0, 1, &charValue);
        // set DPH
        charValue = intValue / 256;
        gSession.target()->write_sfr(0x83, 0, 1, &charValue);
        return true;
      }
      return false;
    }

    // target isn't a register, so try and figure out memory location
    flat_addr = strtoul(cmd[0].c_str(), 0, 0);
    return writeMem(flat_addr, 1, &charValue);
  }

  bool CmdChange::writeMem(uint32_t flat_addr, unsigned int byteLength, unsigned char *writePointer) {
    unsigned char b;
    char area;
    core::target_addr addr = core::mem_remap::target(flat_addr);
    switch (area) {
    case 'c':
      // can't write code memory, so return false
      core::log::printf("ERROR: can't write to code area\n");
      return false;
    case 'd':
      gSession.target()->write_data(addr.addr, byteLength, writePointer);
      return true;
    case 'x':
      gSession.target()->write_xdata(addr.addr, byteLength, writePointer);
      return true;
    case 'i':
      gSession.target()->write_data(addr.addr + 0x100, byteLength, writePointer); // @FIXME: the offset is incorrect and we probably need a target function for accessing idata
      return true;
    case 's':
      gSession.target()->write_sfr(addr.addr, 0, byteLength, writePointer);
      return true;
    default:
      core::log::printf("ERROR: invalid memory area '%c'\n", area);
      return false;
    }
  }

} // namespace debug