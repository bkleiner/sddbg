/***************************************************************************
 *   Copyright (C) 2006 by Ricky White   *
 *   ricky@localhost.localdomain   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "targets51.h"

#include <arpa/inet.h>
#include <cstring>
#include <errno.h> // Error number definitions
#include <fcntl.h> // File control definitions
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h> // POSIX terminal control definitions
#include <unistd.h>

#include <fmt/format.h>

#include "types.h"

TargetS51::TargetS51()
    : Target()
    , bConnected(false) {
  sock = -1;
  simPid = -1;
}

TargetS51::~TargetS51() {
}

bool TargetS51::connect() {
  bRunning = false;

  if (bConnected) {
    return false;
  }

  int retry = 0;

try_connect:
  sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr("127.0.0.1");
  sin.sin_port = htons(9756);

  // connect to the simulator
  if (::connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    // if failed then wait 1 second & try again
    // do this for 10 secs only
    if (retry < 10) {
      if (!retry) {
        /* fork and start the simulator as a subprocess */
        if (simPid = fork()) {
          printf("simi: simulator pid %d\n", (int)simPid);
        } else {
          // we are in the child process : start the simulator
          signal(SIGHUP, SIG_IGN);
          signal(SIGINT, SIG_IGN);
          signal(SIGABRT, SIG_IGN);
          signal(SIGCHLD, SIG_IGN);

          char *argv[] = {"s51", "-P", "-Z9756", NULL};
          if (execvp("s51", argv) < 0) {
            perror("cannot exec simulator");
            exit(1);
          }
        }
      }
      retry++;
      sleep(1);
      goto try_connect;
    }
    perror("connect failed :");
    exit(1);
  }

  int flag = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

  fmt::print("Simulator started, waiting for prompt\n");
  bConnected = true;

  fmt::print("Waiting for sim.\n");
  recvSim(250);
  fmt::print("Ready.\n");
  return true;
}

bool TargetS51::disconnect() {
  if (!bConnected) {
    return true;
  }

  bConnected = false;
  sendSim("quit");
  recvSim(2000);

  shutdown(sock, 2);
  close(sock);
  sock = -1;

  if (simPid > 0) {
    kill(simPid, SIGKILL);
    sleep(1);
  }

  return true;
}

bool TargetS51::is_connected() {
  return bConnected;
}

std::string TargetS51::port() {
  return "local";
}

bool TargetS51::set_port(std::string port) {
  return false; // no port set for simulator
}

std::string TargetS51::target_name() {
  return "S51";
}

std::string TargetS51::target_descr() {
  return "S51 8051 Simulator";
}

std::string TargetS51::device() {
  return "8052";
}

std::string TargetS51::sendSim(std::string cmd) {
  if (!bConnected)
    return "";

  cmd += "\r\n";

  recvSim(250);

  size_t written = 0;
  while (written < cmd.size()) {
    ssize_t n = write(sock, cmd.c_str(), cmd.size() - written);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      throw std::runtime_error("socket write error");
    }
    written += n;
  }

  std::string line = recvSimLine(500);
  if (line != cmd) {
    fmt::print("s51 command verify failed!");
  }
  return line;
}

std::string TargetS51::recvSim(int timeout_ms) {
  std::string resp;
  if (!bConnected) {
    return resp;
  }

  // Initialize the input set
  fd_set input;
  FD_ZERO(&input);
  FD_SET(sock, &input);
  fcntl(sock, F_SETFL, 0); // block if not enough characters available

  // Initialize the timeout structure
  struct timeval timeout;
  timeout.tv_sec = 0; // n seconds timeout
  timeout.tv_usec = timeout_ms * 1000 / 4;

  bool in_escape_sequence = false;

  while (1) {
    // Do the select
    const int n = select(sock + 1, &input, NULL, NULL, &timeout);
    if (n < 0) {
      throw std::runtime_error("select failed");
    }
    if (n == 0) {
      // std::cout << "recvSim timeout" << std::endl;
      break;
    }

    uint8_t ch = 0;

    const ssize_t r = read(sock, &ch, 1);
    if (r < 0) {
      throw std::runtime_error(strerror(errno));
    }
    if (r == 0) {
      // std::cout << "recvSim timeout" << std::endl;
      break;
    }

    if (in_escape_sequence) {
      if (ch != 0x5B && ch >= 0x40 && ch <= 0x7E)
        in_escape_sequence = false;
      continue;
    }
    if (ch == 0x1B) { // ESC character
      in_escape_sequence = true;
      continue;
    }

    resp += ch;
  }

  return resp;
}

