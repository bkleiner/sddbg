#include "log.h"

namespace debug::core::log {
  std_logger::std_logger(std::FILE *fd)
      : fd(fd){};

  void std_logger::write(std::string str) {
    std::fputs(str.c_str(), fd);
  }

  std::unique_ptr<logger> log_ouput = std::unique_ptr<logger>(new std_logger(stdout));
} // namespace debug::core::log