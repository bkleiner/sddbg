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
#include "dbgsession.h"
#include "types.h"
#include <iostream>
#include <list>
#include <stdint.h>
#include <string>
#include <vector>

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
  enum ADDR_SPACE {
    AS_XSTACK,      ///< External stack
    AS_ISTACK,      ///< Internal stack
    AS_CODE,        ///< Code memory
    AS_CODE_STATIC, ///< Code memory, static segment
    AS_IRAM_LOW,    ///< Internal RAM (lower 128 bytes)
    AS_EXT_RAM,     ///< External data RAM
    AS_INT_RAM,     ///< Internal data RAM
    AS_BIT,         ///< Bit addressable area
    AS_SFR,         ///< SFR space
    AS_SBIT,        ///< SBIT space
    AS_REGISTER,    ///< Register space
    AS_UNDEF        ///< Used for function records, or any undefined space code
  };
  static const char addr_space_map[];

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

  void set_addr_space(char c);
  void set_addr(uint32_t addr);
  void set_end_addr(uint32_t addr);

  void setLength(int len) {
    m_length = len;
    m_end_addr = m_start_addr + m_length;
  }

  void add_reg(std::string reg) { m_regs.push_back(reg); }

  // function symbol specific values
  void setIsFunction(bool bfunc = true) { m_bFunction = bfunc; }
  void set_interrupt(bool intr = true) { m_is_int = intr; }
  int set_interrupt_num(int i) {
    int r = m_int_num;
    m_int_num = i;
    return r;
  }
  int set_reg_bank(int bank) {
    int r = m_reg_bank;
    m_reg_bank = bank;
    return r;
  }
  void setType(std::string type_name) {
    m_type_name = type_name;
  }
  void addParam(std::string param_type) { m_params.push_back(param_type); }
  void setReturn(std::string return_type) { m_return_type = return_type; }

  /** Adds a new dimention to the symbol if its an array.
		\param size Size of the new dimention.
	*/
  void AddArrayDim(uint16_t size) { m_array_dim.push_back(size); }

  uint32_t addr() { return m_start_addr; }
  uint32_t end_addr() { return m_end_addr; }

  // function symbol specific values
  bool isFunction() { return m_bFunction; }
  bool is_int_handler() { return m_is_int; }
  int interrupt_num() { return m_int_num; }
  int reg_bank() { return m_reg_bank; }
  std::string type() { return m_type_name; }
  FLAT_ADDR flat_start_addr();

  // type information, especially useful for structures.
  //set_type( std::string type );
  //std::string type();
  // how should the builtin types be handled?  special names or what.
  // this depends on how the type database is implemented.
  // maybe the type should be a pointer to the item in the type database.

  void dump();

  std::string sprint(char format);

  void print(char format);
  void print(char format, std::string expr);

protected:
  DbgSession *mSession;

  symbol_scope _scope;
  symbol_identifier _ident;

  ADDR_SPACE m_addr_space;
  uint32_t m_start_addr;
  uint32_t m_end_addr;

  std::string c_file;
  std::string asm_file;

  bool on_stack;
  int32_t stack_offset;

  int m_length;

  std::list<std::string> m_regs;

  std::list<std::string> m_params; // parameters for function symbols
  std::string m_return_type;       // return type for functions
  bool m_bFunction;
  std::string m_type_name;

  std::vector<uint16_t> m_array_dim;

  bool m_is_int;
  int m_int_num;
  int m_reg_bank;

  void print_array(char format, int dim_num, FLAT_ADDR &addr, SymType *type);
};

#endif
