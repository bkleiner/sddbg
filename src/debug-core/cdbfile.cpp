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
#include "cdbfile.h"
#include "module.h"
#include "symbol.h"
#include "symtypetree.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

//#define MIN(a,b)	a<?b
#define MIN(a, b) (((a) < (b)) ? a : b)

CdbFile::CdbFile(DbgSession *session)
    : mSession(session)
    , pos(0) {
}

CdbFile::~CdbFile() {
}

bool CdbFile::open(std::string filename) {
  std::cout << "Loading " << filename << std::endl;

  std::ifstream in(filename);
  if (!in.is_open()) {
    std::cout << "ERROR coulden't open file '" << filename.c_str() << "'." << std::endl;
    return false;
  }

  while (!in.eof()) {
    pos = 0;
    getline(in, line);
    parse_record();
  }

  return true;
}

char CdbFile::peek() {
  return line[pos];
}

char CdbFile::consume() {
  return line[pos++];
}

void CdbFile::skip(char c) {
  if (peek() == c) {
    pos++;
  }
}

void CdbFile::skip(std::string c) {
  for (size_t i = 0; i < c.size(); i++) {
    skip(c[i]);
  }
}

std::string CdbFile::consume(std::string::size_type n) {
  auto str = line.substr(pos, n);
  pos += n;
  return str;
}

std::string CdbFile::consume_until(char del) {
  return consume_until(std::string(1, del));
}

std::string CdbFile::consume_until(std::string del_set) {
  size_t index = line.find_first_of(del_set, pos);
  if (index == std::string::npos) {
    return consume(index);
  }
  return consume(index - pos);
}

// parse { <G> | F<filename> | L<function> }
symbol_scope CdbFile::parse_scope() {
  switch (consume()) {
  case 'G':
    return symbol_scope{symbol_scope::GLOBAL};
  case 'F':
    return symbol_scope{symbol_scope::FILE, consume_until('$')};
  case 'L':
    return symbol_scope{symbol_scope::LOCAL, "", consume_until('$')};
  }
  return {};
}

// parse <$><Name><$><Level><$><Block>
symbol_identifier CdbFile::parse_identifier() {
  consume(); // remove $
  auto name = consume_until('$');
  consume(); // remove $
  auto level = consume_until('$');
  consume(); // remove $
  auto block = consume_until("(:");
  return {
      name,
      std::stoi(level),
      std::stoi(block),
  };
}

/** parse a link record
	<L><:>
  { <X> | <A> | <C> | - }
  { <G> | F<filename> | L<function> }
	<$><name>
	<$><level>
	<$><block>
	<:><address> 
*/
bool CdbFile::parse_linker() {
  consume(); // skip ':'

  switch (peek()) {
  // asm record
  case 'A': {
    consume(2); // skip 'A$'
    auto file = consume_until('$');
    consume(); // skip '$'
    auto line = std::stol(consume_until(':'));
    consume(); // skip ':'
    auto addr = std::stoul(consume(std::string::npos), 0, 16);

    mSession->symtab()->add_asm_file_entry(file, line, addr);
    break;
  }

  // c record
  case 'C': {
    consume(2); // skip 'C$'
    auto file = consume_until('$');
    consume(); // skip '$'
    auto line = std::stoul(consume_until('$'));
    consume(); // skip '$'
    auto level = std::stoul(consume_until('$'));
    consume(); // skip '$'
    auto block = std::stoul(consume_until(':'));
    consume(); // skip ':'
    auto addr = std::stoul(consume(std::string::npos), 0, 16);

    mSession->symtab()->add_c_file_entry(file, line, level, block, addr);
    break;
  }
    // end address
  case 'X': {
    consume(); // skip 'X'
    auto scope = parse_scope();
    auto ident = parse_identifier();
    Symbol *sym = mSession->symtab()->add_symbol(scope, ident);
    consume(); // skip ':'

    auto addr = std::stoul(consume(std::string::npos), 0, 16);
    sym->set_end_addr(addr);
    break;
  }
    // memory address
  default: {
    auto scope = parse_scope();
    auto ident = parse_identifier();
    Symbol *sym = mSession->symtab()->add_symbol(scope, ident);
    consume(); // skip ':'

    auto addr = std::stoul(consume(std::string::npos), 0, 16);
    sym->set_addr(addr);
    break;
  }
  }
  return true;
}

/**
 * parse type chain
 * <{><Size><}>
 * <DCLType> <,> {<DCLType> <,>} <:> <Sign>
 */
bool CdbFile::parse_type_chain_record(Symbol *sym) {
  skip('{');
  auto size = std::stoul(consume_until('}'));
  skip('}');

  sym->setLength(size);

  char type_char = 0;
  std::string type_name = "";

  while (peek() != ':') {
    switch (peek()) {
    case 'D': {
      skip('D');
      char c = consume();

      if (c == 'A') {
        sym->AddArrayDim(std::stoul(consume_until(",")));
      }
      if (c == 'F') {
        sym->setIsFunction(true);
      }
      // TODO: handle pointers

      skip(',');
      break;
    }

    case 'S': {
      skip('S');
      type_char = consume();

      if (type_char == 'T') {
        type_name = consume_until(",:");
      }
      if (type_char == 'B') {
        consume_until(",:");
      }

      skip(',');
      break;
    }

    default:
      assert(1 == 0);
    }
  }
  skip(':');

  bool is_signed = consume() == 'S';
  switch (type_char) {
  case 'T':
    break;
  case 'C':
    type_name = is_signed ? "char" : "unsigned char";
    break;
  case 'S':
    type_name = is_signed ? "short" : "unsigned short";
    break;
  case 'I':
    type_name = is_signed ? "int" : "unsigned int";
    break;
  case 'L':
    type_name = is_signed ? "long" : "unsigned long";
    break;
  case 'F':
    type_name = "float";
    break;
  case 'V':
    type_name = "void";
    break;
  case 'X':
    type_name = "sbit";
    break;
  case 'B':
    type_name = "bitfield";
    break;
  default:
    assert(1 == 0);
  }

  if (type_name != "") {
    if (sym->isFunction())
      sym->setReturn(type_name);
    else
      sym->setType(type_name);
  }
  skip(')');
  return true;
}

