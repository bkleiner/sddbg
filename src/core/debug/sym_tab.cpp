#include "sym_tab.h"

#include <filesystem>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "module.h"

namespace fs = std::filesystem;

namespace debug::core {

  sym_tab::sym_tab(dbg_session *session)
      : session(session) {
  }

  sym_tab::~sym_tab() {
  }

  void sym_tab::clear() {
    // @TODO clear all tables
    m_symlist.clear();
    file_map.clear();
    c_file_list.clear();
    asm_file_list.clear();
  }

  symbol *sym_tab::add_symbol(const symbol_scope &scope, const symbol_identifier &ident) {
    symbol *sym = get_symbol(scope, ident);
    if (sym != nullptr) {
      return sym;
    }
    m_symlist.emplace_back(session, scope, ident);
    return get_symbol(scope, ident);
  }

  symbol *sym_tab::get_symbol(const symbol_scope &scope, const symbol_identifier &ident) {
    for (auto &sym : m_symlist) {
      if (sym.get_scope() == scope && sym.get_ident() == ident) {
        return &sym;
      }
    }
    return nullptr;
  }

  std::vector<symbol *> sym_tab::get_symbols(context ctx) {
    std::vector<symbol *> result;

    for (auto &sym : m_symlist) {
      if (sym.is_type(symbol::FUNCTION)) {
        continue;
      }

      switch (sym.scope()) {
      case symbol_scope::GLOBAL:
        result.push_back(&sym);
        break;

      case symbol_scope::FILE:
        if (sym.file() == ctx.module)
          result.push_back(&sym);
        break;

      case symbol_scope::LOCAL:
        if ((sym.function() == ctx.module + "." + ctx.function)) {
          result.push_back(&sym);
        }
        break;

      default:
        break;
      }
    }

    return result;
  }

  symbol *sym_tab::get_symbol(const context &ctx, const std::string &name) {
    // there are more efficient ways of doing this.
    // this is just quick and dirty for now and should be cleaned up

    // search local scope
    symbol *ptr = nullptr;
    for (auto &sym : m_symlist) {
      if ((sym.name() == name) &&
          (sym.scope() == symbol_scope::LOCAL) &&
          (sym.function() == ctx.module + "." + ctx.function) &&
          (sym.block() <= ctx.block && sym.level() <= ctx.level) &&
          (ptr == nullptr || (sym.block() > ptr->block() && sym.level() > ptr->level()))) {
        ptr = &sym;
      }
    }
    if (ptr != nullptr) {
      return ptr;
    }

    // File scope
    for (auto &sym : m_symlist) {
      if ((sym.name() == name) &&
          (sym.scope() == symbol_scope::FILE) &&
          (sym.file() == ctx.module)) {
        return &sym;
      }
    }

    // Global scope
    for (auto &sym : m_symlist) {
      if ((sym.name() == name) &&
          (sym.scope() == symbol_scope::GLOBAL)) {
        return &sym;
      }
    }

    return nullptr;
  }

  symbol *sym_tab::get_symbol(const uint16_t hash) {
    for (auto &sym : m_symlist) {
      if (hash == sym.short_hash()) {
        return &sym;
      }
    }
    return nullptr;
  }

  void sym_tab::dump() {
    dump_symbols();
    dump_c_lines();
    dump_asm_lines();
    dump_functions();
  }

