#pragma once

#include "parsecmd.h"

namespace debug {

  class CmdX : public CmdShowSetInfoHelp {
  public:
    CmdX() {
      name = "X";
      unit_size = 4;
      num_units = 1;
      format = 'x';
    }
    bool direct(ParseCmd::Args cmd) override;

  protected:
    bool parseFormat(std::string token);
    bool readMem(uint32_t flat_addr, unsigned int readByteLength, unsigned char *returnPointer);
    int unit_size; ///< size of the units to print out in bytes
    int num_units; ///< number of unit sized object to output
    char format;   ///< Format specifier
  };

  class CmdChange : public CmdShowSetInfoHelp {
  public:
    CmdChange() {
      name = "change";
    }
    bool direct(ParseCmd::Args cmd) override;

  protected:
    bool writeMem(uint32_t flat_addr, unsigned int readByteLength, unsigned char *returnPointer);
  };

  class CmdDisassemble : public CmdShowSetInfoHelp {
  public:
    CmdDisassemble() { name = "Disassemble"; }
    bool direct(ParseCmd::Args cmd) override;
  };

} // namespace debug
