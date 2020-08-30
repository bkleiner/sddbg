#pragma once

#include <cstdint>
#include <string>

struct instuction {
  uint64_t code;
  char branch;
  uint8_t length;
  std::string mnemonic;
  bool is_call;
};

void dissasemble(uint8_t *buf, size_t size);