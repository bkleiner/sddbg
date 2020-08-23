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

const char *Symbol::scope_name[] = {"Global", "File", "Local", 0};
const char Symbol::addr_space_map[] =
    {'A', 'B', 'C', 'D', 'E', 'F', 'H', 'I', 'J', 'R', 'Z'};

Symbol::Symbol(DbgSession *session)
    : mSession(session) {
  setAddrSpace('Z'); // undefined
  m_name = "";
  m_start_addr = 0xffffffff;
  m_end_addr = -1;
  m_length = -1;
  m_bFunction = false;
}

Symbol::~Symbol() {
}

void Symbol::setScope(std::string scope) {
  for (int i = 0; i < SCOPE_CNT; i++) {

    if (scope.compare(scope_name[i]))
      setScope(SCOPE(i));
  }
}

void Symbol::setScope(char scope) {
  switch (scope) {
  case 'G':
    setScope(SCOPE_GLOBAL);
    break;
  case 'F':
    setScope(SCOPE_FILE);
    break;
  case 'L':
    setScope(SCOPE_LOCAL);
    break;
  default:
    std::cout << "ERROR invalid scope" << std::endl;
  }
}

void Symbol::setAddrSpace(char c) {
  for (int i = 0; i < sizeof(addr_space_map); i++) {
    if (addr_space_map[i] == c) {
      m_addr_space = ADDR_SPACE(i);
      return;
    }
  }
  setAddrSpace('Z'); // Invalid
}

void Symbol::setAddr(uint32_t addr) {
  m_start_addr = addr;
  if (m_length != -1)
    m_end_addr = m_start_addr + m_length - 1;
  //	m_start_addr = addr;
}

void Symbol::setEndAddr(uint32_t addr) {
  m_end_addr = addr;
  m_length = m_end_addr - m_start_addr + 1;
}

/** Determin the flat address of the symbol and return it.
	This runction also handles the many to 1 relation ship of areas.
*/
FLAT_ADDR Symbol::flat_start_addr() {
  printf("addr apace = %c\n", addr_space_map[m_addr_space]);
  printf("type = %i\n", m_addr_space);
  char as;
  switch (m_addr_space) {
  case AS_XSTACK:;
    break; ///< External stack
  case AS_ISTACK:;
    break; ///< Internal stack
  case AS_CODE:;
    break; ///< Code memory
  case AS_CODE_STATIC:;
    break; ///< Code memory, static segment
  case AS_IRAM_LOW:
    as = 'd';
    break; ///< Internal RAM (lower 128 bytes)
  case AS_EXT_RAM:;
    break; ///< External data RAM
  case AS_INT_RAM:
    as = 'd';
    break; ///< Internal data RAM
  case AS_BIT:;
    break; ///< Bit addressable area
  case AS_SFR:;
    break; ///< SFR space
  case AS_SBIT:;
    break; ///< SBIT space
  case AS_REGISTER:;
    break;
  }
  return MemRemap::flat(m_start_addr, as);

  FLAT_ADDR a = MemRemap::flat(m_start_addr, addr_space_map[m_addr_space]);
  printf("Flat addr = 0x%08x, m_start_addr=0x%08x\n", a, m_start_addr);
  return a;
}

void Symbol::dump() {
  std::string name = m_name;
  char buf[255];
  memset(buf, 0, sizeof(buf));
  for (int i = 0; i < m_array_dim.size(); i++) {
    snprintf(buf, sizeof(buf), "[%i]", m_array_dim[i]);
    name += buf;
  }
  printf("%-15s0x%08x  0x%08x  %-9s %-8s %-12s %c %-10s",
         name.c_str(),
         m_start_addr,
         m_end_addr,
         m_file.c_str(),
         scope_name[m_scope],
         m_function.c_str(),
         addr_space_map[m_addr_space],
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

/** Print the symbol with the specified indent
*/
void Symbol::print(char format) {
  SymType *type;
  //printf("*** name = %s\n",m_name.c_str());
  std::cout << m_name << " = ";
  //	if( sym_type_tree.get_type( m_type_name, &type ) )
  //sym_type_tree.get_type( m_type_name, file, block, &type ) )
  // FIXME a context woule be usefule here...

  ContextMgr::Context context;
  type = mSession->symtree()->get_type(m_type_name, context);
  if (type) {
    // where do arrays get handles?  count in symbol...
    if (type->terminal()) {
      // print out data now...
      std::cout << "type = " << type->name() << std::endl;
      // @TODO pass flat memory address to the type so it can reterieve the data and print it out.

      // @FIXME: need to use the flat addr from remap rather than just the start without an area.
      uint32_t flat_addr = MemRemap::flat(m_start_addr, addr_space_map[m_addr_space]); // map needs to map to lower case in some cases...!!! maybe fix in memremap

      flat_addr = MemRemap::flat(m_start_addr, 'd'); // @FIXME remove this hack
                                                     //			std::cout << type->pretty_print( m_name, indent, flat_addr );
      std::cout << type->pretty_print(format, m_name, flat_addr);
    } else {
      // its more comples like a structure or typedef
    }
    std::cout << std::endl;
  }
  //	assert(1==0);	// oops unknown type
}

#include <boost/regex.hpp>
#include <iostream>
/** scan for a [ or a . indicating the end of the current element
*/
size_t find_element_end(std::string expr) {
  //	if( expr.find_first_of('[', size_type pos = 0)

  return -1;
}

/** Recursive function to print out an complete arrays contents.
*/
void Symbol::print_array(char format, int dim_num, FLAT_ADDR &addr, SymType *type) {
  //	std::cout << "print_array( "<<format<<", "<<dim_num<<", "<<hex<<addr<<" )"<<std::endl;
  //	std::cout << "m_array_dim.size() = " << m_array_dim.size() << std::endl;

  if (dim_num == (m_array_dim.size() - 1)) {
    //		std::cout <<"elements("<<m_array_dim[dim_num/*-1*/]<<")"<<std::endl;
    // deapest, print elements

    // special case default format with char array
    if (format == 0 && (type->name() == "char" || type->name() == "unsigned char")) {
      std::cout << "\"";
      for (int i = 0; i < m_array_dim[dim_num /*-1*/]; i++) {
        std::cout << type->pretty_print('s', "", addr);
        addr += type->size();
      }
      std::cout << "\"";
    } else {
      std::cout << "{";
      for (int i = 0; i < m_array_dim[dim_num /*-1*/]; i++) {
        std::cout << (i > 0 ? "," : "") << type->pretty_print(format, "", addr);
        addr += type->size();
      }
      std::cout << "}";
    }
  } else {
    std::cout << "{";
    for (int i = 0; i < m_array_dim[dim_num]; i++) {
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
      fmt::print("print invalid expr parser state\n");
      return;
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
      FLAT_ADDR addr = MemRemap::flat(m_start_addr, 'd'); // @FIXME remove this and get correct address
      addr += index * type->size();
      std::cout << type->pretty_print(format, expr, addr) << std::endl;
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
      FLAT_ADDR addr = MemRemap::flat(m_start_addr, 'd'); // @FIXME remove this and get correct address
      addr += type->get_member_offset(member_name);
      std::cout << member_type->pretty_print(format, expr, addr) << std::endl;
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
    if (m_array_dim.size() == 0) {
      // single terminal object
      print(format);
    } else {
      FLAT_ADDR flat_addr = MemRemap::flat(m_start_addr, 'd'); // @FIXME remove this and get correct address
      print_array(format, 0, flat_addr, type);
      std::cout << std::endl;
    }
  } else {
    // print all children
  }
}
