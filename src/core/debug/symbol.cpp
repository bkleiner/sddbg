#include "symbol.h"

#include <assert.h>

#include <stdio.h>
#include <string.h>

#include "context_mgr.h"
#include "log.h"
#include "mem_remap.h"
#include "sym_type_tree.h"

namespace debug::core {

  constexpr const char *const symbol_scope::names[];

  symbol::symbol(dbg_session *session, symbol_scope _scope, symbol_identifier _ident)
      : session(session)
      , _scope(_scope)
      , _ident(_ident)
      , on_stack(false)
      , type(0)
      , m_length(-1) {}

  symbol::~symbol() {
  }

  void symbol::set_addr(target_addr addr) {
    _start_addr = addr;
    if (m_length != -1)
      _end_addr = _start_addr + ADDR(m_length - 1);
  }

  void symbol::set_end_addr(target_addr addr) {
    _end_addr = addr;
    m_length = _end_addr.addr - _start_addr.addr + 1;
  }

  void symbol::set_length(int len) {
    m_length = len;
    _end_addr = _start_addr + ADDR(m_length - 1);
  }

  void symbol::dump() {
    std::string name = _ident.name;
    char buf[255];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < m_array_size.size(); i++) {
      snprintf(buf, sizeof(buf), "[%i]", m_array_size[i]);
      name += buf;
    }
    log::printf("%-15s0x%08x  0x%08x  %-9s %-8s %-12s %c %-10s",
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
      log::printf("Regs: ");
      for (it = m_regs.begin(); it != m_regs.end(); ++it) {
        log::printf("%s ", it->c_str());
      }
    }
    log::printf("\n");
  }

  std::string symbol::sprint(char format) {
    sym_type *type = session->symtree()->get_type(m_type_name, {});
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
  void symbol::print(char format) {
    log::print("{} = {}\n", _ident.name, sprint(format));
  }

  /** Recursive function to print out an complete arrays contents.
*/
  void symbol::print_array(char format, int dim_num, target_addr addr, sym_type *type) {
    if (dim_num == (m_array_size.size() - 1)) {
      // special case default format with char array
      if (format == 0 && (type->name() == "char" || type->name() == "unsigned char")) {
        log::print("\"");
        for (int i = 0; i < m_array_size[dim_num /*-1*/]; i++) {
          log::print(type->pretty_print('s', addr));
          addr = addr + type->size();
        }
        log::print("\"");
      } else {
        log::print("{");
        for (int i = 0; i < m_array_size[dim_num /*-1*/]; i++) {
          log::print((i > 0 ? ",{}" : "{}"), type->pretty_print(format, addr));
          addr = addr + type->size();
        }
        log::print("}");
      }
    } else {
      log::print("{");
      for (int i = 0; i < m_array_size[dim_num]; i++) {
        print_array(format, dim_num + 1, addr, type);
      }
      log::print("}");
    }
    log::print("\n");
  }

  std::string symbol::sprint(char format, std::string expr) {
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

    const auto ctx = session->contextmgr()->get_current();
    if (array_subscripts.size()) {
      sym_type *type = session->symtree()->get_type(m_type_name, ctx);

      // @FIXME: this dosen't handle multiple dimensions
      const uint32_t index = array_subscripts[0];
      if (type->terminal()) {
        // calculate memory location
        target_addr addr = _start_addr + ADDR(index * type->size());
        return type->pretty_print(format, addr);
      }
      return "";
    }

    if (member_names.size()) {
      sym_type_struct *type = dynamic_cast<sym_type_struct *>(session->symtree()->get_type(m_type_name, ctx));
      if (type == nullptr)
        return "";

      // @FIXME: this dosen't handle multiple dimensions
      const std::string member_name = member_names[0];
      sym_type *member_type = type->get_member_type(member_name);
      if (member_type != nullptr && member_type->terminal()) {
        // calculate memory location
        target_addr addr = _start_addr + type->get_member_offset(member_name);
        return member_type->pretty_print(format, addr);
      }
      return "";
    }

    // if we get here and the symbol says it is not terminal then we must print all its children

    // Either a terminal or an array of terminals where we print all.
    // array count is part of symbol.
    sym_type *type = session->symtree()->get_type(m_type_name, ctx);
    if (!type) {
      return "";
    }

    if (type->terminal()) {
      if (m_array_size.size() == 0) {
        return sprint(format);
      } else {
        print_array(format, 0, _start_addr, type);
      }
    } else {
      auto complex = dynamic_cast<sym_type_struct *>(type);
      for (auto &m : complex->get_members()) {
        sym_type *member_type = complex->get_member_type(m.member_name);
        return fmt::format("{} = {}\n", m.member_name, member_type->pretty_print(format, _start_addr + m.offset));
      }
    }

    return "";
  }

  /** Print the symbol,  The expression must start with the symnbol.
	expr is used to determine if a single element or the entire array is to be printed
*/
  void symbol::print(char format, std::string expr) {
    log::print("expr = '{}'\n", expr);
    log::print(sprint(format, expr));
  }
} // namespace debug::core