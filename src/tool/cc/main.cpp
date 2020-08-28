#include <fmt/format.h>

#include "cc_debugger.h"

void print_cfg(uint16_t cfg) {
  fmt::print(" [{}] SOFT_POWER_MODE\n", (cfg & 0x10) != 0 ? "X" : " ");
  fmt::print(" [{}] TIMERS_OFF\n", (cfg & 0x08) != 0 ? "X" : " ");
  fmt::print(" [{}] DMA_PAUSE\n", (cfg & 0x04) != 0 ? "X" : " ");
  fmt::print(" [{}] TIMER_SUSPEND\n", (cfg & 0x02) != 0 ? "X" : " ");
}

void print_status(uint16_t status) {
  fmt::print(" [{}] CHIP_ERASE_DONE\n", (status & 0x80) != 0 ? "X" : " ");
  fmt::print(" [{}] PCON_IDLE\n", (status & 0x40) != 0 ? "X" : " ");
  fmt::print(" [{}] CPU_HALTED\n", (status & 0x20) != 0 ? "X" : " ");
  fmt::print(" [{}] PM_ACTIVE\n", (status & 0x10) != 0 ? "X" : " ");
  fmt::print(" [{}] HALT_STATUS\n", (status & 0x08) != 0 ? "X" : " ");
  fmt::print(" [{}] DEBUG_LOCKED\n", (status & 0x04) != 0 ? "X" : " ");
  fmt::print(" [{}] OSCILLATOR_STABLE\n", (status & 0x02) != 0 ? "X" : " ");
  fmt::print(" [{}] STACK_OVERFLOW\n", (status & 0x01) != 0 ? "X" : " ");
}

void info(driver::cc_debugger &dev) {
  auto info = dev.info();

  fmt::print("\nchip info:\n");
  fmt::print("     chip_id : {:#04x}\n", dev.chip_id().response);
  fmt::print("  flash_size : {} Kb\n", info.flash);
  fmt::print("   sram_size : {} Kb\n", info.sram);
  fmt::print("         usb : {}\n", info.usb ? "yes" : "no");

  fmt::print("\ndebug status:\n");
  print_status(dev.status());

  fmt::print("\ndebug config:\n");
  print_cfg(dev.read_config());
}

int main(int argc, char *argv[]) {
  driver::cc_debugger dev("/dev/ttyACM0");
  if (!dev.detect()) {
    fmt::print("failed to detect chip\n");
    return -1;
  }

  info(dev);
}