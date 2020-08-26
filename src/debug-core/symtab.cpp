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
#include "symtab.h"
#include "module.h"
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

SymTab::SymTab(DbgSession *session)
    : mSession(session) {
}

SymTab::~SymTab() {
}

void SymTab::clear() {
  // @TODO clear all tables
  m_symlist.clear();
  file_map.clear();
  c_file_list.clear();
  asm_file_list.clear();
}

Symbol *SymTab::add_symbol(const symbol_scope &scope, const symbol_identifier &ident) {
  Symbol *sym = get_symbol(scope, ident);
  if (sym != nullptr) {
    return sym;
  }
  m_symlist.emplace_back(mSession, scope, ident);
  return get_symbol(scope, ident);
}

Symbol *SymTab::get_symbol(const symbol_scope &scope, const symbol_identifier &ident) {
  for (auto &sym : m_symlist) {
    if (sym.get_scope() == scope && sym.get_ident() == ident) {
      return &sym;
    }
  }
  return nullptr;
}

std::vector<Symbol *> SymTab::get_symbols(ContextMgr::Context ctx) {
  std::vector<Symbol *> result;

  for (auto &sym : m_symlist) {
    if (sym.is_type(Symbol::FUNCTION)) {
      continue;
    }

    switch (sym.scope()) {
    case symbol_scope::GLOBAL:
      result.push_back(&sym);
      break;

    case symbol_scope::FILE:
      if (sym.file() == ctx.module + ".c" || sym.file() == ctx.module + ".asm")
        result.push_back(&sym);
      break;

    case symbol_scope::LOCAL:
      if ((sym.function() == ctx.module + "." + ctx.function)) {
        result.push_back(&sym);
      }
      break;

    default:
      break;
    }
  }

  return result;
}

Symbol *SymTab::get_symbol(const ContextMgr::Context &ctx, const std::string &name) {
  // there are more efficient ways of doing this.
  // this is just quick and dirty for now and should be cleaned up

  // search local scope
  for (auto &sym : m_symlist) {
    if ((sym.name().compare(name) == 0) &&
        ((sym.file().compare(ctx.module + ".c")) || (sym.file().compare(ctx.module + ".asm"))) &&
        (sym.function() == ctx.module + "." + ctx.function) &&
        (sym.scope() == symbol_scope::LOCAL)) {
      return &sym;
    }
  }

  // File scope
  for (auto &sym : m_symlist) {
    if ((sym.name().compare(name) == 0) &&
        ((sym.file().compare(ctx.module + ".c")) || (sym.file().compare(ctx.module + ".asm"))) &&
        (sym.scope() == symbol_scope::FILE)) {
      return &sym;
    }
  }

  // Global scope
  for (auto &sym : m_symlist) {
    if ((sym.name().compare(name) == 0) &&
        (sym.scope() == symbol_scope::GLOBAL)) {
      return &sym;
    }
  }

  return nullptr;
}

void SymTab::dump() {
  dump_symbols();
  dump_c_lines();
  dump_asm_lines();
  dump_functions();
}

void SymTab::dump_symbols() {
  SYMLIST::iterator it;
  std::cout << "Name           Start addr  End addr    File       Scope   Function\tMem Area\tType" << std::endl;
  std::cout << "=================================================================================" << std::endl;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    it->dump();
  }
}

void SymTab::dump_c_lines() {
  FILE_LIST::iterator it;
  std::cout << "\n\nC file lines" << std::endl;
  std::cout << "name\tline\tlevel\tblock\taddr" << std::endl;
  std::cout << "=================================================================================" << std::endl;
  std::cout << std::endl;
  for (it = c_file_list.begin(); it != c_file_list.end(); ++it) {
    printf("%s\t%i\t%i\t%i\t0x%08x\n",
           file_name((*it).file_id).c_str(),
           (*it).line_num,
           (*it).level,
           (*it).block,
           (*it).addr);
  }
}

