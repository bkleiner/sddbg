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
#include "symbol.h"

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include <fmt/format.h>

#include "contextmgr.h"
#include "memremap.h"
#include "symtypetree.h"

constexpr const char *const symbol_scope::names[];

Symbol::Symbol(DbgSession *session, symbol_scope _scope, symbol_identifier _ident)
    : mSession(session)
    , _scope(_scope)
    , _ident(_ident)
    , on_stack(false)
    , type(0)
    , m_length(-1) {}

Symbol::~Symbol() {
}

void Symbol::set_addr(target_addr addr) {
  _start_addr = addr;
  if (m_length != -1)
    _end_addr = _start_addr + ADDR(m_length - 1);
}

void Symbol::set_end_addr(target_addr addr) {
  _end_addr = addr;
  m_length = _end_addr.addr - _start_addr.addr + 1;
}

void Symbol::set_length(int len) {
  m_length = len;
  _end_addr = _start_addr + ADDR(m_length - 1);
}

void Symbol::dump() {
  std::string name = _ident.name;
  char buf[255];
  memset(buf, 0, sizeof(buf));
  for (int i = 0; i < m_array_size.size(); i++) {
    snprintf(buf, sizeof(buf), "[%i]", m_array_size[i]);
    name += buf;
  }
  printf("%-15s0x%08x  0x%08x  %-9s %-8s %-12s %c %-10s",
         name.c_str(),
         _start_addr,
         _start_addr,
         _scope.file.c_str(),
         symbol_scope::names[_scope.typ],
         _scope.function.c_str(),
         _start_addr.space_name(),
         m_type_name.c_str());
  std::list<std::string>::iterator it;
  if (!m_regs.empty()) {
    printf("Regs: ");
    for (it = m_regs.begin(); it != m_regs.end(); ++it) {
      printf("%s ", it->c_str());
    }
  }
  printf("\n");
}

std::string Symbol::sprint(char format) {
  SymType *type = mSession->symtree()->get_type(m_type_name, {});
  if (!type || is_type(ARRAY)) {
    return "";
  }

  // where do arrays get handles?  count in symbol...
  if (type->terminal()) {
    target_addr addr = _start_addr;
    if (_start_addr.space == target_addr::AS_REGISTER) {
      for (auto r : m_regs) {
        if (r[0] != 'r' && r[0] != 'R') {
          break;
        }
        auto i = std::stoi(r.substr(1));
        addr.addr = i;
        break;
      }
    }
    return type->pretty_print(format, addr);
  }

  // complex type
  return "";
}

/** Print the symbol with the specified indent
*/
void Symbol::print(char format) {
  std::cout << _ident.name << " = "
            << sprint(format)
            << std::endl;
}

/** Recursive function to print out an complete arrays contents.
*/
void Symbol::print_array(char format, int dim_num, target_addr addr, SymType *type) {
  //	std::cout << "print_array( "<<format<<", "<<dim_num<<", "<<hex<<addr<<" )"<<std::endl;
  //	std::cout << "m_array_size.size() = " << m_array_size.size() << std::endl;

  if (dim_num == (m_array_size.size() - 1)) {
    //		std::cout <<"elements("<<m_array_size[dim_num/*-1*/]<<")"<<std::endl;
    // deapest, print elements

    // special case default format with char array
    if (format == 0 && (type->name() == "char" || type->name() == "unsigned char")) {
      std::cout << "\"";
      for (int i = 0; i < m_array_size[dim_num /*-1*/]; i++) {
        std::cout << type->pretty_print('s', addr);
        addr = addr + type->size();
      }
      std::cout << "\"";
    } else {
      std::cout << "{";
      for (int i = 0; i < m_array_size[dim_num /*-1*/]; i++) {
        std::cout << (i > 0 ? "," : "") << type->pretty_print(format, addr);
        addr = addr + type->size();
      }
      std::cout << "}";
    }
  } else {
    std::cout << "{";
    for (int i = 0; i < m_array_size[dim_num]; i++) {
      print_array(format, dim_num + 1, addr, type);
    }
    std::cout << "}";
  }
}

/** Print the symbol,  The expression must start with the symnbol.
	expr is used to determine if a single element or the entire array is to be printed
*/
void Symbol::print(char format, std::string expr) {
  std::cout << "expr = '" << expr << "'" << std::endl;

  enum {
    STATE_START,
    STATE_ARRAY_SUBSCRIPT,
    STATE_MEMBER_NAME,
  } state = STATE_START;

  size_t offset = 0;
  std::string parser_expr = expr;
  std::vector<uint32_t> array_subscripts;
  std::vector<std::string> member_names;

  while (parser_expr.size()) {
    char c = parser_expr[offset];

    switch (state) {
    case STATE_START:
      if (c == '[') {
        parser_expr = parser_expr.substr(offset + 1);
        offset = 0;
        state = STATE_ARRAY_SUBSCRIPT;
      }
      if (c == '.') {
        parser_expr = parser_expr.substr(offset + 1);
        offset = 0;
        state = STATE_MEMBER_NAME;
      }
      if (offset == parser_expr.size()) {
        parser_expr = parser_expr.substr(offset);
        offset = 0;
      }
      break;

    case STATE_ARRAY_SUBSCRIPT:
      if (c == ']') {
        array_subscripts.push_back(std::stoul(parser_expr.substr(0, offset)));
        parser_expr = parser_expr.substr(offset + 1);
        offset = 0;
        state = STATE_START;
      }
      break;

    case STATE_MEMBER_NAME:
      if (offset == parser_expr.size() || c == '.') {
        member_names.push_back(parser_expr.substr(0, offset));
        parser_expr = parser_expr.substr(offset);
        offset = 0;
        state = STATE_START;
      }
      break;

    default:
      throw std::runtime_error("print invalid expr parser state\n");
    }
    offset++;
  }

  ContextMgr::Context context = mSession->contextmgr()->get_current();
  if (array_subscripts.size()) {
    SymType *type = mSession->symtree()->get_type(m_type_name, context);

    // @FIXME: this dosen't handle multiple dimensions
    const uint32_t index = array_subscripts[0];
    if (type->terminal()) {
      // calculate memory location
      target_addr addr = _start_addr + ADDR(index * type->size());
      std::cout << type->pretty_print(format, addr) << std::endl;
    }

    return;
  }

  if (member_names.size()) {
    SymTypeStruct *type = dynamic_cast<SymTypeStruct *>(mSession->symtree()->get_type(m_type_name, context));
    if (type == nullptr)
      return;

    // @FIXME: this dosen't handle multiple dimensions
    const std::string member_name = member_names[0];
    SymType *member_type = type->get_member_type(member_name);
    if (member_type != nullptr && member_type->terminal()) {
      // calculate memory location
      target_addr addr = _start_addr + type->get_member_offset(member_name);
      std::cout << member_type->pretty_print(format, addr) << std::endl;
    }
    return;
  }

  // if we get here and the symbol says it is not terminal then we must print all its children

  // Either a terminal or an array of terminals where we print all.
  // array count is part of symbol.
  SymType *type = mSession->symtree()->get_type(m_type_name, context);
  if (!type) {
    return;
  }

  if (type->terminal()) {
    if (m_array_size.size() == 0) {
      print(format);
    } else {
      print_array(format, 0, _start_addr, type);
      std::cout << std::endl;
    }
  } else {
    auto complex = dynamic_cast<SymTypeStruct *>(type);
    for (auto &m : complex->get_members()) {
      SymType *member_type = complex->get_member_type(m.member_name);
      fmt::print("{} = {}\n", m.member_name, member_type->pretty_print(format, _start_addr + m.offset));
    }
  }
}
