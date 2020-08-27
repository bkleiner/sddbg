#include "serial.h"

#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <features.h>
#include <termios.h> // POSIX terminal control definitions
#include <unistd.h>  // UNIX standard function definitions

namespace core::driver {

  serial::serial(std::string port)
      : _port(port)
      , fd(0) {
    fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
      throw std::runtime_error(strerror(errno));
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
      throw std::runtime_error(strerror(errno));
    }

    // Set the baud rates to 115200
    if (cfsetispeed(&tty, B115200) != 0) {
      throw std::runtime_error(strerror(errno));
    }
    if (cfsetospeed(&tty, B115200) != 0) {
      throw std::runtime_error(strerror(errno));
    }

    // Enable the receiver and set local mode...
    tty.c_cflag |= (CLOCAL | CREAD);

    // set 8N1
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // Disable hardware flow control
    tty.c_cflag &= ~CRTSCTS;

    // Disable software flow control
    tty.c_iflag = 0; // raw mode, no translations, no parity checking etc.

    // select RAW input
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // select raw output
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 1;

    // Set the new options for the port...
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      throw std::runtime_error(strerror(errno));
    }
  }

  serial::~serial() {
    close(fd);
  }

  void serial::read(uint8_t *data, size_t size) {
    size_t read = 0;
    while (read < size) {
      ssize_t n = ::read(fd, data + read, size - read);
      if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        throw std::runtime_error("socket write error");
      }
      read += n;
    }
  }

  void serial::write(uint8_t *data, size_t size) {
    size_t written = 0;
    while (written < size) {
      ssize_t n = ::write(fd, data + written, size - written);
      if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        throw std::runtime_error("socket write error");
      }
      written += n;
    }
  }

} // namespace core::driver