void SymTab::dump_asm_lines() {
  FILE_LIST::iterator it;
  std::cout << "\n\nASM file lines" << std::endl;
  std::cout << "name\tline\taddr" << std::endl;
  std::cout << "=================================================================================" << std::endl;
  std::cout << std::endl;

  for (it = asm_file_list.begin(); it != asm_file_list.end(); ++it) {
    printf("%s\t%i\t0x%08x\n",
           file_name((*it).file_id).c_str(),
           (*it).line_num,
           (*it).addr);
  }
}

///////////////////////////////////////////////////////////////////////////////
// address from functions
///////////////////////////////////////////////////////////////////////////////
/** Get the address that relates to a specific line number in a file
	\returns >=0 address, -1 = failure
*/
int32_t SymTab::get_addr(std::string file, int line_num) {
  FILE_LIST::iterator it;
  int fid = file_id(file);
  if (fid == -1)
    return -1; // failure
  if (file.substr(file.length() - 2).compare(".c") == 0) {
    for (it = c_file_list.begin(); it != c_file_list.end(); ++it)
      if ((*it).file_id == fid && (*it).line_num == line_num)
        return (*it).addr;
    std::cout << " Error: " << file << " line number not found" << std::endl;
  } else if (file.substr(file.length() - 4).compare(".asm") == 0 ||
             file.substr(file.length() - 4).compare(".a51") == 0) {
    for (it = asm_file_list.begin(); it != asm_file_list.end(); ++it)
      if ((*it).file_id == fid && (*it).line_num == line_num)
        return (*it).addr;
    std::cout << " Error: " << file << " line number not found" << std::endl;
  } else {
    printf("Sorry I don't know how to handle File type = '%s'\n", file.substr(file.find(".") + 1).c_str());
    return -1; // failure
  }
  return -1; // failure
}

/** Get the address of the start of a function in a file
	\returns >=0 address, -1 = failure
*/
bool SymTab::get_addr(std::string file, std::string function, int32_t &addr,
                      int32_t &endaddr) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).is_type(Symbol::FUNCTION)) {
      std::cout << "checking file=" << (*it).file() << " function=" << (*it).name() << std::endl;
      if ((*it).name() == function) {
        addr = (*it).addr();
        endaddr = (*it).end_addr();
        return true;
      }
    }
  }
  return false;
}

/** Get the address of the start of a function (any file)
	\returns >=0 address, -1 = failure
*/
int32_t SymTab::get_addr(std::string function) {
  FILE_LIST::iterator it;
  for (it = c_file_list.begin(); it != c_file_list.end(); ++it)
    if ((*it).function == function)
      return (*it).addr;
  return -1; // failure
}

bool SymTab::get_addr(std::string function, int32_t &addr, int32_t &endaddr) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).is_type(Symbol::FUNCTION)) {
      if ((*it).name() == function) {
        addr = (*it).addr();
        endaddr = (*it).end_addr();
        return true;
      }
    }
  }
  return false; // failure
}

bool SymTab::find_c_file_line(ADDR addr, std::string &file, LINE_NUM &line_num) {
  FILE_LIST::iterator it;

  for (it = c_file_list.begin(); it != c_file_list.end(); ++it)
    if ((*it).addr == addr) {
      file = file_name((*it).file_id);
      line_num = (*it).line_num;
      return true;
    }
  file = "no match";
  line_num = LINE_NUM(-1);
  return false; // not found
}

bool SymTab::find_asm_file_line(uint16_t addr, std::string &file, int &line_num) {
  FILE_LIST::iterator it;
  for (it = asm_file_list.begin(); it != asm_file_list.end(); ++it)
    if ((*it).addr == addr) {
      file = file_name((*it).file_id);
      line_num = (*it).line_num;
      return true;
    }
  file = "no match";
  line_num = -1;
  return false; // not found
}

