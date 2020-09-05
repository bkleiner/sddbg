#pragma once

#include <deque>
#include <list>
#include <string>

namespace debug {
  class ParseCmd {
  public:
    typedef std::list<ParseCmd *> List;
    typedef std::deque<std::string> Args;

    virtual bool parse(std::string cmd) = 0;

    Args tokenize(const std::string &str, const std::string &delimiters = " ");
    bool match(const std::string &token, const std::string &mask);
    std::string join(Args args, const std::string &delimiter = " ");

    const std::string &get_name() const {
      return name;
    }

  protected:
    std::string name;
  };

  class CmdShowSetInfoHelp : public ParseCmd {
  public:
    enum Mode {
      DIRECT,
      SET,
      SHOW,
      INFO,
      HELP
    };

    virtual bool parse(std::string cmd);

  protected:
    virtual int compare_name(std::string s);

    virtual bool help(ParseCmd::Args args) { return false; }
    virtual bool set(ParseCmd::Args args) { return false; }
    virtual bool show(ParseCmd::Args args) { return false; }
    virtual bool info(ParseCmd::Args args) { return false; }
    virtual bool direct(ParseCmd::Args args) { return false; }
    virtual bool directnoarg() { return false; }
  };

} // namespace debug