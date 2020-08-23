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

bool SymTab::getSymbol(std::string file,
                       Symbol::SCOPE scope,
                       std::string name,
                       SYMLIST::iterator &it) {
  std::cout << "looking for " << file << ", " << name << ", " << scope << std::endl;
  for (it = m_symlist.begin(); it != m_symlist.end(); it++) {
    std::cout << "checking " << it->file() << ", " << it->scope() << ", " << it->name() << std::endl;
    if (it->file().compare(file) == 0 &&
        it->scope() == scope &&
        it->name().compare(name) == 0) {
      //std::cout <<"lookup match!!!!!"<<std::endl;
      return true;
    }
  }
  return false;
}

bool SymTab::getSymbol(std::string name,
                       ContextMgr::Context context,
                       SYMLIST::iterator &it) {
  // there are more efficient ways of doing this.
  // this is just quick and dirty for now and should be cleaned up

  // search local scope
  for (it = m_symlist.begin(); it != m_symlist.end(); it++) {
    if ((it->name().compare(name) == 0) &&
        ((it->file().compare(context.module + ".c")) ||
         (it->file().compare(context.module + ".asm"))) &&
        (it->function() == context.module + "." + context.function) &&
        (it->scope() == Symbol::SCOPE_LOCAL)) {
      std::cout << "MATCH LOCAL" << std::endl;
      return true;
    }
  }

  // File scope
  for (it = m_symlist.begin(); it != m_symlist.end(); it++) {
    if ((it->name().compare(name) == 0) &&
        ((it->file().compare(context.module + ".c")) ||
         (it->file().compare(context.module + ".asm"))) &&
        (it->scope() == Symbol::SCOPE_FILE)) {
      std::cout << "MATCH FILE" << std::endl;
      return true;
    }
  }

  // Global scope
  for (it = m_symlist.begin(); it != m_symlist.end(); it++) {
    if ((it->name().compare(name) == 0) &&
        (it->scope() == Symbol::SCOPE_GLOBAL)) {
      std::cout << "MATCH FILE" << std::endl;
      return true;
    }
  }
  return false;
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

void SymTab::addSymbol(Symbol sym) {
  // check we don't alreeady have a symbol by that name
  // @FIXME: currently we are seeing duplicate symbols here and are creating 2 entries...
  // FIXME: the duplicate check should n't be here, we know what will double up in cdbfile so detect and manage  it there.

  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    //		if( (*it).name()==sym.name()	&&
    //			(*it).file()==sym.file()	&&
    //			(*it).level()==sym.level()	&&
    //			(*it).block()==sym.block()	&&
    //			(*it).isFunction()			)
    if ((*it).name() == sym.name() &&
        (*it).scope() == sym.scope()) {
      std::cout << "===================================================================================================================================================RELOADING SYMBOL!!!!!" << std::endl;
    }
  }

  m_symlist.push_back(sym);
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
    if ((*it).isFunction()) {
      std::cout << "checking file=" << (*it).file() << " function=" << (*it).name() << std::endl;
      if ((*it).name() == function) {
        addr = (*it).addr();
        endaddr = (*it).endAddr();
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
    if ((*it).isFunction()) {
      if ((*it).name() == function) {
        addr = (*it).addr();
        endaddr = (*it).endAddr();
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

bool SymTab::add_function_file_entry(std::string file_name, std::string func_name,
                                     int line_num, uint16_t addr) {
#if 0
	FUNC_ENTRY ent;
	std::cout << "############# adding new function ########################" << std::endl;
	if( file_id(file_name)==-1 )
		file_map.push_back(file_name);
	
	ent.file_id		= file_id(file_name);
	ent.name		= func_name;
	ent.start_addr	= addr;
	ent.line_num	= line_num;
	func_list.push_back(ent);
	return true;
#endif
  return false;
}

bool SymTab::add_function_file_entry(std::string file_name, int line_num,
                                     uint16_t addr) {
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
#if 0
	for(it=func_list.begin(); it!=func_list.end(); ++it)
	{
		printf("%-20s  %-20s  0x%08x  0x%08x\n",
			   file_name( (*it).file_id ).c_str(),
			   (*it).name.c_str(),
			   (*it).start_addr,
			   (*it).end_addr
			  );
	}
#else
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).isFunction()) {
      printf("%-20s  %-20s  0x%08x  0x%08x\n",
             (*it).file().c_str(),
             (*it).name().c_str(),
             (*it).addr(),
             (*it).endAddr());
    }
  }
#endif
}

bool SymTab::compare(Symbol &sym1, Symbol &sym2) {
  if (sym1.scope() == sym2.scope() &&
      sym1.name() == sym2.name() &&
      sym1.level() == sym2.level() &&
      sym1.block() == sym2.block())
    return true;
  else
    return false;
}

// @FIXME: needs to check for function name match for local scope!
Symbol *SymTab::getSymbol(Symbol sym) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if (compare((*it), sym))
      return &(*it);
  }
  m_symlist.push_back(sym);
  return getSymbol(sym);
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
    if ((*it).isFunction()) {
      if (addr >= (*it).addr() && addr <= (*it).endAddr()) {
        func = (*it).name();

        return true;
      }
      //			printf("%-20s  %-20s  0x%08x  0x%08x\n",
      //				   (*it).file().c_str(),
      //				   (*it).name().c_str(),
      //				   (*it).addr(),
      //				   (*it).endAddr()
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

#if 0	
	SYMLIST::iterator it;
	for( it=m_symlist.begin(); it!=m_symlist.end(); ++it )
	{
		if( file_name((*it).file_id).c_str()==file && (*it).line()==line )
		{
			std::cout <<"MATCH MATCH MATCH MATCH MATCH"<<std::endl;
			block = (*it).block();
			level = (*it).level();
			std::cout << "(*it).block()="<< (*it).block()<<std::endl;
			return true;
		}
	}
	return false;	// not found
#endif
}
#include <assert.h>

/// @FIXME dosen't work flat vs normal address issue
std::string SymTab::get_symbol_name(FLAT_ADDR addr) {
  SYMLIST::iterator it;
  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    if ((*it).addr() == addr)
      return (*it).name();
  }
  return "NOT IMPLEMENTED";
}

/// @FIXME dosen't work flat vs normal address issue
/// DO we need to look at scope here also? would need an extra parameter
std::string SymTab::get_symbol_name_closest(FLAT_ADDR flat_addr) {
  ADDR closest = 0;
  SYMLIST::iterator it;
  SYMLIST::iterator close_it = m_symlist.begin();
  std::cout << "size of list = " << m_symlist.size() << std::endl;

  for (it = m_symlist.begin(); it != m_symlist.end(); ++it) {
    printf("sym_addr = 0x%08x, addr = 0x%08x, '%s'\n", (*it).flat_start_addr(), flat_addr, (*it).name().c_str());
    if ((*it).flat_start_addr() == flat_addr)
      return (*it).name();
    else if ((*it).flat_start_addr() < flat_addr &&
             (*it).flat_start_addr() > closest) {
      std::cout << "closest = " << std::hex << closest << std::endl;
      closest = (*it).addr();
      close_it = it;
    }
  }
  return (*close_it).name();
}
