/***************************************************************************
 *   Copyright (C) 2005 by Ricky White   *
 *   rickyw@neatstuff.co.nz   *
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
#include "symtypetree.h"
#include "contextmgr.h"
#include "dbgsession.h"
#include "memremap.h"
#include "outformat.h"
#include "target.h"
#include <iomanip>
#include <iostream>
#include <stdio.h>

SymTypeTree::SymTypeTree(DbgSession *session)
    : mSession(session) {
  clear();
}

SymTypeTree::~SymTypeTree() {
}

/** Clear out the tree and add back the terminal types.
*/
void SymTypeTree::clear() {
  m_types_scope.clear();
  m_types.clear();

  // Add terminal types to the tree
  m_types.reserve(10);
  m_types.push_back(new SymTypeChar(mSession));
  m_types.push_back(new SymTypeUChar(mSession));
  m_types.push_back(new SymTypeShort(mSession));
  m_types.push_back(new SymTypeUShort(mSession));
  m_types.push_back(new SymTypeInt(mSession));
  m_types.push_back(new SymTypeUInt(mSession));
  m_types.push_back(new SymTypeLong(mSession));
  m_types.push_back(new SymTypeULong(mSession));
  m_types.push_back(new SymTypeFloat(mSession));
  m_types.push_back(new SymTypeSbit(mSession));
}

bool SymTypeTree::add_type(SymType *ptype) {
  m_types.push_back(ptype);
  return true;
}

void SymTypeTree::dump() {
  std::cout << std::setw(24) << std::left << "Type name"
            << std::setw(9) << std::left << "Terminal"
            << std::setw(8) << std::left << "Size"
            << std::setw(24) << std::left << "Scope"
            << std::endl;
  std::cout << std::string(80, '=') << std::endl;
  for (int i = 0; i < m_types.size(); i++) {
    std::cout << std::setw(24) << std::left << m_types[i]->name()
              << std::setw(9) << std::left << std::boolalpha << m_types[i]->terminal()
              << std::setw(8) << std::left << m_types[i]->size()
              << std::setw(24) << std::left << m_types[i]->file()
              << std::endl;
  }
  std::cout << std::endl;
}

void SymTypeTree::dump(std::string type_name) {
  // find type
  for (int i = 0; i < m_types.size(); i++) {
    std::cout << "checking " << m_types[i]->name() << std::endl;
    if (m_types[i]->name() == type_name) {
      std::cout << "Dumping type = '" << type_name << "'" << std::endl;
      std::cout << m_types[i]->text() << std::endl;
      return;
    }
  }
  std::cout << "ERROR Type = '" << type_name << "' not found." << std::endl;
}

/** Search for the specified type is the supplied context.
	Starts at closest scope and works out.
		1) blocks in the current function
		2) File scope
		3) Global scope

	\param name		Name of the type to search for.
	\param context	Context to search for the type
	\returns 		A pointer to the best matching type entry or NULL if not found
*/
SymType *SymTypeTree::get_type(std::string type_name,
                               ContextMgr::Context context) {
  //std::cout << "looking for '" << type_name << "'" << std::endl;

  /// @FIXME: this a a crude hack that just takes the first matching type and ignores the scope requirments
  for (int i = 0; i < m_types.size(); i++) {
    if (m_types[i]->name() == type_name) {
      return m_types[i];
    }
  }

  return nullptr; // not found
}

std::string SymTypeTree::pretty_print(SymType *ptype,
                                      char fmt,
                                      uint32_t flat_addr,
                                      std::string subpath) {
  std::cout << "Sorry Print not implemented for this type!" << std::endl;
  return "";
}

////////////////////////////////////////////////////////////////////////////////
// SymTypeChar / SymTypeUChar
////////////////////////////////////////////////////////////////////////////////

std::string SymTypeChar::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, 1);
}

std::string SymTypeUChar::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, 1);
}

std::string SymTypeInt::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, size());
}

std::string SymTypeUInt::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, size());
}

std::string SymTypeLong::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, size());
}

std::string SymTypeULong::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, size());
}

std::string SymTypeFloat::pretty_print(char fmt, target_addr addr) {
  OutFormat of(mSession);
  return of.print(fmt == 0 ? default_format() : fmt, addr, size());
}

////////////////////////////////////////////////////////////////////////////////
// SymTypeStruct
////////////////////////////////////////////////////////////////////////////////

int32_t SymTypeStruct::size() {
  int32_t size = 0;
  for (auto &m : m_members) {
    auto type = mSession->symtree()->get_type(m.type_name, {});
    size += type->size();
  }
  return size;
}

ADDR SymTypeStruct::get_member_offset(std::string member_name) {
  uint32_t offset = 0;
  for (auto &m : m_members) {
    if (m.member_name == member_name) {
      return m.offset;
    }
  }
  return 0;
}

SymType *SymTypeStruct::get_member_type(std::string member_name) {
  for (auto &m : m_members) {
    if (m.member_name == member_name)
      return mSession->symtree()->get_type(m.type_name, {});
  }
  return nullptr;
}

// GDB standard text representation
std::string SymTypeStruct::text() {
  char buf[255];
  std::string s;

  for (int i = 0; i < m_members.size(); i++) {
    if (m_members[i].count != 1)
      snprintf(buf, sizeof(buf), "%s %s[%i]\n",
               m_members[i].type_name.c_str(),
               m_members[i].member_name.c_str(),
               m_members[i].count);
    else
      snprintf(buf, sizeof(buf), "%s %s\n",
               m_members[i].type_name.c_str(),
               m_members[i].member_name.c_str());
    s += buf;
  }
  return s;
}

/** Add a member to a structure.
	\param name		Name of the member to add.
	\param ptype	pointer to the type to add.
	\param count	Count of number of items if the member is an array.

	This type only stores the names of the associated types.
	The lookup for each must be done manually for now.
*/
void SymTypeStruct::add_member(ADDR offset, std::string member_name, std::string type_name, uint32_t count) {
  m_members.emplace_back(offset, member_name, type_name, count);
}
