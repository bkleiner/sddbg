#include "target_cc.h"

#include <chrono>
#include <thread>

#include "log.h"

namespace debug::core {

  target_cc::target_cc()
      : _port("/dev/ttyACM0")
      , halted_by_breakpoint(false) {
  }

  bool target_cc::connect() {
    dev = std::make_unique<driver::cc_debugger>(port());
    if (!dev->detect()) {
      return false;
    }
    dev->enter();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
  }

  bool target_cc::disconnect() {
    if (is_connected())
      dev->exit();

    dev.release();
    return true;
  }

  bool target_cc::load_file(std::string name) {
    if (!dev->chip_erase()) {
      return false;
    }

    uint16_t status = 0;
    while ((status & driver::CC_STATUS_CHIP_ERASE_DONE) == 0) {
      log::print("erasing...\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      status = dev->status();
    }

    return target::load_file(name);
  }

  bool target_cc::is_connected() {
    return dev != nullptr;
  }

  std::string target_cc::port() {
    return _port;
  }

  bool target_cc::set_port(std::string port) {
    _port = port;
    return true;
  }

  std::string target_cc::target_name() {
    return "CC";
  }

  std::string target_cc::target_descr() {
    return "";
  }

  std::string target_cc::device() {
    return "";
  }

  bool target_cc::is_running() {
    if (!is_connected()) {
      return false;
    }

    const uint16_t status = dev->status();
    return (status & driver::CC_STATUS_CPU_HALTED) == 0;
  }

  void target_cc::reset() {
    disconnect();
    if (!connect()) {
      throw std::runtime_error("target_cc detect failed!");
    }
    halted_by_breakpoint = false;
    write_PC(0x0);
  }

  uint16_t target_cc::step() {
    dev->step();
    halted_by_breakpoint = false;
    return read_PC();
  }

  void target_cc::go() {
    if (is_connected() && !is_running()) {
      dev->resume();
      halted_by_breakpoint = false;
    }
  }

  void target_cc::run_to_bp(int ignore_cnt) {
    if (!is_running()) {
      go();
    }

    do {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      log::print("PC {:#x}\n", dev->pc().response);
    } while (is_running());

    halted_by_breakpoint = true;
  }

  void target_cc::stop() {
    if (is_running()) {
      dev->halt();
      halted_by_breakpoint = false;
    }
    target::stop();
  }

  bool target_cc::add_breakpoint(uint16_t addr) {
    if (!is_connected() || is_running()) {
      return false;
    }
    return dev->add_breakpoint(addr);
  }

  bool target_cc::del_breakpoint(uint16_t addr) {
    if (!is_connected() || is_running()) {
      return false;
    }
    return dev->del_breakpoint(addr);
  }

  void target_cc::clear_all_breakpoints() {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to clear_all_breakpoints on running target\n");
      return;
    }
    dev->clear_all_breakpoints();
  }

  // memory reads
  void target_cc::read_data(uint8_t addr, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to read_data on running target\n");
      return;
    }
    dev->read_data_raw(addr, buf, len);
  }

  void target_cc::read_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to read_sfr on running target\n");
      return;
    }
    dev->read_sfr_raw(addr, buf, len);
  }

  void target_cc::read_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to read_sfr on running target\n");
      return;
    }
    dev->read_sfr_raw(addr, buf, len);
  }

  void target_cc::read_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to read_xdata on running target\n");
      return;
    }

    dev->read_xdata_raw(addr, buf, len);
  }

  void target_cc::read_code(uint32_t addr, int len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to read_code on running target\n");
      return;
    }

    const auto page_size = dev->info().page_size;
    const auto pages = len / page_size + (len % page_size ? 1 : 0);

    for (int i = 0; i < pages; i++) {
      const auto offset = i * page_size;
      const uint32_t size = std::min(page_size - (addr - offset) % page_size, uint32_t(len - offset));
      dev->read_code_raw(addr + offset, buf + offset, size);
    }
  }

  uint16_t target_cc::read_PC() {
    const uint16_t pc = dev->pc();
    if (halted_by_breakpoint) {
      // if we halted because of a breakpoint, subtract the current instr
      return pc - 1;
    }
    return pc;
  }

  // memory writes
  void target_cc::write_data(uint8_t addr, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_data on running target\n");
      return;
    }
    dev->write_data_raw(addr, buf, len);
  }

  void target_cc::write_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_sfr on running target\n");
      return;
    }
  }

  void target_cc::write_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_sfr on running target\n");
      return;
    }
  }

  void target_cc::write_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_xdata on running target\n");
      return;
    }
    dev->write_xdata_raw(addr, buf, len);
  }

  void target_cc::write_code(uint16_t addr, int len, unsigned char *buf) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_code on running target\n");
      return;
    }

    const auto page_size = dev->info().page_size;
    const auto pages = len / page_size + (len % page_size ? 1 : 0);

    for (int i = 0; i < pages; i++) {
      const auto offset = i * page_size;
      const uint32_t size = std::min(page_size - (addr - offset) % page_size, len - offset);
      dev->write_code_raw(addr + offset, buf + offset, size);
    }
  }

  void target_cc::write_PC(uint16_t addr) {
    if (!is_connected() || is_running()) {
      log::print("target_cc: tried to write_PC on running target\n");
      return;
    }
    dev->set_pc(addr);
  }
} // namespace debug::core