/** pase type record
 * <T><:>
 * <F><Filename><$>
 * <Name>
 * <[><TypeMember>]>
 */
bool CdbFile::parse_type() {
  skip(":F");

  SymTypeStruct *t = new SymTypeStruct(mSession);
  t->set_file(consume_until('$'));
  skip('$');

  t->set_name(consume_until('['));
  skip('[');

  while (peek() == '(') {
    if (!parse_type_member(t))
      return false;
  }

  return true;
}

/** parse type member
 * <(><{><Offset><}><SymbolRecord><)>
 */
bool CdbFile::parse_type_member(SymTypeStruct *t) {
  if (consume() != '(')
    return false;

  if (consume() != '{')
    return false;

  auto offset = std::stoul(consume_until('}'));
  skip('}');

  skip("S:S");
  auto ident = parse_identifier();
  skip('(');

  skip('{');
  auto size = std::stoul(consume_until('}'));
  skip('}');

  char type_char = 0;
  std::string type_name = "";
  uint32_t array_element_cnt = 1;

  while (peek() != ':') {
    switch (peek()) {
    case 'D': {
      skip('D');
      char c = consume();

      if (c == 'A') {
        array_element_cnt = std::stoul(consume_until(","));
      }
      // TODO: handle pointers

      skip(',');
      break;
    }

    case 'S': {
      skip('S');
      type_char = consume();

      if (type_char == 'T') {
        type_name = consume_until(",:");
      }
      if (type_char == 'B') {
        consume_until(",:");
      }

      skip(',');
      break;
    }

    default:
      assert(1 == 0);
    }
  }
  skip(':');

  bool is_signed = consume() == 'S';
  switch (type_char) {
  case 'T':
    break;
  case 'C':
    type_name = is_signed ? "char" : "unsigned char";
    break;
  case 'S':
    type_name = is_signed ? "short" : "unsigned short";
    break;
  case 'I':
    type_name = is_signed ? "int" : "unsigned int";
    break;
  case 'L':
    type_name = is_signed ? "long" : "unsigned long";
    break;
  case 'F':
    type_name = "float";
    break;
  case 'V':
    type_name = "void";
    break;
  case 'X':
    type_name = "sbit";
    break;
  case 'B':
    type_name = "bitfield";
    break;
  default:
    assert(1 == 0);
  }

  t->add_member(offset, ident.name, type_name, array_element_cnt);
  skip("),Z,0,0");

  return consume() == ')';
}

bool CdbFile::parse_record() {
  if (line[1] != ':')
    return false;

  int npos = 0;
  switch (consume()) {
  case 'M':
    consume();
    cur_module = consume(std::string::npos);
    break;
  case 'F': {
    // <F><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
    // <$><Name><$><Level><$><Block><(><TypeRecord><)><,><AddressSpace>
    // <,><OnStack><,><Stack><,><Interrupt><,><Interrupt Num>
    // <,><Register Bank>

    consume(); // skip ':'
    auto scope = parse_scope();
    auto ident = parse_identifier();
    auto sym = mSession->symtab()->add_symbol(scope, ident);
    consume(); // skip '('

    sym->setIsFunction(true);
    sym->set_c_file(cur_module + ".c");
    sym->set_c_file(cur_module + ".asm");

    parse_type_chain_record(sym);

    consume(); // skip ','
    sym->set_addr_space(consume());
    consume(); // skip ','

    auto on_stack = consume_until(",");
    consume(); // skip ','
    auto stack_offset = consume_until(",");
    if (on_stack != "0") {
      sym->set_stack_offset(std::stoi(stack_offset));
    }
    consume(); // skip ','

    sym->set_interrupt(consume_until(",") != "0");
    consume(); // skip ','

    sym->set_interrupt_num(std::stoul(consume_until(",")));
    consume(); // skip ','

    sym->set_reg_bank(std::stoul(consume(std::string::npos)));
    break;
  }
  case 'S': {
    // <S><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
    // <$><Name><$><Level><$><Block><(><TypeRecord><)>
    // <,><AddressSpace><,><OnStack><,><Stack><,><[><Reg><,>{<Reg><,>}<]>+

    consume(); // skip ':'
    auto scope = parse_scope();
    auto ident = parse_identifier();
    auto sym = mSession->symtab()->add_symbol(scope, ident);
    consume(); // skip '('

    sym->set_c_file(cur_module + ".c");
    sym->set_c_file(cur_module + ".asm");

    parse_type_chain_record(sym);

    consume(); // skip ','
    sym->set_addr_space(consume());
    consume(); // skip ','

    auto on_stack = consume_until(",");
    consume(); // skip ','
    auto stack_offset = consume_until(",");
    if (on_stack != "0") {
      sym->set_stack_offset(std::stoi(stack_offset));
    }
    consume(); // skip ','

    if (consume() == '[') {
      while (peek() != ']') {
        sym->add_reg(consume_until(",]"));
        if (peek() == ',')
          consume(); // skip ','
      };
    }
    break;
  }
  case 'T':
    parse_type();
    break;

  case 'L':
    parse_linker();
    break;

  default:
    break;
  }
  return true;
}
