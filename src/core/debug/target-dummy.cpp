#include "target-dummy.h"

#include <cstring>
#include <iostream>

namespace debug::core {

  target_dummy::target_dummy()
      : target() {
  }

  target_dummy::~target_dummy() {
  }

  bool target_dummy::connect() {
    is_connected_flag = true;
    return is_connected_flag;
  }

  bool target_dummy::disconnect() {
    is_connected_flag = false;
    return is_connected_flag;
  }

  bool target_dummy::is_connected() {
    return is_connected_flag;
  }

  std::string target_dummy::port() {
    return "<sink>";
  }

  bool target_dummy::set_port(std::string port) {
    return false;
  }

  std::string target_dummy::target_name() {
    return "<none>";
  }

  std::string target_dummy::target_descr() {
    return "Command sink, this is not a real target";
  }

  std::string target_dummy::device() {
    return "sink";
  }

  bool target_dummy::command(std::string cmd) {
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Device control
  ///////////////////////////////////////////////////////////////////////////////

  void target_dummy::reset() {
    std::cout << "Resetting target." << std::endl;
  }

  uint16_t target_dummy::step() {
    return 0;
  }

  bool target_dummy::add_breakpoint(uint16_t addr) {
    std::cout << "adding breakpoint to sink device" << std::endl;
    return true;
  }

  bool target_dummy::del_breakpoint(uint16_t addr) {
    std::cout << "bool target_dummy::del_breakpoint(uint16_t addr)" << std::endl;
    return true;
  }

  void target_dummy::clear_all_breakpoints() {
    std::cout << "bool target_dummy::clear_all_breakpoints()" << std::endl;
  }

  void target_dummy::run_to_bp(int ignore_cnt) {
    std::cout << "target_dummy::run_to_bp(int ignore_cnt)" << std::endl;
  }

  bool target_dummy::is_running() {
    return false;
  }

  void target_dummy::stop() {
    target::stop();
    std::cout << "Stopping....." << std::endl;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Memory reads
  ///////////////////////////////////////////////////////////////////////////////

  void target_dummy::read_data(uint8_t addr, uint8_t len, unsigned char *buf) {
    memset(buf, 0x55, len);
  }

  /** @DEPRECIATED
*/
  void target_dummy::read_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
    memset(buf, 0x55, len);
  }

  void target_dummy::read_sfr(uint8_t addr,
                              uint8_t page,
                              uint8_t len,
                              unsigned char *buf) {
    memset(buf, 0x55, len);
  }

  void target_dummy::read_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
    memset(buf, 0x55, len);
  }

  void target_dummy::read_code(uint32_t addr, int len, unsigned char *buf) {
    memset(buf, 0x55, len);
  }

  uint16_t target_dummy::read_PC() {
    return 0x1234;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Memory writes
  ///////////////////////////////////////////////////////////////////////////////
  void target_dummy::write_data(uint8_t addr, uint8_t len, unsigned char *buf) {
  }

  /** @DEPRECIATED
*/
  void target_dummy::write_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
  }

  void target_dummy::write_sfr(uint8_t addr,
                               uint8_t page,
                               uint8_t len,
                               unsigned char *buf) {
  }

  void target_dummy::write_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
  }

  void target_dummy::write_code(uint16_t addr, int len, unsigned char *buf) {
  }

  void target_dummy::write_PC(uint16_t addr) {
  }

} // namespace debug::core