  void sym_tab::dump_symbols() {
    SYMLIST::iterator it;
    log::print("Name           Start addr  End addr    File       Scope   Function\tMem Area\tType\n");
    log::print("=================================================================================\n");
    for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
      it->dump();
    }
  }

  void sym_tab::dump_c_lines() {
    FILE_LIST::iterator it;
    log::print("\n\nC file lines\n");
    log::print("name\tline\tlevel\tblock\taddr\n");
    log::print("=================================================================================\n\n");
    for (it = c_file_list.begin(); it != c_file_list.end(); ++it) {
      log::printf("%s\t%i\t%i\t%i\t0x%08x\n",
                  file_name((*it).file_id).c_str(),
                  (*it).line_num,
                  (*it).level,
                  (*it).block,
                  (*it).addr);
    }
  }

  void sym_tab::dump_asm_lines() {
    FILE_LIST::iterator it;
    log::print("\n\nASM file lines\n");
    log::print("name\tline\taddr\n");
    log::print("=================================================================================\n\n");

    for (it = asm_file_list.begin(); it != asm_file_list.end(); ++it) {
      log::printf("%s\t%i\t0x%08x\n",
                  file_name((*it).file_id).c_str(),
                  (*it).line_num,
                  (*it).addr);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////
  // address from functions
  ///////////////////////////////////////////////////////////////////////////////
  /** Get the address that relates to a specific line number in a file
	\returns >=0 address, -1 = failure
*/
  int32_t sym_tab::get_addr(std::string file, int line_num) {
    int fid = file_id(file);
    if (fid == -1)
      return -1; // failure

    if (file.substr(file.length() - 2) == ".c") {

      for (auto &f : c_file_list) {
        if (f.file_id != fid || f.line_num != line_num)
          continue;
        return f.addr;
      }
      log::print(" Error: {} line number not found\n", file);
    }
    if (file.substr(file.length() - 4).compare(".asm") == 0 ||
        file.substr(file.length() - 4).compare(".a51") == 0) {

      for (auto &f : asm_file_list) {
        if (f.file_id != fid || f.line_num != line_num)
          continue;
        return f.addr;
      }
      log::print(" Error: {} line number not found\n", file);
    }

    return -1; // failure
  }

  /** Get the address of the start of a function in a file
	\returns >=0 address, -1 = failure
*/
  bool sym_tab::get_addr(std::string file, std::string function, int32_t &addr, int32_t &endaddr) {
    for (auto it = m_symlist.begin(); it != m_symlist.end(); ++it) {
      if (it->is_type(symbol::FUNCTION)) {
        log::print("checking file={} function={}\n", it->file(), it->name());
        if (it->name() == function) {
          addr = it->addr();
          endaddr = it->end_addr();
          return true;
        }
      }
    }
    return false;
  }

  /** Get the address of the start of a function (any file)
	\returns >=0 address, -1 = failure
*/
  int32_t sym_tab::get_addr(std::string function) {
    FILE_LIST::iterator it;
    for (it = c_file_list.begin(); it != c_file_list.end(); ++it)
      if ((*it).function == function)
        return (*it).addr;
    return -1; // failure
  }

  bool sym_tab::get_addr(std::string function, int32_t &addr, int32_t &endaddr) {
    SYMLIST::iterator it;
    for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
      if ((*it).is_type(symbol::FUNCTION)) {
        if ((*it).name() == function) {
          addr = (*it).addr();
          endaddr = (*it).end_addr();
          return true;
        }
      }
    }
    return false; // failure
  }

  bool sym_tab::find_c_file_line(ADDR addr, std::string &file, LINE_NUM &line_num) {
    FILE_LIST::iterator it;

    for (it = c_file_list.begin(); it != c_file_list.end(); ++it)
      if ((*it).addr == addr) {
        file = file_name((*it).file_id);
        line_num = (*it).line_num;
        return true;
      }
    file = "no match";
    line_num = LINE_NUM(-1);
    return false; // not found
  }

  bool sym_tab::find_asm_file_line(uint16_t addr, std::string &file, int &line_num) {
    FILE_LIST::iterator it;
    for (it = asm_file_list.begin(); it != asm_file_list.end(); ++it)
      if ((*it).addr == addr) {
        file = file_name((*it).file_id);
        line_num = (*it).line_num;
        return true;
      }
    file = "no match";
    line_num = -1;
    return false; // not found
  }

  bool sym_tab::add_c_file_entry(std::string filename, int line_num, int level, int block, uint16_t addr) {
    if (!fs::is_regular_file(filename)) {
      return false;
    }

    auto name = fs::path(filename).filename();
    module &m = session->modulemgr()->add_module(name.stem());

    int fid = file_id(name);
    if (fid == -1) {
      file_map.push_back(name);
      fid = file_id(name);

      m.load_c_file(filename);
    }

    // build and add the entry
    FILE_ENTRY ent;
    ent.file_id = fid;
    ent.line_num = line_num;
    ent.level = level;
    ent.block = block;
    ent.addr = addr;
    c_file_list.push_back(ent);

    m.set_c_addr(line_num, addr);
    m.set_c_block_level(line_num, block, level);
    return true;
  }

  bool sym_tab::add_asm_file_entry(std::string filename, int line_num, uint16_t addr) {
    if (!fs::is_regular_file(filename)) {
      return false;
    }

    auto name = fs::path(filename).filename();
    module &m = session->modulemgr()->add_module(name.stem());

    int fid = file_id(name);
    if (fid == -1) {
      file_map.push_back(name);
      fid = file_id(name);

      m.load_asm_file(filename);
    }

    // build and add the entry
    FILE_ENTRY ent;
    ent.file_id = fid;
    ent.line_num = line_num;
    ent.addr = addr;
    asm_file_list.push_back(ent);

    m.set_asm_addr(line_num, addr);
    return true;
  }

  int sym_tab::file_id(std::string filename) {
    for (size_t i = 0; i < file_map.size(); i++) {
      if (file_map[i] == filename)
        return i;
    }
    return -1; // Failure
  }

  std::string sym_tab::file_name(int id) {
    return file_map[id];
  }

  void sym_tab::dump_functions() {
    //	FUNC_LIST::iterator it;
    log::printf("File                  Function              start     end\n");
    log::print("=================================================================================\n");
    SYMLIST::iterator it;
    for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
      if ((*it).is_type(symbol::FUNCTION)) {
        log::printf("%-20s  %-20s  0x%08x  0x%08x\n",
                    (*it).file().c_str(),
                    (*it).name().c_str(),
                    (*it).addr(),
                    (*it).end_addr());
      }
    }
  }

  /** get the name of a function that the specified code address is within
	\param address to find out which function it is part of.
	\returns true on success, false on failure ( no function found)
*/
  bool sym_tab::get_c_function(ADDR addr, std::string &file, std::string &func) {
    for (auto &sym : m_symlist) {
      if (!sym.is_type(symbol::FUNCTION))
        continue;

      if (addr >= sym.addr() && addr <= sym.end_addr()) {
        func = sym.name();
        file = sym.get_c_file();
        return true;
      }
    }
    return false;
  }

  bool sym_tab::get_c_block_level(std::string file, LINE_NUM line, BLOCK &block, LEVEL &level) {
    const auto fid = file_id(file);
    for (auto &f : c_file_list) {
      if (f.file_id != fid || f.line_num != line)
        continue;

      level = f.level;
      block = f.block;
      return true;
    }
    return false;
  }

  /// @FIXME dosen't work flat vs normal address issue
  std::string sym_tab::get_symbol_name(FLAT_ADDR addr) {
    SYMLIST::iterator it;
    for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
      if ((*it).addr() == addr)
        return (*it).name();
    }
    return "NOT IMPLEMENTED";
  }
} // namespace debug::core