#include "cdb_file.h"

#include <filesystem>

#include <fstream>
#include <stdexcept>
#include <string>

#include "log.h"
#include "module.h"
#include "sym_type_tree.h"
#include "symbol.h"

namespace fs = std::filesystem;

namespace debug::core {
  cdb_file::cdb_file(dbg_session *session)
      : line_parser("")
      , session(session) {
  }

  cdb_file::~cdb_file() {
  }

  bool cdb_file::open(std::string filename, std::string dir) {
    log::print("loading \"{}\"\n", filename);

    src_dir = base_dir = fs::absolute(filename).parent_path();
    if (dir != "") {
      src_dir = dir;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
      log::print("ERROR coulden't open file \"{}\"\n", filename);
      return false;
    }

    std::string str;
    while (std::getline(file, str)) {
      reset(str);

      if (!parse_record()) {
        return false;
      }
    }

    return true;
  }

  // parse { <G> | F<filename> | L<function> }
  symbol_scope cdb_file::parse_scope() {
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
  symbol_identifier cdb_file::parse_identifier() {
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
  bool cdb_file::parse_linker() {
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

      if (!session->symtab()->add_asm_file_entry(fs::path(base_dir).append(file), line, addr)) {
        log::print("ERROR loading \"{}\"\n", file);
        return false;
      }
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

      if (!session->symtab()->add_c_file_entry(fs::path(src_dir).append(file), line, level, block, addr)) {
        log::print("ERROR loading \"{}\"\n", file);
        return false;
      }
      break;
    }
      // end address
    case 'X': {
      consume(); // skip 'X'
      auto scope = parse_scope();
      auto ident = parse_identifier();
      symbol *sym = session->symtab()->add_symbol(scope, ident);
      consume(); // skip ':'

      auto addr = std::stoi(consume(std::string::npos), 0, 16);
      sym->set_end_addr({sym->addr().space, addr});
      break;
    }
      // memory address
    default: {
      auto scope = parse_scope();
      auto ident = parse_identifier();
      symbol *sym = session->symtab()->add_symbol(scope, ident);
      consume(); // skip ':'

      auto addr = std::stoi(consume(std::string::npos), 0, 16);
      sym->set_addr({sym->addr().space, addr});
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
  bool cdb_file::parse_type_chain_record(symbol *sym) {
    skip('{');
    auto size = std::stoul(consume_until('}'));
    skip('}');

    sym->set_length(size);

    char type_char = 0;
    std::string type_name = "";

    while (peek() != ':') {
      switch (peek()) {
      case 'D': {
        skip('D');
        char c = consume();

        if (c == 'A') {
          sym->set_type(symbol::ARRAY);
          sym->add_array_size(std::stoul(consume_until(",")));
        }
        if (c == 'F') {
          sym->set_type(symbol::FUNCTION);
        }
        // TODO: handle pointers

        skip(',');
        break;
      }

      case 'S': {
        skip('S');
        type_char = consume();

        if (type_char == 'T') {
          sym->set_type(symbol::STRUCT);
          type_name = consume_until(",:");
        }
        if (type_char == 'B') {
          consume_until(",:");
        }

        skip(',');
        break;
      }

      default:
        throw std::runtime_error(fmt::format("invalid type prefix {}", peek()));
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
      throw std::runtime_error(fmt::format("invalid type {}", type_char));
    }

    if (type_name != "") {
      sym->set_type_name(type_name);
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
  bool cdb_file::parse_type() {
    skip(":F");

    sym_type_struct *t = new sym_type_struct(session);
    t->set_file(consume_until('$'));
    skip('$');

    t->set_name(consume_until('['));
    skip('[');

    while (peek() == '(') {
      if (!parse_type_member(t))
        return false;
    }

    session->symtree()->add_type(t);

    return true;
  }

  /** parse type member
 * <(><{><Offset><}><SymbolRecord><)>
 */
  bool cdb_file::parse_type_member(sym_type_struct *t) {
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
        throw std::runtime_error(fmt::format("invalid type prefix {}", peek()));
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
      throw std::runtime_error(fmt::format("invalid type {}", type_char));
    }

    t->add_member(offset, ident.name, type_name, array_element_cnt);
    skip("),Z,0,0");

    return consume() == ')';
  }

  bool cdb_file::parse_record() {
    if (line[1] != ':')
      return false;

    char record_type = consume();
    switch (record_type) {
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
      auto sym = session->symtab()->add_symbol(scope, ident);
      consume(); // skip '('

      sym->set_type(symbol::FUNCTION);
      sym->set_c_file(cur_module + ".c");
      sym->set_asm_file(cur_module + ".asm");

      if (!parse_type_chain_record(sym)) {
        return false;
      }

      consume(); // skip ','
      sym->set_addr(target_addr::from_name(consume(), INVALID_ADDR));
      consume(); // skip ','

      auto on_stack = consume_until(",");
      consume(); // skip ','
      auto stack_offset = consume_until(",");
      if (on_stack != "0") {
        sym->set_stack_offset(std::stoi(stack_offset));
      }
      consume(); // skip ','

      if (consume_until(",") != "0") {
        sym->set_type(symbol::INTERRUPT);
      }
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
      auto sym = session->symtab()->add_symbol(scope, ident);
      consume(); // skip '('

      sym->set_type(symbol::VARIABLE);
      sym->set_c_file(cur_module + ".c");
      sym->set_asm_file(cur_module + ".asm");

      if (!parse_type_chain_record(sym)) {
        return false;
      }

      consume(); // skip ','
      sym->set_addr(target_addr::from_name(consume(), INVALID_ADDR));
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
      return parse_type();

    case 'L':
      return parse_linker();

    default:
      log::print("unhandled record type {}\n", record_type);
      break;
    }
    return true;
  }
} // namespace debug::core