/** Reads from simulator until line end or timeout.
*/
std::string TargetS51::recvSimLine(int timeout_ms) {
  std::string resp;
  if (!bConnected) {
    return resp;
  }

  // Initialize the input set
  fd_set input;
  FD_ZERO(&input);
  FD_SET(sock, &input);
  fcntl(sock, F_SETFL, 0); // block if not enough characters available

  // Initialize the timeout structure
  struct timeval timeout;
  timeout.tv_sec = 0; // n seconds timeout
  timeout.tv_usec = timeout_ms * 1000;

  bool in_escape_sequence = false;

  while (1) {
    // Do the select
    const int n = select(sock + 1, &input, NULL, NULL, &timeout);
    if (n < 0) {
      throw std::runtime_error("select failed");
    }
    if (n == 0) {
      // std::cout << "recvSimLine timeout" << std::endl;
      break;
    }

    char ch = 0;

    const ssize_t r = read(sock, &ch, 1);
    if (r < 0) {
      throw std::runtime_error("read failed");
    }
    if (r == 0) {
      // std::cout << "recvSimLine timeout" << std::endl;
      continue;
    }

    if (in_escape_sequence) {
      if (ch != 0x5B && ch >= 0x40 && ch <= 0x7E)
        in_escape_sequence = false;
      continue;
    }
    if (ch == 0x1B) { // ESC character
      in_escape_sequence = true;
      continue;
    }

    resp += ch;

    if (ch == '\n') {
      break;
    }
  }

  return resp;
}

///////////////////////////////////////////////////////////////////////////////
// Device control
///////////////////////////////////////////////////////////////////////////////

void TargetS51::reset() {
  sendSim("reset");
  recvSim(250);
  bRunning = false;
}

/** step over one assembly instruction
	\returns PC
*/
uint16_t TargetS51::step() {
  sendSim("step");
  recvSim(100);
  bRunning = false;
  return read_PC();
}

bool TargetS51::add_breakpoint(uint16_t addr) {
  sendSim(fmt::format("break 0x{:x}", addr));
  return true;
}

bool TargetS51::del_breakpoint(uint16_t addr) {
  sendSim(fmt::format("clear 0x{:x}", addr));
  std::string r = recvSim(250);
  if (r.find("No breakpoint at") == 0)
    return false;

  if (r.find("Breakpoint") == 0)
    return true;

  return true;
}

void TargetS51::clear_all_breakpoints() {
  // for the simulator we need to clear all at the simulator level
  // in case we have connected to an already sued simulator
  // any other breakpoints will have been cleared by calling the BreakpointMgr
  sendSim("info breakpoints");
  std::string s = recvSim(100);

  // parse the table deleting as we go
  std::string line;
  int pos = 0, epos;
  while (1) {
    epos = s.find('\n', pos);
    if (pos >= epos)
      return;
    line = s.substr(pos, epos - pos);
    pos = epos + 1;

    // skip header
    if (line[0] == 'N') {
      continue;
    }

    auto bpid = std::stoi(line.substr(0, line.find(' ')));
    sendSim(fmt::format("delete {}", bpid));
    std::cout << recvSim(100) << std::endl;
  }
}

void TargetS51::run_to_bp(int ignore_cnt) {
  //	recvSim(100);
  //	Simulation started, PC=0x00006f
  //			Stop at 0x000078: (104) Breakpoint
  //					0x00 00 00 00 00 00 00 00 00 ........
  //					000000 00 .  ACC= 0x6d 109 m  B= 0x00   DPTR= 0x0000 @DPTR= //0x00   0 .
  //					000000 00 .  PSW= 0x01 CY=0 AC=0 OV=0 P=1
  //					F? 0x0078 74 04    MOV   A,#04
  //					F 0x000078
  // fixme: need to know when sim has actually stopped.
  for (int i = 0; i <= ignore_cnt; i++) {
    sendSim("go");
    // wait for Stop
    std::string r;
    while (1) {
      r = recvSim(100);
      if (r.empty())
        continue;

      std::cout << "r=" << r << std::endl;
      if (r.find("Stop") != -1)
        break;
    }
  }
}

bool TargetS51::is_running() {
  return bRunning;
}

void TargetS51::stop() {
  /// @FIXME problem:S51 aborts when it sees the CTRL-C that newcdb uses to get here.  this is why we see Exit 255 then its all over.
  Target::stop();
  sendSim("stop");
  recvSim(100);
  bRunning = false;
}

/** Start simulator running then return
*/
void TargetS51::go() {
  sendSim("go");
  std::string msg = recvSimLine(100);
  //"Simulation started"
  bRunning = true;
}

/** Poll to see if the simulator has stopped
*/
bool TargetS51::poll_for_halt() {
  // FIXME maybe we should only look for this when we know the simulator is
  // running
  if (bRunning) {
    std::string s = recvSim(100);
    return s.length() > 0;
  } else
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Memory reads
///////////////////////////////////////////////////////////////////////////////

void TargetS51::read_data(uint8_t addr, uint8_t len, unsigned char *buf) {
  sendSim(fmt::format("di 0x{:02x} 0x{:02x}", addr, (addr + len - 1)));
  std::string s = recvSim(500);
  parse_mem_dump(s, buf, len);
}

/** @OBSOLETE
*/
void TargetS51::read_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
  sendSim(fmt::format("ds 0x{:02x} 0x{:02x}", addr, (addr + len - 1)));
  parse_mem_dump(recvSim(250), buf, len);
}

