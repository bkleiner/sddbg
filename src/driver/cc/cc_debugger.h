#pragma once

#include <cstdint>

enum cc_debugger_cmd : uint8_t {
  CC_CMD_ENTER = 0x01,
  CC_CMD_EXIT = 0x02,
  CC_CMD_CHIP_ID = 0x03,
  CC_CMD_STATUS = 0x04,
  CC_CMD_PC = 0x05,
  CC_CMD_STEP = 0x06,
  CC_CMD_EXEC_1 = 0x07,
  CC_CMD_EXEC_2 = 0x08,
  CC_CMD_EXEC_3 = 0x09,
  CC_CMD_BRUSTWR = 0x0A,
  CC_CMD_RD_CFG = 0x0B,
  CC_CMD_WR_CFG = 0x0C,
  CC_CMD_CHPERASE = 0x0D,
  CC_CMD_RESUME = 0x0E,
  CC_CMD_HALT = 0x0F,
  CC_CMD_PING = 0xF0,
};

enum cc_debugger_answer : uint8_t {
  ANS_OK = 0x01,
  ANS_ERROR = 0x02,
  ANS_READY = 0x03,
};

struct cc_debugger_request {
  cc_debugger_cmd cmd;
  uint8_t payload[3];
};

struct cc_debugger_response {
  cc_debugger_answer ans;
  uint8_t payload[2];
};