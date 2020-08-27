#pragma once

#include <string>
#include <vector>

namespace driver::core {
  class serial {
  public:
    serial(std::string port);
    ~serial();

    void read(uint8_t *data, size_t size);
    void write(uint8_t *data, size_t size);

  private:
    std::string _port;

    int fd;
  };
} // namespace driver::core