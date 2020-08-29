#pragma once

#include <stdint.h>

namespace debug::core {

  /** Line number in a file, 1 = first line, 0 is invalid
*/
  typedef uint32_t LINE_NUM;

#define LINE_INVALID 0

  /** Address in the targets format.
	-ve values indicate an error,
	+ve values are assumed valid if returned from a function
*/
  typedef int32_t ADDR;
  static constexpr ADDR INVALID_ADDR = -1;

  typedef uint32_t FLAT_ADDR;
  static constexpr FLAT_ADDR INVALID_FLAT_ADDR = 0xffffffff;

  /** Breakpoint ID
	-ve indicates invalid breakpoint
*/
  typedef int32_t BP_ID;
  static constexpr BP_ID BP_ID_INVALID = -1;

  typedef int32_t BLOCK;
  typedef int32_t LEVEL;

} // namespace debug::core