//bool SymTab::add_c_file_entry( std::string name, int line_num, uint16_t addr )
bool SymTab::add_c_file_entry(std::string name, int line_num, int level, int block, uint16_t addr) {
  int fid = file_id(name);
  std::cout << "*** ADDING C FILE '" << name << "'" << std::endl;
  // note add resturns the exsisting module if one exsists.
  Module &m =
      mSession->modulemgr()->add_module(name.substr(0, name.length() - 2)); //-".c"
  if (fid == -1) {
    // first time we have encountered this c file
    file_map.push_back(name);
    fid = file_id(name);

    m.load_c_file(name);
  }
  // build and add the entry
  FILE_ENTRY ent;
  ent.file_id = fid;
  ent.line_num = line_num;
  ent.level = level;
  ent.block = block;
  ent.addr = addr;
  c_file_list.push_back(ent);
  m.set_c_addr(line_num, addr);
  m.set_c_block_level(line_num, block, level);
  return true;
}

bool SymTab::add_asm_file_entry(std::string name, int line_num, uint16_t addr) {
  // @FIXME need to know if the file is a .a51 of an .asm
  //		  use convention .asm if we have a matching c file, .a51 if not?
  struct stat buf;
  std::string ext;
  if (stat((name + ".asm").c_str(), &buf) == 0)
    ext = ".asm";
  if (stat((name + ".a51").c_str(), &buf) == 0)
    ext = ".a51";

  int fid = file_id(name + ext);
  // note add returns the exsisting module if one exsists.
  Module &m = mSession->modulemgr()->add_module(name);

  if (fid == -1) {
    std::cout << "loading ASM '" << name << "'" << std::endl;
    file_map.push_back(name + ext);
    fid = file_id(name + ext);
    m.load_asm_file(name + ext);
  }
  // build and add the entry
  FILE_ENTRY ent;
  ent.file_id = fid;
  ent.line_num = line_num;
  ent.addr = addr;
  asm_file_list.push_back(ent);

  m.set_asm_addr(line_num, addr);
  return true;
  return false;
}

int SymTab::file_id(std::string filename) {
  // iterate over the std::vector till we fine a match or the end.
  int i = 0;
  while (i < file_map.size()) {
    //std::cout<<i<<" = ["<<file_map[i]<<"]"<<std::endl;
    if (file_map[i] == filename)
      return i;
    i++;
  }
  return -1; // Failure
}

std::string SymTab::file_name(int id) {
  return file_map[id];
}

void SymTab::dump_functions() {
  //	FUNC_LIST::iterator it;
  printf("File                  Function              start     end\n");
  std::cout << "=================================================================================" << std::endl;
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).is_type(Symbol::FUNCTION)) {
      printf("%-20s  %-20s  0x%08x  0x%08x\n",
             (*it).file().c_str(),
             (*it).name().c_str(),
             (*it).addr(),
             (*it).end_addr());
    }
  }
}

/** get the name of a function that the specified code address is within
	\param address to find out which function it is part of.
	\returns true on success, false on failure ( no function found)
*/
bool SymTab::get_c_function(ADDR addr,
                            std::string &file,
                            std::string &func) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).is_type(Symbol::FUNCTION)) {
      if (addr >= (*it).addr() && addr <= (*it).end_addr()) {
        func = (*it).name();

        return true;
      }
      //			printf("%-20s  %-20s  0x%08x  0x%08x\n",
      //				   (*it).file().c_str(),
      //				   (*it).name().c_str(),
      //				   (*it).addr(),
      //				   (*it).end_addr()
      //				  );
    }
  }
  return false;
}

bool SymTab::get_c_block_level(std::string file,
                               LINE_NUM line,
                               BLOCK &block,
                               LEVEL &level) {
  FILE_LIST::iterator it;
  for (it = c_file_list.begin(); it != c_file_list.end(); ++it) {
    if (file_name((*it).file_id).c_str() == file) {
      //std::cout << "FOUND: correct file"<<std::endl;
      if ((*it).line_num == line) {
        level = (*it).level;
        block = (*it).block;
        return true;
      }
    }
  }
  return false; // failure
}

/// @FIXME dosen't work flat vs normal address issue
std::string SymTab::get_symbol_name(FLAT_ADDR addr) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).addr() == addr)
      return (*it).name();
  }
  return "NOT IMPLEMENTED";
}
