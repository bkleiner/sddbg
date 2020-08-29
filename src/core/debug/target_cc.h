#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "cc_debugger.h"
#include "target.h"

namespace debug::core {
  class target_cc : public target {
  public:
    target_cc();

    bool connect();
    bool disconnect();
    bool is_connected();

    std::string port();
    bool set_port(std::string port);

    std::string target_name();
    std::string target_descr();
    std::string device();

    uint32_t max_breakpoints() { return 4; }

    bool is_running();
    void reset();
    uint16_t step();
    void run_to_bp(int ignore_cnt = 0);
    void go();
    void stop();

    bool add_breakpoint(uint16_t addr);
    bool del_breakpoint(uint16_t addr);
    void clear_all_breakpoints();

    // memory reads
    void read_data(uint8_t addr, uint8_t len, unsigned char *buf);
    void read_sfr(uint8_t addr, uint8_t len, unsigned char *buf);
    void read_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf);
    void read_xdata(uint16_t addr, uint16_t len, unsigned char *buf);
    void read_code(uint32_t addr, int len, unsigned char *buf);
    uint16_t read_PC();

    // memory writes
    void write_data(uint8_t addr, uint8_t len, unsigned char *buf);
    void write_sfr(uint8_t addr, uint8_t len, unsigned char *buf);
    void write_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf);
    void write_xdata(uint16_t addr, uint16_t len, unsigned char *buf);
    void write_code(uint16_t addr, int len, unsigned char *buf);
    void write_PC(uint16_t addr);

  private:
    std::string _port;
    std::unique_ptr<driver::cc_debugger> dev;
  };
} // namespace debug::core