void TargetS51::read_sfr(uint8_t addr, uint8_t page,
                         uint8_t len, unsigned char *buf) {
  /// @TODO select page register here???

  read_sfr(addr, len, buf);
}

void TargetS51::read_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
  sendSim(fmt::format("dx 0x{:04x} 0x{:04x}", addr, (addr + len - 1)));
  parse_mem_dump(recvSim(250), buf, len);
}

void TargetS51::read_code(uint32_t addr, int len, unsigned char *buf) {
  sendSim(fmt::format("dch 0x{:04x} 0x{:04x}", addr, (addr + len - 1)));
  parse_mem_dump(recvSim(250), buf, len);
}

uint16_t TargetS51::read_PC() {
  if (!bConnected)
    return 0;
  std::string r = recvSim(250); // to flush any remaining data
  sendSim("pc");
  r = recvSim(250);
  //	std::cout << "["<<r<<"]";
  int pos = r.find("0x", 0);
  int npos = r.find(' ', pos);
  return strtoul(r.substr(pos, npos - pos).c_str(), 0, 16);
}

///////////////////////////////////////////////////////////////////////////////
// Memory writes
///////////////////////////////////////////////////////////////////////////////

void TargetS51::write_data(uint8_t addr, uint8_t len, unsigned char *buf) {
  write_mem("iram", addr, len, buf);
}

/** @OBSOLETE
*/
void TargetS51::write_sfr(uint8_t addr, uint8_t len, unsigned char *buf) {
  write_mem("sfr", addr, len, buf);
}

void TargetS51::write_sfr(uint8_t addr, uint8_t page,
                          uint8_t len, unsigned char *buf) {
  Target::write_sfr(addr, page, len, buf);
  /// @TODO Select page here...

  write_mem("sfr", addr, len, buf);
}

void TargetS51::write_xdata(uint16_t addr, uint16_t len, unsigned char *buf) {
  write_mem("xram", addr, len, buf);
}

void TargetS51::write_code(uint16_t addr, int len, unsigned char *buf) {
  write_mem("rom", addr, len, buf);
}

void TargetS51::write_PC(uint16_t addr) {
  sendSim(fmt::format("pc 0x{:04x}", addr));
  recvSim(250);
}

void print_buf(unsigned char *buf, int len) {
  const int PerLine = 16;
  int i, addr;

  for (addr = 0; addr < len; addr += PerLine) {
    printf("%04x\t", addr);
    // print each hex byte
    for (i = 0; i < PerLine; i++)
      printf("%02x ", buf[addr + i]);
    printf("\t");
    for (i = 0; i < PerLine; i++)
      putchar((buf[addr + i] >= '0' && buf[addr + i] <= 'z') ? buf[addr + i] : '.');
    putchar('\n');
  }
}

bool TargetS51::command(std::string cmd) {
  unsigned char buf[256];
  if (cmd.compare("test") == 0) {
    read_data(0, 0x80, buf);
    print_buf(buf, 0x80);
  } else {
    sendSim(cmd);
    std::cout << recvSim(250) << std::endl;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Helper routines
// simplify communications with the simulator
///////////////////////////////////////////////////////////////////////////////

/** Take a memory dump from the dimulator and parse it into the supplied buffer.
	Format: 
		0x08 00 bc d4 b6 3d 1c 3e 22 ....=.>"
		reads with less bytes will return a partial row,
		those with more will return multiple rows
	note the address can use more characters in the event of 16 bit so we must parse it
*/
void TargetS51::parse_mem_dump(std::string dump, unsigned char *buf, int len) {
  int pos = 0, epos = 0;
  std::string line;
  int total_rows = (len % 8 == 0) ? len / 8 : len / 8 + 1;
  int ofs = 0;
  // split returned std::string up into lines and process independantly
  for (int row = 0; row < total_rows; row++) {
    epos = dump.find('\n', pos);
    line = dump.substr(pos, epos - pos);
    // parse the line
    ofs = line.find(' ', 0) + 1; // skip over address
    int max_j = (len - (row * 8)) > 8 ? 8 : (len - (row * 8));
    for (int j = 0; j < max_j; j++) {
      *buf++ = strtol(line.substr(ofs, 2).c_str(), 0, 16);
      ofs += 3;
    }
    pos = epos + 1;
  }
}

/** write to a specific memory area on the simulator.
	the area provided must match a memory area recognised by the simulator
	eg xram, rom, iram, sfr
*/
void TargetS51::write_mem(std::string area, uint16_t addr, uint16_t len, unsigned char *buf) {
  const int MAX_CHUNK = 16;
  for (ADDR offset = 0; offset < len; offset += MAX_CHUNK) {
    std::string cmd = fmt::format("set mem {} 0x{:04x}", area, addr + offset);
    for (int i = 0; (i < MAX_CHUNK && (offset + i) < len); i++) {
      cmd += fmt::format(" 0x{:02x}", buf[offset + i]);
    }

    sendSim(cmd);
  }
}
