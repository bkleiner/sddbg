#pragma once

#include <stdint.h>

namespace debug::core {

  typedef uint32_t LINE_NUM;
  static constexpr LINE_NUM LINE_INVALID = 0;

  typedef int32_t ADDR;
  static constexpr ADDR INVALID_ADDR = -1;

  typedef uint32_t FLAT_ADDR;
  static constexpr FLAT_ADDR INVALID_FLAT_ADDR = 0xffffffff;

  typedef int32_t BLOCK;
  typedef int32_t LEVEL;

} // namespace debug::core
