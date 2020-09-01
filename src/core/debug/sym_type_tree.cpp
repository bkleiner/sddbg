#include "sym_type_tree.h"

#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "context_mgr.h"
#include "dbg_session.h"
#include "mem_remap.h"
#include "out_format.h"
#include "target.h"

namespace debug::core {

  sym_type_tree::sym_type_tree(dbg_session *session)
      : session(session) {
    clear();
  }

  sym_type_tree::~sym_type_tree() {
  }

  /** Clear out the tree and add back the terminal types.
*/
  void sym_type_tree::clear() {
    m_types_scope.clear();
    m_types.clear();

    // Add terminal types to the tree
    m_types.reserve(10);
    m_types.push_back(new sym_type_char(session));
    m_types.push_back(new sym_type_uchar(session));
    m_types.push_back(new sym_type_short(session));
    m_types.push_back(new sym_type_ushort(session));
    m_types.push_back(new sym_type_int(session));
    m_types.push_back(new sym_type_uint(session));
    m_types.push_back(new sym_type_long(session));
    m_types.push_back(new sym_type_ulong(session));
    m_types.push_back(new sym_type_float(session));
    m_types.push_back(new sym_type_sbit(session));
  }

  bool sym_type_tree::add_type(sym_type *ptype) {
    m_types.push_back(ptype);
    return true;
  }

  void sym_type_tree::dump() {
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

  void sym_type_tree::dump(std::string type_name) {
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

  sym_type *sym_type_tree::get_type(std::string type_name, context ctx) {
    /// @FIXME: this a a crude hack that just takes the first matching type and ignores the scope requirments
    for (int i = 0; i < m_types.size(); i++) {
      if (m_types[i]->name() == type_name) {
        return m_types[i];
      }
    }

    return nullptr; // not found
  }

  std::string sym_type_tree::pretty_print(sym_type *ptype, char fmt, uint32_t flat_addr, std::string subpath) {
    std::cout << "Sorry Print not implemented for this type!" << std::endl;
    return "";
  }

  ////////////////////////////////////////////////////////////////////////////////
  // sym_type_char / sym_type_uchar
  ////////////////////////////////////////////////////////////////////////////////

  std::string sym_type_char::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, 1);
  }

  std::string sym_type_uchar::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, 1);
  }

  std::string sym_type_int::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, size());
  }

  std::string sym_type_uint::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, size());
  }

  std::string sym_type_long::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, size());
  }

  std::string sym_type_ulong::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, size());
  }

  std::string sym_type_float::pretty_print(char fmt, target_addr addr) {
    out_format of(session);
    return of.print(fmt == 0 ? default_format() : fmt, addr, size());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // sym_type_struct
  ////////////////////////////////////////////////////////////////////////////////

  int32_t sym_type_struct::size() {
    int32_t size = 0;
    for (auto &m : m_members) {
      auto type = session->symtree()->get_type(m.type_name, {});
      size += type->size();
    }
    return size;
  }

  ADDR sym_type_struct::get_member_offset(std::string member_name) {
    uint32_t offset = 0;
    for (auto &m : m_members) {
      if (m.member_name == member_name) {
        return m.offset;
      }
    }
    return 0;
  }

  sym_type *sym_type_struct::get_member_type(std::string member_name) {
    for (auto &m : m_members) {
      if (m.member_name == member_name)
        return session->symtree()->get_type(m.type_name, {});
    }
    return nullptr;
  }

  // GDB standard text representation
  std::string sym_type_struct::text() {
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
  void sym_type_struct::add_member(ADDR offset, std::string member_name, std::string type_name, uint32_t count) {
    m_members.emplace_back(offset, member_name, type_name, count);
  }
} // namespace debug::core