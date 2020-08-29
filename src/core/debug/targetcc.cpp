#include "targetcc.h"

#include <chrono>
#include <thread>

#include <fmt/format.h>

target_cc::target_cc()
    : _port("/dev/ttyACM0") {
}

bool target_cc::connect() {
  dev = std::make_unique<driver::cc_debugger>(port());
  if (!dev->detect()) {
    return false;
  }
  dev->enter();
  return true;
}

bool target_cc::disconnect() {
  dev.release();
  return true;
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
  return "";
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

  return !(dev->status().response & driver::CC_STATUS_CPU_HALTED);
}

void target_cc::reset() {
  disconnect();
  connect();
  write_PC(0x0);
}

uint16_t target_cc::step() {
  dev->step();
  return dev->pc();
}

void target_cc::go() {
  dev->resume();
}

void target_cc::run_to_bp(int ignore_cnt) {
  go();

  while (is_running()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

bool target_cc::add_breakpoint(uint16_t addr) {
  return dev->add_breakpoint(addr - 1);
}

bool target_cc::del_breakpoint(uint16_t addr) {
  return dev->del_breakpoint(addr - 1);
}

void target_cc::clear_all_breakpoints() {
  if (!is_connected()) {
    return;
  }
  dev->clear_all_breakpoints();
}

// memory reads
void target_cc::read_data(uint8_t addr, uint8_t len, unsigned char *buf) {
  dev->read_data_raw(addr, buf, len);
}

void target_cc::read_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
  dev->read_sfr_raw(addr, buf, len);
}

void target_cc::read_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf) {
  dev->read_sfr_raw(addr, buf, len);
}

void target_cc::read_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
  dev->read_xdata_raw(addr, buf, len);
}

void target_cc::read_code(uint32_t addr, int len, unsigned char *buf) {
  const auto page_size = dev->info().page_size;
  const auto pages = len / page_size + (len % page_size ? 1 : 0);

  for (int i = 0; i < pages; i++) {
    const auto offset = i * page_size;
    dev->read_code_raw(addr + offset, buf + offset, page_size - offset % page_size);
  }
}

uint16_t target_cc::read_PC() {
  return dev->pc();
}

// memory writes
void target_cc::write_data(uint8_t addr, uint8_t len, unsigned char *buf) {
  dev->write_data_raw(addr, buf, len);
}

void target_cc::write_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
}
void target_cc::write_sfr(uint8_t addr, uint8_t page, uint8_t len, unsigned char *buf) {
}

void target_cc::write_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
  dev->write_xdata_raw(addr, buf, len);
}

void target_cc::write_code(uint16_t addr, int len, unsigned char *buf) {
  const auto page_size = dev->info().page_size;
  const auto pages = len / page_size + (len % page_size ? 1 : 0);

  for (int i = 0; i < pages; i++) {
    const auto offset = i * page_size;
    const uint32_t size = std::min(page_size - (addr - offset) % page_size, len - offset);
    dev->write_code_raw(addr + offset, buf + offset, size);
  }
}

void target_cc::write_PC(uint16_t addr) {
  dev->set_pc(addr);
}