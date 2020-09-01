#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <stdexcept>

#include "serial.h"

namespace driver {

  enum cc_debugger_cmd : uint8_t {
    CC_CMD_ENTER = 0x01,
    CC_CMD_EXIT = 0x02,
    CC_CMD_CHIP_ID = 0x03,
    CC_CMD_STATUS = 0x04,
    CC_CMD_PC = 0x05,
    CC_CMD_STEP = 0x06,
    CC_CMD_EXEC_1 = 0x07,
    CC_CMD_EXEC_2 = 0x08,
    CC_CMD_EXEC_3 = 0x09,
    CC_CMD_BRUSTWR = 0x0A,
    CC_CMD_RD_CFG = 0x0B,
    CC_CMD_WR_CFG = 0x0C,
    CC_CMD_CHPERASE = 0x0D,
    CC_CMD_RESUME = 0x0E,
    CC_CMD_HALT = 0x0F,
    CC_CMD_SET_BREAKPOINT = 0x10,
    CC_CMD_PING = 0xF0,
  };

  enum cc_debugger_answer : uint8_t {
    ANS_OK = 0x01,
    ANS_ERROR = 0x02,
    ANS_READY = 0x03,
  };

  enum cc_debugger_status : uint8_t {
    CC_STATUS_CHIP_ERASE_DONE = 0x80,
    CC_STATUS_PCON_IDLE = 0x40,
    CC_STATUS_CPU_HALTED = 0x20,
    CC_STATUS_POWER_MODE_0 = 0x10,
    CC_STATUS_HALT_STATUS = 0x08,
    CC_STATUS_DEBUG_LOCKED = 0x04,
    CC_STATUS_OSCILLATOR_STABLE = 0x02,
    CC_STATUS_STACK_OVERFLOW = 0x01,
  };

  struct cc_debugger_request {
    cc_debugger_cmd cmd;
    uint8_t payload[3];
  };

  struct cc_debugger_response {
    cc_debugger_answer ans;
    uint8_t payload[2];
  };

  struct cc_chip_info {
    uint16_t flash;
    uint16_t page_size;
    uint16_t block_size;
    bool usb;
    uint16_t sram;
    uint16_t word_size;
  };

  struct cc_breakpoint {
    bool enabled;
    uint16_t addr;
  };

  class cc_debugger {
  public:
    struct response_or_error {
      response_or_error() {}
      response_or_error(uint16_t res)
          : response(res) {}
      response_or_error(std::string err)
          : error(err) {}

      inline operator bool() const {
        return error.size() <= 0;
      }

      inline operator uint16_t() const {
        if (error.size()) {
          throw std::runtime_error("response error: " + error);
        }
        return response;
      }

      uint16_t response;
      std::string error;
    };

    cc_debugger(std::string port);

    bool ping();

    bool detect();
    cc_chip_info info();

    bool enter();
    bool exit();

    bool step();

    bool chip_erase();
    bool resume();
    bool halt();

    bool add_breakpoint(uint16_t addr);
    bool del_breakpoint(uint16_t addr);
    void clear_all_breakpoints();

    response_or_error chip_id();
    response_or_error status();
    response_or_error pc();
    void set_pc(uint16_t val);

    response_or_error read_config();
    response_or_error write_config(uint8_t cfg);

    response_or_error instr(uint8_t c1);
    response_or_error instr(uint8_t c1, uint8_t c2);
    response_or_error instr(uint8_t c1, uint8_t c2, uint8_t c3);

    void read_data_raw(uint8_t addr, uint8_t *buf, uint32_t size);
    void read_sfr_raw(uint8_t addr, uint8_t *buf, uint32_t size);
    void read_xdata_raw(uint16_t addr, uint8_t *buf, uint32_t size);
    void read_code_raw(uint16_t addr, uint8_t *buf, uint32_t size);

    void write_data_raw(uint8_t addr, uint8_t *buf, uint32_t size);
    void write_xdata_raw(uint16_t addr, uint8_t *buf, uint32_t size);
    void write_code_raw(uint16_t addr, uint8_t *buf, uint32_t size);

  private:
    static std::map<uint32_t, cc_chip_info> chip_info_map;
    std::mutex mu;
    core::serial serial;
    cc_chip_info chip_info;
    std::array<cc_breakpoint, 4> breakpoints;

    bool set_breakpoint(uint8_t id, bool enabled, uint16_t addr);

    cc_debugger_response send(cc_debugger_request req);
    response_or_error send_frame(cc_debugger_request req);
  };

} // namespace driver