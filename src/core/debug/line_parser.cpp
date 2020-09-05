#include "line_parser.h"

namespace debug::core {

  line_parser::line_parser(std::string line)
      : line(line)
      , pos(0) {}

  void line_parser::reset(std::string l) {
    line = l;
    pos = 0;
  }

  char line_parser::peek() {
    return line[pos];
  }

  char line_parser::consume() {
    if (pos > line.size()) {
      return '\0';
    }
    return line[pos++];
  }

  void line_parser::skip(char c) {
    if (peek() == c) {
      pos++;
    }
  }

  void line_parser::skip(std::string c) {
    for (size_t i = 0; i < c.size(); i++) {
      skip(c[i]);
    }
  }

  std::string line_parser::consume(std::string::size_type n) {
    auto str = line.substr(pos, n);
    if (n == std::string::npos) {
      pos = line.size();
    } else {
      pos += n;
    }
    return str;
  }

  std::string line_parser::consume_until(char del) {
    return consume_until(std::string(1, del));
  }

  std::string line_parser::consume_until(std::string del_set) {
    size_t index = line.find_first_of(del_set, pos);
    if (index == std::string::npos) {
      return consume(index);
    }
    return consume(index - pos);
  }

} // namespace debug::core