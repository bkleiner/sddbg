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
#ifndef SYMBOL_H
#define SYMBOL_H

#include <iostream>
#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#include "dbgsession.h"
#include "memremap.h"
#include "types.h"

class SymType;

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

/**	base class for all types of symbols

	@author Ricky White <ricky@localhost.localdomain>
*/
class Symbol {
public:
  enum symbol_type {
    VARIABLE = (1 << 1),
    ARRAY = (1 << 2),
    STRUCT = (1 << 3),
    FUNCTION = (1 << 4),
    INTERRUPT = (1 << 5),
  };

  Symbol(DbgSession *session, symbol_scope _scope, symbol_identifier _ident);
  ~Symbol();

  const symbol_scope &get_scope() {
    return _scope;
  }

  const symbol_identifier &get_ident() {
    return _ident;
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

  void set_asm_file(std::string file) {
    asm_file = file;
  }
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

  void set_length(int len) {
    m_length = len;
    _end_addr = {_start_addr.space, _start_addr.addr + m_length};
  }

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

  void print(char format);
  void print(char format, std::string expr);

protected:
  DbgSession *mSession;

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

  void print_array(char format, int dim_num, FLAT_ADDR &addr, SymType *type);
};

#endif
