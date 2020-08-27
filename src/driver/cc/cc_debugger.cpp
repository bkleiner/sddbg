#include "cc_debugger.h"

#include <fmt/format.h>

namespace driver {
  std::map<uint32_t, cc_chip_info> cc_debugger::chip_info_map = {
      {0x8100, {16 * 1024, 0x400, 0x800, false, 2 * 1024, 2}},
  };

  cc_debugger::cc_debugger(std::string port)
      : serial(port) {}

  cc_debugger_response cc_debugger::send(cc_debugger_request req) {
    cc_debugger_response res;
    serial.write(reinterpret_cast<uint8_t *>(&req), sizeof(cc_debugger_request));
    serial.read(reinterpret_cast<uint8_t *>(&res), sizeof(cc_debugger_response));
    return res;
  }

  cc_debugger::response_or_error cc_debugger::send_frame(cc_debugger_request req) {
    auto res = send(req);
    if (res.ans == ANS_ERROR) {
      return fmt::format("cc debugger error {:#x}", res.payload[1]);
    }
    if (res.ans != ANS_OK) {
      if (res.ans == ANS_READY) {
        return ANS_READY;
      }
    }
    return (res.payload[0] << 8) | res.payload[1];
  }

  bool cc_debugger::detect() {
    if (!ping()) {
      return false;
    }

    const auto id = chip_id();
    if (!id) {
      fmt::print(id.error);
      return false;
    }

    auto it = chip_info_map.find(id.response & 0xFF00);
    if (it == chip_info_map.end()) {
      return false;
    }

    chip_info = (*it).second;
    return true;
  }

  cc_chip_info cc_debugger::info() {
    return chip_info;
  }

  bool cc_debugger::ping() {
    auto res = send({
        driver::CC_CMD_PING,
        {0, 0, 0},
    });
    return res.ans == ANS_OK;
  }

  cc_debugger::response_or_error cc_debugger::chip_id() {
    return send_frame({
        driver::CC_CMD_CHIP_ID,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::status() {
    return send_frame({
        driver::CC_CMD_STATUS,
        {0, 0, 0},
    });
  }

  cc_debugger::response_or_error cc_debugger::read_config() {
    return send_frame({
        driver::CC_CMD_RD_CFG,
        {0, 0, 0},
    });
  }

} // namespace driver