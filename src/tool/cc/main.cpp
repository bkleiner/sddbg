#include "serial.h"

#include <fmt/format.h>

#include "cc_debugger.h"

int main(int argc, char *argv[]) {
  core::driver::serial serial("/dev/ttyACM0");

  cc_debugger_request req = {
      CC_CMD_PING,
      {0, 0, 0},
  };
  serial.write((uint8_t *)&req, sizeof(cc_debugger_request));
  fmt::print("sent {:#x} | {:#x} {:#x} {:#x}\n", req.cmd, req.payload[0], req.payload[1], req.payload[2]);

  cc_debugger_response res;
  serial.read((uint8_t *)&res, sizeof(cc_debugger_response));
  fmt::print("recv {:#x} | {:#x} {:#x}\n", res.ans, res.payload[0], res.payload[1]);
}