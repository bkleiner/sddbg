#include "cc_debugger.h"

#include <fmt/format.h>

#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))

struct cc_breakpoint_cfg {
  uint8_t _reserved : 2;
  uint8_t enabled : 1;
  uint8_t num : 2;
  uint8_t _unused : 3;
};

namespace driver {
  std::map<uint32_t, cc_chip_info> cc_debugger::chip_info_map = {
      {0x8100, {16, 0x400, 0x800, false, 2, 2}},
  };

  cc_debugger::cc_debugger(std::string port)
      : serial(port) {
    for (size_t i = 0; i < breakpoints.size(); i++) {
      breakpoints[i] = {
          false,
          0x0,
      };
    }
  }

  cc_debugger_response cc_debugger::send(cc_debugger_request req) {
    cc_debugger_response res;
    serial.write(reinterpret_cast<uint8_t *>(&req), sizeof(cc_debugger_request));
    serial.read(reinterpret_cast<uint8_t *>(&res), sizeof(cc_debugger_response));
    return res;
  }

  cc_debugger::response_or_error cc_debugger::send_frame(cc_debugger_request req) {
    auto res = send(req);
    if (res.ans == ANS_ERROR) {
      return fmt::format("cc debugger error {:#x}", res.payload[1]);
    }
    if (res.ans != ANS_OK) {
      if (res.ans == ANS_READY) {
        return ANS_READY;
      }
    }
    return (res.payload[0] << 8) | res.payload[1];
  }

  bool cc_debugger::detect() {
    if (!ping()) {
      return false;
    }

    const auto id = chip_id();
    if (!id) {
      fmt::print(id.error);
      return false;
    }

    auto it = chip_info_map.find(id.response & 0xFF00);
    if (it == chip_info_map.end()) {
      return false;
    }
    chip_info = (*it).second;

    // init clock
    instr(0x75, 0xC6, 0x00);

    return true;
  }

  cc_chip_info cc_debugger::info() {
    return chip_info;
  }

  bool cc_debugger::ping() {
    auto res = send({
        driver::CC_CMD_PING,
        {0, 0, 0},
    });
    return res.ans == ANS_OK;
  }

  bool cc_debugger::enter() {
    auto res = send_frame({
        driver::CC_CMD_ENTER,
        {0, 0, 0},
    });
    return res;
  }

  bool cc_debugger::exit() {
    auto res = send_frame({
        driver::CC_CMD_EXIT,
        {0, 0, 0},
    });
    return res;
  }

  bool cc_debugger::step() {
    auto res = send_frame({
        driver::CC_CMD_STEP,
        {0, 0, 0},
    });
    return res;
  }

