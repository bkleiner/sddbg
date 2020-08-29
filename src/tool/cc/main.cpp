#include <fmt/format.h>

#include "ihex.h"

#include "cc_debugger.h"

void print_cfg(uint16_t cfg) {
  fmt::print(" [{}] SOFT_POWER_MODE\n", (cfg & 0x10) != 0 ? "X" : " ");
  fmt::print(" [{}] TIMERS_OFF\n", (cfg & 0x08) != 0 ? "X" : " ");
  fmt::print(" [{}] DMA_PAUSE\n", (cfg & 0x04) != 0 ? "X" : " ");
  fmt::print(" [{}] TIMER_SUSPEND\n", (cfg & 0x02) != 0 ? "X" : " ");
}

void print_status(uint16_t status) {
  fmt::print(" [{}] CHIP_ERASE_DONE\n", (status & driver::CC_STATUS_CHIP_ERASE_DONE) != 0 ? "X" : " ");
  fmt::print(" [{}] PCON_IDLE\n", (status & driver::CC_STATUS_PCON_IDLE) != 0 ? "X" : " ");
  fmt::print(" [{}] CPU_HALTED\n", (status & driver::CC_STATUS_CPU_HALTED) != 0 ? "X" : " ");
  fmt::print(" [{}] PM_ACTIVE\n", (status & driver::CC_STATUS_POWER_MODE_0) != 0 ? "X" : " ");
  fmt::print(" [{}] HALT_STATUS\n", (status & driver::CC_STATUS_HALT_STATUS) != 0 ? "X" : " ");
  fmt::print(" [{}] DEBUG_LOCKED\n", (status & driver::CC_STATUS_DEBUG_LOCKED) != 0 ? "X" : " ");
  fmt::print(" [{}] OSCILLATOR_STABLE\n", (status & driver::CC_STATUS_OSCILLATOR_STABLE) != 0 ? "X" : " ");
  fmt::print(" [{}] STACK_OVERFLOW\n", (status & driver::CC_STATUS_STACK_OVERFLOW) != 0 ? "X" : " ");
}

void info(driver::cc_debugger &dev) {
  auto info = dev.info();

  fmt::print("\nchip info:\n");
  fmt::print("     chip_id : {:#04x}\n", dev.chip_id().response);
  fmt::print("  flash_size : {} Kb\n", info.flash * 1024);
  fmt::print("   sram_size : {} Kb\n", info.sram * 1024);
  fmt::print("         usb : {}\n", info.usb ? "yes" : "no");

  fmt::print("\ndebug status:\n");
  print_status(dev.status());

  fmt::print("\ndebug config:\n");
  print_cfg(dev.read_config());
}

void read_flash(driver::cc_debugger &dev) {
  dev.enter();

  const auto page_size = dev.info().page_size;
  const auto pages = dev.info().flash;

  const int size = pages * page_size;
  uint8_t data[size];

  for (int i = 0; i < pages; i++) {
    const auto addr = i * page_size;
    dev.read_code_raw(addr, data + addr, page_size);
  }

  ihex_save_file("test-dump.hex", (char *)data, 0, size);
}

void write_flash(driver::cc_debugger &dev) {
  const auto page_size = dev.info().page_size;
  const auto pages = dev.info().flash;

  const int size = std::max(pages * page_size, 65536);
  uint8_t data[size];

  uint32_t start = 0, end = 0;
  ihex_load_file("test.hex", (char *)data, &start, &end);

  dev.enter();

  for (int i = start; i < end; i += page_size) {
    dev.write_code_raw(i, data + i, page_size);
  }
}

int main(int argc, char *argv[]) {
  driver::cc_debugger dev("/dev/ttyACM0");
  if (!dev.detect()) {
    fmt::print("failed to detect chip\n");
    return -1;
  }

  write_flash(dev);
  read_flash(dev);
}