#include <fmt/format.h>

#include "cc_debugger.h"

void print_cfg(uint16_t cfg) {
  if ((cfg & 0x10) != 0)
    fmt::print(" [X] SOFT_POWER_MODE\n");
  else
    fmt::print(" [ ] SOFT_POWER_MODE\n");

  if ((cfg & 0x08) != 0)
    fmt::print(" [X] TIMERS_OFF\n");
  else
    fmt::print(" [ ] TIMERS_OFF\n");

  if ((cfg & 0x04) != 0)
    fmt::print(" [X] DMA_PAUSE\n");
  else
    fmt::print(" [ ] DMA_PAUSE\n");

  if ((cfg & 0x02) != 0)
    fmt::print(" [X] TIMER_SUSPEND\n");
  else
    fmt::print(" [ ] TIMER_SUSPEND\n");
}

void print_status(uint16_t status) {
  if ((status & 0x80) != 0)
    fmt::print(" [X] CHIP_ERASE_DONE\n");
  else
    fmt::print(" [ ] CHIP_ERASE_DONE\n");

  if ((status & 0x40) != 0)
    fmt::print(" [X] PCON_IDLE\n");
  else
    fmt::print(" [ ] PCON_IDLE\n");

  if ((status & 0x20) != 0)
    fmt::print(" [X] CPU_HALTED\n");
  else
    fmt::print(" [ ] CPU_HALTED\n");

  if ((status & 0x10) != 0)
    fmt::print(" [X] PM_ACTIVE\n");
  else
    fmt::print(" [ ] PM_ACTIVE\n");

  if ((status & 0x08) != 0)
    fmt::print(" [X] HALT_STATUS\n");
  else
    fmt::print(" [ ] HALT_STATUS\n");

  if ((status & 0x04) != 0)
    fmt::print(" [X] DEBUG_LOCKED\n");
  else
    fmt::print(" [ ] DEBUG_LOCKED\n");

  if ((status & 0x02) != 0)
    fmt::print(" [X] OSCILLATOR_STABLE\n");
  else
    fmt::print(" [ ] OSCILLATOR_STABLE\n");

  if ((status & 0x01) != 0)
    fmt::print(" [X] STACK_OVERFLOW\n");
  else
    fmt::print(" [ ] STACK_OVERFLOW\n");
}

void info(driver::cc_debugger &dev) {
  auto info = dev.info();

  fmt::print("\nchip info:\n");
  fmt::print("     chip_id : {:#04x}\n", dev.chip_id().response);
  fmt::print("  flash_size : {} Kb\n", info.flash);
  fmt::print("   sram_size : {} Kb\n", info.sram);
  fmt::print("         usb : {}\n", info.usb);

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