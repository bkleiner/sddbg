#pragma once

#include <map>
#include <string>
#include <vector>

#include "types.h"

namespace debug::core {
  struct src_line {
    src_line(std::string src = "")
        : addr(INVALID_ADDR)
        , block(0)
        , level(0)
        , src(src) {
    }

    ADDR addr;
    uint32_t block, level; // scope information
    std::string src;       // actual source line
  };

  class module {
  public:
    module();

    void reset();
    void dump();

    void set_name(std::string name);
    bool load_c_file(std::string path);
    bool load_asm_file(std::string path);

    bool set_c_block_level(LINE_NUM line, uint32_t block, uint32_t level);
    bool set_asm_block_level(LINE_NUM line, uint32_t block, uint32_t level);
    void set_c_addr(LINE_NUM line, ADDR addr);
    void set_asm_addr(LINE_NUM line, ADDR addr);

    ADDR get_c_addr(LINE_NUM line);
    ADDR get_asm_addr(LINE_NUM line);

    LINE_NUM get_c_line(ADDR addr);
    LINE_NUM get_asm_line(ADDR addr);

    uint32_t get_c_block(uint32_t line);
    uint32_t get_c_level(uint32_t line);

    src_line get_c_src_line(uint32_t line);
    src_line get_asm_src_line(uint32_t line);

    const std::string &get_name() { return module_name; }

    const std::string &get_c_file_path() { return c_file_path; }
    const std::string &get_c_file_name() { return c_file_name; }

    const std::string &get_asm_file_path() { return asm_file_path; }
    const std::string &get_asm_file_name() { return asm_file_name; }

    uint32_t get_c_num_lines() { return c_src.size(); }
    uint32_t get_asm_num_lines() { return asm_src.size(); }

    std::string get_asm_src(LINE_NUM line) { return asm_src[line - 1].src; }

  protected:
    typedef std::vector<src_line> SrcVec;
    typedef std::map<ADDR, LINE_NUM> AddrMap;

    std::string module_name;
    std::string c_file_name;
    std::string c_file_path;
    SrcVec c_src;
    AddrMap c_addr_map;

    std::string asm_file_name;
    std::string asm_file_path;
    SrcVec asm_src;
    AddrMap asm_addr_map;

    bool load_file(std::string path, SrcVec &srcvec);
  };

  class module_mgr {
  public:
    void reset();

    debug::core::module &module(std::string mod_name) { return add_module(mod_name); } // fixme need a variant of this that won't create new entries as this quick hack does.
    debug::core::module &add_module(std::string mod_name);
    bool del_module(std::string mod_name);

    void dump();

    bool get_asm_addr(ADDR addr, std::string &module, LINE_NUM &line);
    bool get_c_addr(ADDR addr, std::string &module, LINE_NUM &line);

  protected:
    std::map<std::string, debug::core::module> module_map;
  };

} // namespace debug::core
