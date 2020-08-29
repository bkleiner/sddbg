#include "targetsilabs.h"

#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "ec2drv.h"

namespace debug::core {

  target_silabs::target_silabs()
      : target()
      , running(false)
      , debugger_port("/dev/ttyS0")
      , is_connected_flag(false) {
    obj.mode = AUTO;
  }

  target_silabs::~target_silabs() {
    running = false;
    if (run_thread)
      pthread_join(run_thread, NULL); // wait for thread to stop
  }

  bool target_silabs::connect() {
    if (ec2_connect(&obj, debugger_port.c_str())) {
      is_connected_flag = true;
      return true;
    } else
      return false;
  }

  bool target_silabs::disconnect() {
    if (is_connected()) {
      ec2_disconnect(&obj);
      is_connected_flag = false;
      return true;
    }
    return false;
  }

  bool target_silabs::is_connected() {
    return is_connected_flag;
  }

  std::string target_silabs::port() {
    return debugger_port;
  }

  bool target_silabs::set_port(std::string port) {
    debugger_port = port;
    return true;
  }

  std::string target_silabs::target_name() {
    return "SL51";
  }

  std::string target_silabs::target_descr() {
    return "Silicon Laboritories Debug adapter EC2 / EC3";
  }

  std::string target_silabs::device() {
    running = false;
    return obj.dev ? obj.dev->name : "unknown";
  }

  bool target_silabs::command(std::string cmd) {
    if (cmd == "special") {
      char buf[19];
      ec2_read_ram_sfr(&obj, buf, 0x20, sizeof(buf), true);
      std::cout << "Special register area:" << std::endl;
      print_buf_dump(buf, sizeof(buf));
      return true;
    } else if (cmd == "mode=C2") {
      obj.mode = C2;
      std::cout << "MODE = C2" << std::endl;
      return true;
    } else if (cmd == "mode=JTAG") {
      obj.mode = JTAG;
      std::cout << "MODE = JTAG" << std::endl;
      return true;
    } else if (cmd == "mode AUTO") {
      obj.mode = AUTO;
      std::cout << "MODE = AUTO" << std::endl;
      return true;
    }
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Device control
  ///////////////////////////////////////////////////////////////////////////////

  void target_silabs::reset() {
    std::cout << "Resetting target." << std::endl;
    ec2_target_reset(&obj);
  }

  uint16_t target_silabs::step() {
    force_stop = false;
    return ec2_step(&obj);
  }

  bool target_silabs::add_breakpoint(uint16_t addr) {
    std::cout << "adding breakpoint to silabs device" << std::endl;
    ec2_addBreakpoint(&obj, addr);
    return true;
  }

  bool target_silabs::del_breakpoint(uint16_t addr) {
    std::cout << "bool target_silabs::del_breakpoint(uint16_t addr)" << std::endl;
    return ec2_removeBreakpoint(&obj, addr);
  }

  void target_silabs::clear_all_breakpoints() {
    ec2_clear_all_bp(&obj);
  }

  void target_silabs::run_to_bp(int ignore_cnt) {
    std::cout << "starting a run now..." << std::endl;
    running = TRUE;
    force_stop = false;
    int i = 0;

    //obj.debug = true;
    do {
      //ec2_target_run_bp( &obj, &running );
      ec2_target_go(&obj);
      while (!ec2_target_halt_poll(&obj)) {
        usleep(250);
        if (!running) {
          ec2_target_halt(&obj);
          return; // someone stopped us early
        }
      }
    } while ((i++) != ignore_cnt);
  }

  /** Start the target running.
	You must now call poll_for_halt to determin when the target has halted.
	calling stop will cause the target to be halted.
*/
  void target_silabs::go() {
    ec2_target_go(&obj);
  }

  /** Call after starting the target running to determnin if the traget has halted
	at a breakpoint or for some other reason.
*/
  bool target_silabs::poll_for_halt() {
    //std::cout <<"Poll for halt....."<<std::endl;
    return ec2_target_halt_poll(&obj);
  }

  bool target_silabs::is_running() {
    return running;
  }

  void target_silabs::stop() {
    std::cout << "Stopping....." << std::endl;
    target::stop();
    running = FALSE;
  }

  void target_silabs::stop2() {
    target::stop();
    std::cout << "Stopping....." << std::endl;
    ec2_target_halt_no_wait(&obj);
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Memory reads
  ///////////////////////////////////////////////////////////////////////////////

  void target_silabs::read_data(uint8_t addr, uint8_t len, unsigned char *buf) {
    ec2_read_ram(&obj, (char *)buf, addr, len);
  }

  /** @DEPRECIATED
*/
  void target_silabs::read_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
    for (uint16_t offset = 0; offset < len; offset++)
      ec2_read_sfr(&obj, (char *)&buf[offset], addr + offset);
  }

  void target_silabs::read_sfr(uint8_t addr,
                               uint8_t page,
                               uint8_t len,
                               unsigned char *buf) {
    BOOL ok;
    SFRREG sfr_reg;
    for (uint16_t offset = 0; offset < len; offset++) {
      sfr_reg.addr = addr + offset;
      sfr_reg.page = page;
      buf[offset] = ec2_read_paged_sfr(&obj, sfr_reg, &ok);
    }
  }

  void target_silabs::read_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
    ec2_read_xdata(&obj, (char *)buf, addr, len);
  }

  void target_silabs::read_code(uint32_t addr, int len, unsigned char *buf) {

    ec2_read_flash(&obj, buf, addr, len);
  }

  uint16_t target_silabs::read_PC() {
    return ec2_read_pc(&obj);
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Memory writes
  ///////////////////////////////////////////////////////////////////////////////
  void target_silabs::write_data(uint8_t addr, uint8_t len, unsigned char *buf) {
    ec2_write_ram(&obj, (char *)buf, addr, len);
  }

  /** @DEPRECIATED
*/
  void target_silabs::write_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
    for (uint16_t offset = 0; offset < len; offset++)
      ec2_write_sfr(&obj, buf[offset], addr + offset);
  }

  void target_silabs::write_sfr(uint8_t addr,
                                uint8_t page,
                                uint8_t len,
                                unsigned char *buf) {
    target::write_sfr(addr, page, len, buf);
    BOOL ok;
    SFRREG sfr_reg;

    for (uint16_t offset = 0; offset < len; offset++) {
      sfr_reg.page = page;
      sfr_reg.addr = addr + offset;
      buf[offset] = ec2_write_paged_sfr(&obj, sfr_reg, buf[offset]);
    }
  }

  void target_silabs::write_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
    ec2_write_xdata(&obj, (char *)buf, addr, len);
  }

  void target_silabs::write_code(uint16_t addr, int len, unsigned char *buf) {
    std::cout << "Writing to flash with auto erase as necessary" << std::endl;
    printf("\tWriting %d bytes at 0x%04x\n", len, addr);
    // also erase scratchpad, since we may be using that for storage
    std::cout << "Erasing scratchpad";
    ec2_erase_flash_scratchpad(&obj);
    if (ec2_write_flash_auto_erase(&obj, buf, (int)addr, len))
      std::cout << "Flash write successful." << std::endl;
    else
      std::cout << "ERROR: Flash write Failed." << std::endl;
  }

  void target_silabs::write_PC(uint16_t addr) {
    ec2_set_pc(&obj, addr);
  }
} // namespace debug::core