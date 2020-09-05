#pragma once

#include <memory>
#include <string>

#include <fmt/format.h>
#include <fmt/printf.h>

namespace debug::core::log {

  class logger {
  public:
    virtual void write(std::string str) = 0;
  };

  class std_logger : public logger {
  public:
    std_logger(std::FILE *fd);

    virtual void write(std::string str) override;

  private:
    std::FILE *fd;
  };

  extern std::unique_ptr<logger> log_ouput;

  template <typename... args_type>
  void print(std::string fmt, args_type &&... args) {
    log_ouput->write(fmt::format(fmt, std::forward<args_type>(args)...));
  }

  template <typename... args_type>
  void printf(std::string fmt, args_type &&... args) {
    log_ouput->write(fmt::sprintf(fmt, std::forward<args_type>(args)...));
  }

} // namespace debug::core::log