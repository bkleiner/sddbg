#pragma once

#include <stdint.h>

#include "target.h"

namespace debug::core {

  class target_dummy : public target {
  public:
    target_dummy();
    virtual ~target_dummy();
    virtual bool connect();
    virtual bool disconnect();
    virtual bool is_connected();
    virtual std::string port();
    virtual bool set_port(std::string port);
    virtual std::string target_name();
    virtual std::string target_descr();
    virtual std::string device();
    virtual uint32_t max_breakpoints() { return 4; }
    //	virtual bool load_file( std::string name );
    virtual bool command(std::string cmd);

    // device control
    virtual void reset();
    virtual uint16_t step();
    virtual bool add_breakpoint(uint16_t addr);
    virtual bool del_breakpoint(uint16_t addr);
    virtual void clear_all_breakpoints();
    virtual void run_to_bp(int ignore_cnt = 0);
    virtual bool is_running();
    virtual void stop();

    // memory reads
    virtual void read_data(uint8_t addr, uint8_t len, unsigned char *buf);
    virtual void read_sfr(uint8_t addr, uint8_t len, unsigned char *buf);
    virtual void read_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf);
    virtual void read_xdata(uint16_t addr, uint16_t len, unsigned char *buf);
    virtual void read_code(uint32_t addr, int len, unsigned char *buf);
    virtual uint16_t read_PC();

    // memory writes
    virtual void write_data(uint8_t addr, uint8_t len, unsigned char *buf);
    virtual void write_sfr(uint8_t addr, uint8_t len, unsigned char *buf);
    virtual void write_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf);
    virtual void write_xdata(uint16_t addr, uint16_t len, unsigned char *buf);
    virtual void write_code(uint16_t addr, int len, unsigned char *buf);
    virtual void write_PC(uint16_t addr);

  protected:
    bool is_connected_flag;
  };

} // namespace debug::core