  cc_debugger::response_or_error cc_debugger::instr(uint8_t c1) {
    return send_frame({
        CC_CMD_EXEC_1,
        {c1, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::instr(uint8_t c1, uint8_t c2) {
    return send_frame({
        CC_CMD_EXEC_2,
        {c1, c2, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::instr(uint8_t c1, uint8_t c2, uint8_t c3) {
    return send_frame({
        CC_CMD_EXEC_3,
        {c1, c2, c3},
    });
  }

  cc_debugger::response_or_error cc_debugger::chip_id() {
    return send_frame({
        driver::CC_CMD_CHIP_ID,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::status() {
    return send_frame({
        driver::CC_CMD_STATUS,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::pc() {
    return send_frame({
        driver::CC_CMD_PC,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::read_config() {
    return send_frame({
        driver::CC_CMD_RD_CFG,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::write_config(uint8_t cfg) {
    return send_frame({
        driver::CC_CMD_WR_CFG,
        {cfg, 0, 0},
    });
  }

  bool cc_debugger::chip_erase() {
    instr(0x0);
    return send_frame({
        driver::CC_CMD_CHPERASE,
        {0, 0, 0},
    });
  }

  bool cc_debugger::resume() {
    return send_frame({
        driver::CC_CMD_RESUME,
        {0, 0, 0},
    });
  }

  bool cc_debugger::halt() {
    return send_frame({
        driver::CC_CMD_HALT,
        {0, 0, 0},
    });
  }

  bool cc_debugger::set_breakpoint(uint8_t id, bool enabled, uint16_t addr) {
    cc_breakpoint_cfg cfg = {
        0x0,
        uint8_t(enabled ? 0x1 : 0x0),
        id,
        0x0,
    };

    uint8_t c = *((uint8_t *)&cfg);

    breakpoints[id].enabled = enabled;
    breakpoints[id].addr = addr;

    return send_frame({
        driver::CC_CMD_SET_BREAKPOINT,
        {c, HIBYTE(addr), LOBYTE(addr)},
    });
  }

  bool cc_debugger::add_breakpoint(uint16_t addr) {
    uint8_t id = 0;
    for (auto &bp : breakpoints) {
      if (!bp.enabled) {
        break;
      }
      id++;
    }

    return set_breakpoint(id, true, addr);
  }

  bool cc_debugger::del_breakpoint(uint16_t addr) {
    uint8_t id = 0;
    for (auto &bp : breakpoints) {
      if (bp.addr == addr) {
        break;
      }
      id++;
    }

    return set_breakpoint(id, false, addr);
  }

  void cc_debugger::clear_all_breakpoints() {
    for (size_t i = 0; i < breakpoints.size(); i++) {
      set_breakpoint(i, false, 0x0);
    }
  }

  void cc_debugger::set_pc(uint16_t addr) {
    instr(0x02, HIBYTE(addr), LOBYTE(addr));
  }

  void cc_debugger::read_data_raw(uint8_t addr, uint8_t *buf, uint32_t size) {
    instr(0x78, addr); //MOV  R0, addr;
    for (int n = 0; n < size; n++) {
      auto res = instr(0xE6); //MOV A,@R0
      buf[n] = res.response;

      instr(0x08); //INC R0
    }
  }

  void cc_debugger::read_sfr_raw(uint8_t addr, uint8_t *buf, uint32_t size) {
    for (int n = 0; n < size; n++) {
      const uint8_t a = addr + n;
      auto res = instr(0xE5, a); //MOV A,addr
      buf[n] = res.response;
    }
  }

  void cc_debugger::read_code_raw(uint16_t addr, uint8_t *buf, uint32_t size) {
    const int bank = (addr >> 15) & 0x03;
    instr(0x75, 0xC7, bank * 16 + 1);
    addr = addr & 0xFFFF;

    instr(0x90, HIBYTE(addr), LOBYTE(addr)); //MOV DPTR, addr;

    for (int n = 0; n < size; n++) {
      instr(0xE4);            // CLR A
      auto res = instr(0x93); // MOVC A, @A+DPTR
      instr(0xA3);            // INC DPTR;
      buf[n] = res.response;
    }
  }

  void cc_debugger::read_xdata_raw(uint16_t addr, uint8_t *buf, uint32_t size) {
    instr(0x90, HIBYTE(addr), LOBYTE(addr)); //MOV DPTR, addr;
    for (int n = 0; n < size; n++) {
      auto res = instr(0xE0); //MOVX A, @DPTR;
      buf[n] = res.response;

      instr(0xA3); //INC DPTR;
    }
  }

  void cc_debugger::write_data_raw(uint8_t addr, uint8_t *buf, uint32_t size) {
    instr(0x78, addr); //MOV  R0, addr;
    for (int n = 0; n < size; n++) {
      instr(0x74, buf[n]); // MOV A, #inputArray[n]
      instr(0xF6);         //MOV @R0,A
      instr(0x08);         //INC R0
    }
  }

  void cc_debugger::write_xdata_raw(uint16_t addr, uint8_t *buf, uint32_t size) {
    instr(0x90, HIBYTE(addr), LOBYTE(addr)); //MOV DPTR, addr;
    for (int n = 0; n < size; n++) {
      instr(0x74, buf[n]); // MOV A, #inputArray[n]
      instr(0xF0);         // MOVX @DPTR, A
      instr(0xA3);         // INC DPTR
    }
  }

  void cc_debugger::write_code_raw(uint16_t addr, uint8_t *buf, uint32_t size) {
    const uint8_t FLASH_WORD_SIZE = chip_info.word_size;
    const uint16_t WORDS_PER_FLASH_PAGE = chip_info.page_size / FLASH_WORD_SIZE;
    const uint8_t imm = ((addr >> 8) / FLASH_WORD_SIZE) & 0x7E;

    uint8_t routine_8[] = {
        0x75, 0xAD, imm,  // MOV FADDRH, #imm;
        0x75, 0xAC, 0x00, // MOV FADDRL, #00;
        0x75, 0xAE, 0x01, // MOV FLC, #01H; // ERASE
        // ; Wait for flash erase to complete
        0xE5, 0xAE,                         // eraseWaitLoop: MOV A, FLC;
        0x20, 0xE7, 0xFB,                   // JB ACC_BUSY, eraseWaitLoop;
                                            // ; Initialize the data pointer
        0x90, 0xF0, 0x00,                   // MOV DPTR, #0F000H;
                                            // ; Outer loops
        0x7F, HIBYTE(WORDS_PER_FLASH_PAGE), // MOV R7, #imm;
        0x7E, LOBYTE(WORDS_PER_FLASH_PAGE), // MOV R6, #imm;
        0x75, 0xAE, 0x02,                   // MOV FLC, #02H; // WRITE
                                            // ; Inner loops
        0x7D, FLASH_WORD_SIZE,              // writeLoop: MOV R5, #imm;
        0xE0,                               // writeWordLoop: MOVX A, @DPTR;
        0xA3,                               // INC DPTR;
        0xF5, 0xAF,                         // MOV FWDATA, A;
        0xDD, 0xFA,                         // DJNZ R5, writeWordLoop;
                                            // ; Wait for completion
        0xE5, 0xAE,                         // writeWaitLoop: MOV A, FLC;
        0x20, 0xE6, 0xFB,                   // JB ACC_SWBSY, writeWaitLoop;
        0xDE, 0xF1,                         // DJNZ R6, writeLoop;
        0xDF, 0xEF,                         // DJNZ R7, writeLoop;
                                            // ; Done, fake a breakpoint
        0xA5                                // DB 0xA5;
    };

    uint8_t write_buffer[chip_info.page_size];
    memcpy(write_buffer, buf, size);

    const uint32_t fill_size = chip_info.page_size - size;
    if (fill_size > 0) {
      memset(write_buffer + size, 0xFF, fill_size);
    }

    write_xdata_raw(0xF000, write_buffer, chip_info.page_size);
    write_xdata_raw(0xF000 + chip_info.page_size, routine_8, sizeof(routine_8));
    instr(0x75, 0xC7, 0x51); // MOV MEMCTR, (bank * 16) + 1
    set_pc(0xF000 + chip_info.page_size);
    resume();

    while (!(status().response & CC_STATUS_CPU_HALTED))
      ;
  }

} // namespace driver