#pragma once

#include <string>

#include "dbg_session.h"
#include "line_parser.h"
#include "sym_tab.h"
#include "sym_type_tree.h"

namespace debug::core {

  class cdb_file : public line_parser {
  public:
    cdb_file(dbg_session *session);
    ~cdb_file();
    bool open(std::string filename, std::string src_dir = "");
    bool parse_record();

  protected:
    dbg_session *mSession;

    std::string base_dir;
    std::string src_dir;

    std::string cur_module;
    std::string cur_file;

    symbol_scope parse_scope();
    symbol_identifier parse_identifier();

    bool parse_linker();
    bool parse_type_chain_record(symbol *sym);

    bool parse_type();
    bool parse_type_member(sym_type_struct *t);
  };

} // namespace debug::core
