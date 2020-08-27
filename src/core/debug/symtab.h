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
#ifndef SYMTAB_H
#define SYMTAB_H

#include <list>
#include <vector>

#include "contextmgr.h"
#include "memremap.h"
#include "symbol.h"
#include "types.h"

/**	Symbol tables for ec2cdb
	Consists of:
		- main symbol table
		- file / line number to address mapping
	
	@author Ricky White <ricky@localhost.localdomain>
*/
class SymTab {
public:
  SymTab(DbgSession *session);
  ~SymTab();
  typedef std::list<Symbol> SYMLIST;

  /** clear all tables, get read for the load of a new cdb file
	*/
  void clear();

  std::vector<Symbol *> get_symbols(ContextMgr::Context context);

  Symbol *add_symbol(const symbol_scope &scope, const symbol_identifier &ident);

  Symbol *get_symbol(const symbol_scope &scope, const symbol_identifier &ident);
  Symbol *get_symbol(const ContextMgr::Context &ctx, const std::string &name);
  Symbol *get_symbol(const int32_t hash);

  /** get a symbol given its location in memory.
		Exact matches only.
		\param addr	Address to look for symbol at
	*/
  std::string get_symbol_name(FLAT_ADDR addr);

  /** get a symbol given its location in memory.
		\param addr	Address to look for symbol at
		\returns the name of the sysbol at the address or the closest preceeding symbol name.
	*/
  std::string get_symbol_name_closest(FLAT_ADDR addr);

  void dump();
  void dump_symbols();
  void dump_c_lines();
  void dump_asm_lines();
  void dump_functions();

  ///////////////////////////////////////////////////////////////////////////
  // address from functions
  ///////////////////////////////////////////////////////////////////////////
  /** Get the address that relates to a specific line number in a file
		\returns >=0 address, -1 = failure
	*/
  int32_t get_addr(std::string file, int line_num);
  /** Get the address of the start of a function in a file
		\returns >=0 address, -1 = failure
	 */
  bool get_addr(std::string file, std::string function, int32_t &addr, int32_t &endaddr);

  /** Get the address of the start of a function (any file)
		\returns >=0 address, -1 = failure
	 */
  int32_t get_addr(std::string function);
  bool get_addr(std::string function, int32_t &addr, int32_t &endaddr);

  ///////////////////////////////////////////////////////////////////////////
  // Find file location based on address
  ///////////////////////////////////////////////////////////////////////////
  //bool find_c_file_line( uint16_t addr, std::string &file, int &line_num );
  bool find_c_file_line(ADDR addr, std::string &file, LINE_NUM &line_num);
  bool find_asm_file_line(uint16_t addr, std::string &file, int &line_num);

  ///////////////////////////////////////////////////////////////////////////
  // Adding file entries
  ///////////////////////////////////////////////////////////////////////////
  bool add_c_file_entry(std::string path, int line_num, int level, int block, uint16_t addr);
  bool add_asm_file_entry(std::string path, int line_num, uint16_t addr);

  ///////////////////////////////////////////////////////////////////////////
  // reverse lookups from address.
  ///////////////////////////////////////////////////////////////////////////
  bool get_c_line(ADDR addr, std::string &module, LINE_NUM &line);
  bool get_c_function(ADDR addr, std::string &file, std::string &func);
  bool get_c_block_level(std::string file, LINE_NUM line, BLOCK &block, LEVEL &level);

protected:
  SYMLIST m_symlist;
  typedef struct
  {
    int file_id;
    std::string function;
    int line_num;
    int level;
    int block;
    int16_t addr;
  } FILE_ENTRY;

  typedef std::list<FILE_ENTRY> FILE_LIST;
  FILE_LIST c_file_list;
  FILE_LIST asm_file_list;

  typedef std::vector<std::string> FILE_VEC;
  FILE_VEC file_map;
  int file_id(std::string filename);
  std::string file_name(int id);

  typedef struct
  {
    std::string name;
    int file_id;
    int32_t start_addr;
    int32_t end_addr;
    int line_num;
  } FUNC_ENTRY;
  typedef std::list<FUNC_ENTRY> FUNC_LIST;
  //	FUNC_LIST	func_list;
  DbgSession *mSession;
};

#endif
