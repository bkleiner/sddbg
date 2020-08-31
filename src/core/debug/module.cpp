#include "module.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>

#include "types.h"

namespace debug::core {
  module::module() {
    reset();
  }

  void module::set_name(std::string name) {
    module_name = name;
  }

  bool module::load_c_file(std::string path) {
    c_file_path = path;
    c_file_name = path.substr(path.rfind('/') + 1);
    return load_file(path, c_src);
  }

  bool module::load_asm_file(std::string path) {
    asm_file_path = path;
    asm_file_name = path.substr(path.rfind('/') + 1);
    return load_file(path, asm_src);
  }

  bool module::load_file(std::string path, std::vector<src_line> &srcvec) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
      std::cout << "ERROR: couldent open \"" << path << "\"" << std::endl;
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      srcvec.emplace_back(line);
    }

    return true;
  }

  uint32_t module::get_c_block(LINE_NUM line) {
    assert(line <= c_src.size());
    return c_src[line - 1].block;
  }

  uint32_t module::get_c_level(LINE_NUM line) {
    assert(line <= c_src.size());
    return c_src[line - 1].level;
  }

  src_line module::get_c_src_line(LINE_NUM line) {
    assert(line <= c_src.size());
    return c_src[line - 1];
  }

  src_line module::get_asm_src_line(LINE_NUM line) {
    assert(line <= asm_src.size());
    return asm_src[line - 1];
  }

  void module::reset() {
    c_file_name.clear();
    c_file_path.clear();
    c_src.clear();
    c_addr_map.clear();

    asm_file_name.clear();
    asm_file_path.clear();
    asm_src.clear();
    asm_addr_map.clear();
  }

  bool module::set_c_block_level(LINE_NUM line, uint32_t block, uint32_t level) {
    if (line > (c_src.size() - 1))
      c_src.resize(line);

    c_src[line - 1].block = block;
    c_src[line - 1].level = level;
    return true;
  }

  void module::set_c_addr(LINE_NUM line, ADDR addr) {
    if (line > (c_src.size() - 1) || c_src.size() == 0)
      c_src.resize(line);

    c_src[line - 1].addr = addr;
    c_addr_map[addr] = line;
  }

  void module::set_asm_addr(LINE_NUM line, ADDR addr) {
    if (line > (asm_src.size() - 1) || asm_src.size() == 0)
      asm_src.resize(line);

    int size = asm_src.size();
    asm_src[line - 1].addr = addr;
    asm_addr_map[addr] = line;
  }

  void module::dump() {
    for (auto it = c_src.begin(); it != c_src.end(); ++it) {
      ADDR a = it->addr;
      if (a == -1)
        printf("\t\t[%s]\n", it->src.c_str());
      else
        printf("0x%08x\t[%s]\n", it->addr, it->src.c_str());
    }

    for (auto it = asm_src.begin(); it != asm_src.end(); ++it) {
      ADDR a = it->addr;
      if (a == -1)
        printf("\t\t[%s]\n", it->src.c_str());
      else
        printf("0x%08x\t[%s]\n", it->addr, it->src.c_str());
    }
  }

  ADDR module::get_c_addr(LINE_NUM line) {
    return c_src[line - 1].addr;
  }

  ADDR module::get_asm_addr(LINE_NUM line) {
    return asm_src[line - 1].addr;
  }

  LINE_NUM module::get_c_line(ADDR addr) {
    const auto it = c_addr_map.find(addr);
    if (it != c_addr_map.end()) {
      return it->second;
    }
    return INVALID_LINE;
  }

  LINE_NUM module::get_asm_line(ADDR addr) {
    const auto it = asm_addr_map.find(addr);
    if (it != asm_addr_map.end()) {
      return it->second;
    }
    return INVALID_LINE;
  }

  void module_mgr::reset() {
    module_map.clear();
  }

  debug::core::module &module_mgr::add_module(std::string mod_name) {
    auto it = module_map.find(mod_name);
    if (it == module_map.end()) {
      module_map[mod_name].set_name(mod_name);
    }
    return module_map[mod_name];
  }

  bool module_mgr::del_module(std::string mod_name) {
    return module_map.erase(mod_name);
  }

  void dump_module(const std::pair<std::string, module> &pr) {
    module *m = (module *)&pr.second;
    std::cout << "module: "
              << m->get_name() << ", "
              << m->get_c_num_lines() << " c lines, "
              << m->get_asm_num_lines() << " asm lines"
              << std::endl;
  }

  void module_mgr::dump() {
    for (auto &entry : module_map) {
      dump_module(entry);
    }
  }

  bool module_mgr::get_asm_addr(ADDR addr, std::string &module, LINE_NUM &line) {
    for (auto it = module_map.begin(); it != module_map.end(); ++it) {
      line = it->second.get_asm_line(addr);
      if (line != INVALID_LINE) {
        module = it->second.get_name();
        return true;
      }
    }
    return false;
  }

  bool module_mgr::get_c_addr(ADDR addr, std::string &module, LINE_NUM &line) {
    for (auto it = module_map.begin(); it != module_map.end(); ++it) {
      line = it->second.get_c_line(addr);
      if (line != INVALID_LINE) {
        module = it->second.get_name();
        return true;
      }
    }
    return false;
  }
} // namespace debug::core