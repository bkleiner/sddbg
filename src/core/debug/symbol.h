#pragma once

#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#include "dbg_session.h"
#include "mem_remap.h"
#include "types.h"

namespace debug::core {

  class sym_type;

  struct symbol_identifier {
    symbol_identifier(
        std::string name,
        int32_t level,
        int32_t block)
        : name(name)
        , level(level)
        , block(block) {}

    std::string name;
    int32_t level;
    int32_t block;

    bool operator==(const symbol_identifier &rhs) const {
      return name == rhs.name &&
             level == rhs.level &&
             block == rhs.block;
    }

    std::size_t hash() {
      std::size_t h1 = std::hash<std::string>{}(name);
      std::size_t h2 = std::hash<int32_t>{}(level);
      std::size_t h3 = std::hash<int32_t>{}(block);
      return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
  };

  struct symbol_scope {
    static constexpr const char *const names[] = {
        "Global",
        "File",
        "Local",
    };

    enum types {
      INVALID,
      GLOBAL,
      FILE,
      LOCAL
    };

    symbol_scope()
        : typ(INVALID) {}

    symbol_scope(
        types typ,
        std::string file = "",
        std::string function = "")
        : typ(typ)
        , file(file)
        , function(function) {}

    types typ;
    std::string file;
    std::string function;

    bool operator==(const symbol_scope &rhs) const {
      if (typ == rhs.typ) {
        return file == rhs.file &&
               function == rhs.function;
      }
      return false;
    }
  };

  class symbol {
  public:
    enum symbol_type {
      VARIABLE = (1 << 1),
      ARRAY = (1 << 2),
      STRUCT = (1 << 3),
      FUNCTION = (1 << 4),
      INTERRUPT = (1 << 5),
    };

    symbol(dbg_session *session, symbol_scope _scope, symbol_identifier _ident);
    ~symbol();

    const symbol_scope &get_scope() {
      return _scope;
    }

    const symbol_identifier &get_ident() {
      return _ident;
    }

    std::size_t hash() {
      return _ident.hash();
    }

    uint32_t short_hash() {
      auto h = hash();
      return h & 0xFFFFFFFF + ((h >> 32) & 0xFFFFFFFF);
    }

    symbol_scope::types scope() { return _scope.typ; }
    std::string file() { return _scope.file; }
    std::string function() { return _scope.function; }

    std::string name() { return _ident.name; }
    int level() { return _ident.level; }
    int block() { return _ident.block; }

    void set_type(symbol_type typ) {
      type |= typ;
    }
    bool is_type(symbol_type typ) {
      return type & typ;
    }

    std::string type_name() {
      return m_type_name;
    }
    void set_type_name(std::string type_name) {
      m_type_name = type_name;
    }

    std::string get_asm_file() { return asm_file; }
    void set_asm_file(std::string file) {
      asm_file = file;
    }
    std::string get_c_file() { return c_file; }
    void set_c_file(std::string file) {
      c_file = file;
    }

    void set_stack_offset(int32_t offset) {
      on_stack = true;
      stack_offset = offset;
    }

    target_addr addr() { return _start_addr; }
    void set_addr(target_addr addr);

    target_addr end_addr() { return _end_addr; }
    void set_end_addr(target_addr addr);

    void set_length(int len);

    void add_reg(std::string reg) { m_regs.push_back(reg); }

    uint32_t array_size() { return m_array_size[0]; }
    void add_array_size(uint32_t size) { m_array_size.push_back(size); }

    // function symbol specific values
    int interrupt_num() { return m_int_num; }
    int reg_bank() { return m_reg_bank; }
    void set_interrupt_num(int i) { m_int_num = i; }
    void set_reg_bank(int bank) { m_reg_bank = bank; }

    void dump();

    std::string sprint(char format);
    std::string sprint(char format, std::string expr);

    void print(char format);
    void print(char format, std::string expr);

  protected:
    dbg_session *session;

    uint32_t type;
    symbol_scope _scope;
    symbol_identifier _ident;

    target_addr _start_addr;
    target_addr _end_addr;

    std::string c_file;
    std::string asm_file;

    bool on_stack;
    int32_t stack_offset;

    int m_length;

    // type for variable
    // return type for function
    std::string m_type_name;

    std::list<std::string> m_regs;
    std::vector<uint32_t> m_array_size;

    int m_int_num;
    int m_reg_bank;

    void print_array(char format, int dim_num, target_addr addr, sym_type *type);
  };

} // namespace debug::core