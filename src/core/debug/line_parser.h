#pragma once

#include <string>

namespace debug::core {

  class line_parser {
  public:
    line_parser(std::string line);

    void reset(std::string l);

    char peek();
    char consume();

    void skip(char c);
    void skip(std::string cs);

    std::string consume(std::string::size_type n);
    std::string consume_until(char del);
    std::string consume_until(std::string del_set);

  protected:
    uint32_t pos;
    std::string line;
  };

} // namespace debug::core