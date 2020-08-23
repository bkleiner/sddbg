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
    : mSession(session) {
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

  std::string line;
  while (!in.eof()) {
    getline(in, line);
    parse_record(line);
  }
  in.close();
  return true;
}

bool CdbFile::parse_record(std::string line) {
  if (line[1] != ':')
    return false;

  Symbol sym(mSession);
  std::string tmp;
  Symbol *pSym;

  int pos = 0, npos = 0;
  switch (line[pos++]) {
  case 'M':
    pos++;
    cur_module = line.substr(2);
    break;
  case 'F':
    // <F><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
    // <$><Name><$><Level><$><Block><(><TypeRecord><)><,><AddressSpace>
    // <,><OnStack><,><Stack><,><Interrupt><,><Interrupt Num>
    // <,><Register Bank>
    pos++; // skip ':'
    parse_scope_name(line, sym, pos);
    pos++;
    npos = line.find('$', pos);
    sym.setLevel(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
    pos = npos + 1;
    npos = line.find('(', pos);
    sym.setBlock(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
    pos = npos;
    pos++;

    // check if it already exsists
    pSym = mSession->symtab()->getSymbol(sym);

    parse_type_chain_record(line, *pSym, pos);
    pos++;
    pSym->setAddrSpace(line[pos]);
    pos += 2;
    pos += 2;
    npos = line.find(',', pos);
    //			std::cout <<"stack = "<<line.substr(pos,npos-pos)<<std::endl;
    pos = npos; //','
    pos++;
    // <Interrupt><,><Interrupt Num><,><Register Bank>
    npos = line.find(',', pos);
    pSym->set_interrupt(line.substr(pos, npos - pos) == "1");
    //			std::cout <<"Interrupt = "<<sym.is_int_handler()<<std::endl;
    pos = npos; //','
    pos++;
    npos = line.find(',', pos);
    pSym->set_interrupt_num(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
    //			std::cout <<"Interrupt number = "<<sym.interrupt_num()<<std::endl;
    pos = npos; //','
    pos++;
    npos = line.length();
    pSym->set_reg_bank(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
    //			std::cout <<"register bank = "<<sym.reg_bank()<<std::endl;
    pSym->setIsFunction(true);
    pSym->setFile(cur_module + ".c");
    // not needed now its part of the normal symbol table m_symtab->add_function_file_entry(pSym->file(),pSym->name(),pSym->line(),pSym->addr());
    break;
  case 'S':
    // <S><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
    // <$><Name><$><Level><$><Block><(><TypeRecord><)>
    // <,><AddressSpace><,><OnStack><,><Stack><,><[><Reg><,>{<Reg><,>}<]>
    pos++; // skip ':'
    //std::cout <<"%1%";
    parse_scope_name(line, sym, pos);
    //std::cout <<"%2%";
    pos++;
    npos = line.find('$', pos);
    //			std::cout <<"level["<<line.substr( pos, npos-pos )<<"]"<<std::endl;
    sym.setLevel(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
    pos = npos + 1;
    npos = line.find('(', pos);
    //			std::cout <<"block["<<line.substr( pos, npos-pos )<<"]"<<std::endl;
    sym.setBlock(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
    pos = npos;
    //			std::cout <<"level="<<sym.level()<<", block="<<sym.block()<<std::endl;
    //			std::cout <<"at pos = "<<line[pos]<<std::endl;
    pos++;

    // check if it already exsists
    pSym = mSession->symtab()->getSymbol(sym);

    //std::cout <<"%3%";
    parse_type_chain_record(line, *pSym, pos);
    //std::cout <<"%4%";
    pos++; // skip ','
           //			std::cout <<"["<<line.substr(pos)<<"]"<<std::endl;
           //			std::cout <<"addr space = "<<line[pos]<<std::endl;
    pSym->setAddrSpace(line[pos]);
    pos += 2;
    //			std::cout <<"on stack = "<<line[pos]<<std::endl;
    pos += 2;
    npos = line.find(',', pos);
    //			std::cout <<"stack = "<<line.substr(pos,npos-pos)<<std::endl;
    pos = npos; //','
    pos++;
    //			std::cout <<"line[pos] = "<<line[pos]<<std::endl;
    if (line[pos] == '[') {
      // looks like there are some registers
      pos++; // skip '['
      do {
        npos = line.find(',', pos);
        if (npos == -1)
          npos = line.find(']', pos);
        //					std::cout <<"reg="<<line.substr(pos,npos-pos)<<std::endl;
        pSym->addReg(line.substr(pos, npos - pos));
        pos = npos + 1;
      } while (line[npos] != ']');
    }
    //			std::cout <<"DONE"<<std::endl;
    // 			m_symtab->addSymbol( sym );	// @FIXME: shoulden't do this if the symbol aready exsists such as a function
    break;
  case 'T':
    parse_type(line);
    break;
  case 'L':
    parse_linker(line);
    break;
  default:
    //			std::cout << "unsupported record type '"<<line[0]<<"'"<<std::endl;
    break;
  }
  return true;
}

int CdbFile::parse_type_chain_record(std::string s) {
  int pos = 0, npos = 0;
  std::cout << "parse_type_record( \"" << s << "\" )" << std::endl;
  int size;
  char *endptr;

  if (s[0] != '(')
    return -1; // failure

  pos = s.find('{', 0) + 1;
  npos = s.find('}', pos);
  std::stringstream m(s.substr(pos, npos - pos));
  if (!(m >> size))
    return -1; // bad format
  std::cout << "size = " << size << std::endl;

  std::string DCLtype;
  pos = npos + 1;
  // loop through grabbing <DCLType> until we hit a ':'
  int limit = s.find(':', pos);
  while (npos < limit) {
    pos = npos + 1;
    npos = s.find(',', pos);
    npos = (npos > limit) ? limit : npos;
    std::cout << "DCLTYPE = [" << s.substr(pos, npos - pos) << "]" << std::endl;
  }
  if (s[npos++] != ':')
    return -1; // failure
               //	std::cout <<"Signed = "<<s[npos]<<std::endl;
  npos++;
  if (s[npos++] != ')')
    return -1; // failure
  return npos;

  //	std::cout << "DCLTYPE = ["<<s.substr(pos,npos-pos)<<"]"<<std::endl;

  // pull apart DCL typeCdbFile
  //	if(
}

bool CdbFile::parse_type_chain_record(std::string line, Symbol &sym, int &pos) {
  int npos;
  std::cout << "parse_type_chain_record( \"" << line << "\", sym, " << pos << " )" << std::endl;
  int size;
  char *endptr;

  pos = line.find('{', 0) + 1;
  npos = line.find('}', pos);
  std::stringstream m(line.substr(pos, npos - pos));
  if (!(m >> size))
    return false; // bad format
                  //	std::cout <<"size = "<<size<<std::endl;
  sym.setLength(size);

  std::string DCLtype;
  pos = npos + 1;
  // loop through grabbing <DCLType> until we hit a ':'
  int limit = line.find(':', pos);

  char type_char;
  std::string type_name = "";
  int cnt = 0;

  // The last loop will be followed by a sign type if an integer type
  while (npos < limit) {
    pos = npos + 1;
    npos = line.find(',', pos);
    npos = (npos > limit) ? limit : npos;
    std::cout << "DCLTYPE = [" << line.substr(pos, npos - pos) << "]" << std::endl;

    // which type and sign
    if (line[pos] == 'D') {
      if (line[pos + 1] == 'F') {
        // enter function symbol declaration mode...
        std::cout << "FUNCTION :";
        sym.setIsFunction(true);
        // need to have a list of parameters and push then back or similar.
        // a function symbol is a bit special
        // also needs to set a retun type
      } else if (line[pos + 1] == 'A') {
        std::cout << "================= ARRAY =================" << std::endl;
        // DAxxx,
        // where xxx is the number of elements
        npos = line.find(',', pos + 2);
        if (npos == -1)
          std::cerr << "BAD CDB FILE format" << std::endl;
        else {
          //std::cout << "substr = ["<<line.substr(pos+2,npos-pos-2)<<"]"<<std::endl;
          int c = strtoul(line.substr(pos + 2, npos - pos - 2).c_str(), 0, 10);
          sym.AddArrayDim(c);
        }
      }
    } else if (line[pos] == 'S') {
      type_char = line[pos + 1];
      switch (line[pos + 1]) {
      case 'T': // typedef
        type_name = line.substr(pos + 2, npos - pos - 2);
        break;
      }
    }
    cnt++;
  }
  if (line[npos++] != ':')
    return false; // failure

  bool issigned = line[npos] == 'S';
  switch (type_char) {
  case 'T':
    break;
  case 'C':
    type_name = issigned ? "char" : "unsigned char";
    break;
  case 'S':
    type_name = issigned ? "short" : "unsigned short";
    break;
  case 'I':
    type_name = issigned ? "int" : "unsigned int";
    break;
  case 'L':
    type_name = issigned ? "long" : "unsigned long";
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
    type_name = "bitfield of n bits???";
    break;
  default:
    std::cerr << "ERROR unhandled type" << std::endl;
    assert(1 == 0);
  }

  if (type_name != "") {
    std::cout << type_name << std::endl;
    if (sym.isFunction())
      sym.setReturn(type_name);
    else
      sym.setType(type_name);
  }

  npos++;
  if (line[npos++] != ')')
    return false; // failure
  pos = npos;
  //	std::cout <<"DONE DONE"<<std::endl;
  return true;
}

/** parse a link record
	<pre>
	Format:
	<L><:>{ <G> | F<filename> | L<function> }
	<$><name>
	<$><level>
	<$><block>
	<:><address> 
	</pre>
	
	
*/
bool CdbFile::parse_linker(std::string line) {
  //	std::cout <<"parsing linker record \""<<line<<"\""<<std::endl;
  int pos, npos;
  std::string filename;
  Symbol sym(mSession), *pSym;
  SymTab::SYMLIST::iterator it;

  pos = 2;
  // <L><:>{ <G> | F<filename> | L<function> }<$><name>
  // <$><level><$><block><:><address>
  switch (line[pos++]) {
  case 'G': // Global
  case 'F': // File
  case 'L': // Function
    // <L><:>{ <G> | F<filename> | L<function> }<$><name>
    // <$><level><$><block><:><address>
    pos--; // parse_scope needs to see G F or L
    parse_scope_name(line, sym, pos);
  case '$': // fallthrough
    pos++;  // this seems necessary for local function vars, what about the rest?
    parse_level_block_addr(line, sym, pos, true);
    //			printf("addr=0x%08x\n",sym.addr());
    pSym = mSession->symtab()->getSymbol(sym);
    pSym->setAddr(sym.addr());
    //			std::cout << "??linker record"<<std::endl;
    //			std::cout << "\tscope = "<<pSym->scope()<<std::endl;
    //			std::cout << "\tname = "<<pSym->name()<<std::endl;
    //			std::cout << "\tlevel = "<<pSym->level()<<std::endl;
    //			std::cout << "\tblock = "<<pSym->block()<<std::endl;

    break;
  case 'A':
    // Linker assembly line record
    // <L><:><A><$><Filename><$><Line><:><EndAddress>
    //			std::cout <<"Assembly line record"<<std::endl;
    if (line[pos++] != '$')
      return false;
    // grab the filename
    npos = line.find('$', pos);
    sym.setFile(line.substr(pos, npos - pos));
    pos = npos + 1;
    // line
    npos = line.find(':', pos);
    sym.setLine(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
    pos = npos + 1;
    npos = line.length();
    // @FIXME: there is some confusion over the end address / start address thing
    //			std::cout <<"??endaddr= ["<<line.substr(pos,npos-pos)<<"]"<<std::endl;
    sym.setAddr(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
    mSession->symtab()->add_asm_file_entry(sym.file(),
                                           sym.line(),
                                           sym.addr());
    break;
  case 'C':
    // Linker C record
    // This isn't a symbol in the normal sense,  we will use a separate table for crecords as thaaea are line to c code mappings
    // <L><:><C><$><Filename><$><Line><$><Level><$><Block><:><EndAddress>
    if (line[pos++] != '$')
      return false;
    // grab the filename
    npos = line.find('$', pos);
    sym.setFile(line.substr(pos, npos - pos));
    //			std::cout << "test filemane = "<<sym.file()<<std::endl;
    pos = npos + 1;
    npos = line.find('$', pos);
    sym.setLine(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
    pos = npos + 1;
    parse_level_block_addr(line, sym, pos, true);

    //			std::cout << "C linker record"<<std::endl;
    //			std::cout << "\tscope = "<<sym.scope()<<std::endl;
    //			std::cout << "\tname = "<<sym.name()<<std::endl;
    //			std::cout << "\tlevel = "<<sym.level()<<std::endl;
    //			std::cout << "\tblock = "<<sym.block()<<std::endl;
    // @FIXME: need to handle block
    mSession->symtab()->add_c_file_entry(sym.file(),
                                         sym.line(),
                                         sym.level(),
                                         sym.block(),
                                         sym.addr());
    break;
  case 'X':
    // linker symbol end address record
    // <L><:><X>{ <G> | F<filename> | L<functionName> }
    // <$><name><$><level><$><block><:><Address>
    //			std::cout <<"linker symbol end address record"<<std::endl;
    parse_scope_name(line, sym, pos);
    parse_level_block_addr(line, sym, pos, false);

    pSym = mSession->symtab()->getSymbol(sym);
    //			pSym->setAddr( sym.addr() );
    // The Linker Symbol end address record is primarily used to
    // indicate the Ending address of functions. This is because
    // function records do not contain a size value, as symbol records do.
    // need to find and modify the origional symbol

    //SymTab::SYMLIST::iterator it;
    //it = m_symtab->getSymbol(sym.file(),sym.scope(),sym.name());
    //m_symtab->getSymbol(sym.file(),sym.scope(),sym.name(),it);
    // @FIXME: need to deal with not found error
    //it->dump();
    pSym->setEndAddr(sym.endAddr());
    break;
  }

  //	npos = line.find( 'pos

  //	m_symtab->addSymbol( sym );
  return true;
}

bool CdbFile::parse_level_block_addr(std::string line, Symbol &sym, int &pos, bool bStartAddr) {
  int npos;

  // level
  npos = line.find('$', pos);
  sym.setLevel(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
  pos = npos + 1;
  // block
  npos = line.find(':', pos);
  sym.setBlock(strtoul(line.substr(pos, npos - pos).c_str(), 0, 10));
  pos = npos + 1;
  npos = line.length();

  if (bStartAddr)
    sym.setAddr(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
  else
    sym.setEndAddr(strtoul(line.substr(pos, npos - pos).c_str(), 0, 16));
  return line.length();
}

// parse { <G> | F<filename> | L<function> }<$><name>
bool CdbFile::parse_scope_name(std::string data, Symbol &sym, int &pos) {
  int npos;
  //	std::cout <<"int CdbFile::parse_scope_name( "<<data<<", &sym, "<<pos<<" )"<<std::endl;
  switch (data[pos++]) {
  case 'G':
    pos++; // skip $
    sym.setScope(Symbol::SCOPE_GLOBAL);
    npos = data.find('$', pos);
    sym.setName(data.substr(pos, npos - pos));
    pos = npos;
    break;
  case 'F':
    sym.setScope(Symbol::SCOPE_FILE);
    npos = data.find('$', pos);
    sym.setFile(data.substr(pos, npos - pos));
    pos = npos + 1; // +1 = skip '$'
    sym.setName(data.substr(pos, npos - pos));
    pos = npos;
    break;
  case 'L':
    sym.setScope(Symbol::SCOPE_LOCAL);
    npos = data.find('$', pos);
    sym.setFunction(data.substr(pos, npos - pos));
    pos = npos + 1; // +1 = skip '$'
    npos = data.find('$', pos);
    sym.setName(data.substr(pos, npos - pos));
    pos = npos;
    break;
  default:
    // optional section not matched
    return false;
  }
  return true;
}

/** Parse a type record and load into internal data structures.
	\param line std::string of the line from the file containing the type record.
*/
bool CdbFile::parse_type(std::string line) {
  std::cout << "Type record [" << line << "]" << std::endl;
  std::cout << "-----------------------------------------------------------" << std::endl;
  int epos, spos;
  std::string file, name;
  spos = 2;
  epos = 2;
  if (line[spos++] == 'F') {
    // pull out the file name
    epos = line.find('$', spos);
    file = line.substr(spos, epos - spos);
    spos = epos + 1;
    epos = line.find('[', spos);
    name = line.substr(spos, epos - spos);
    std::cout << "File = '" << file << "'" << std::endl;
    std::cout << "Name = '" << name << "'" << std::endl;
    spos = epos + 1;
    std::cout << "line[spos] = '" << line[spos] << "'" << std::endl;

    SymTypeStruct *t = new SymTypeStruct(mSession);
    t->set_name(name);
    t->set_file(file);
    while (line[spos] == '(') {
      parse_type_member(line, spos, t);
    }
    mSession->symtree()->add_type(t);
  }
  std::cout << "-----------------------------------------------------------" << std::endl;
  return false; // failure
}

/** Parse a type member record that in s.
	\param line line containing the type member definition
	\param spos start position in line of the type member to parse, received the position after the record on return
	\returns success=true, failure = false
*/
bool CdbFile::parse_type_member(std::string line, int &spos, SymTypeStruct *t) {
  size_t epos;
  std::cout << "part line '" << line.substr(spos) << "'" << std::endl;
  if (line[spos++] != '(')
    return false;

  // Offset
  int offset;
  if (line[spos++] == '{') {
    epos = line.find('}', spos);
    if (epos == -1)
      return false; // failure

    offset = strtoul(line.substr(spos, epos - spos).c_str(), 0, 0);
    std::cout << "offset = " << offset << std::endl;
    std::cout << "spos = " << spos << ", epos = " << epos << std::endl;
    spos = epos + 1;

    if (!parse_symbol_record(line, spos, t))
      return false;
  }
  return true;
}

/** parse a symbol record starting at spos in the supplied line
	({0}S:S$pNext$0$0({3}DG,STTTinyBuffer:S),Z,0,0)
	@TODO should a parameter to recieve the symbol by referance be added?

	Only called in the type parsing code
	@TODO change the name of this function to reflect the above.
*/
bool CdbFile::parse_symbol_record(std::string line, int &spos, SymTypeStruct *t) {
  size_t epos, tmp[2];
  Symbol sym(mSession);
  std::string name;

  std::cout << "^" << line.substr(spos) << "^" << std::endl;
  if (line.substr(spos, 2) != "S:") {
    std::cout << "symbol start not found!" << std::endl;
    return false; // failure
  }
  spos += 2;
  epos = spos;
  // Scope
  switch (line[spos++]) {
  case 'G': // Global scope
    std::cout << "Scope = Global" << std::endl;
    break;
  case 'F': // File scope
    epos = line.find('$', spos);
    std::cout << "Scope = File '" << line.substr(spos, epos - spos) << "'" << std::endl;
    break;
  case 'L': // Function scope
    epos = line.find('$', spos);
    std::cout << "Scope = Function '" << line.substr(spos, epos - spos) << "'" << std::endl;
    break;
  case 'S': // Symbol definition (part of type record)
    spos++;
    epos = line.find('$', spos);
    std::cout << "Scope = type symbol record" << std::endl;
    name = line.substr(spos, epos - spos);
    std::cout << "\tName = '" << name << "'" << std::endl;

    spos = epos + 1;
    epos = line.find('$', spos);
    std::cout << "\tLevel = '" << line.substr(spos, epos - spos) << "'" << std::endl;
    spos = epos + 1;
    epos = line.find('(', spos);
    std::cout << "\tBlock = '" << line.substr(spos, epos - spos) << "'" << std::endl;

    // ({2}SI:S)
    // ({16}DA16,SC:S)
    // etc
    spos = epos + 1;

    // get size
    spos = line.find('{', spos) + 1;
    epos = line.find('}', spos);
    std::cout << "size = " << line.substr(spos, epos - spos) << std::endl;
    spos = epos + 1;
    epos = line.find(')', spos);

    std::cout << "interesting part='" << line.substr(spos, epos - spos) << "'" << std::endl;

    parse_struct_member_dcl(line, spos, name, t);

    //			spos = line.find(')',spos);	// skip over type record for now!!!
    spos = epos;
    spos++;
    //

    if (line[spos] != ',')
      return false;
    spos++;
    std::cout << "Address space = '" << line[spos] << "'" << std::endl;
    spos += 2;
    std::cout << "On stack '" << line[spos] << "'" << std::endl;
    spos += 2;
    tmp[0] = line.find(',', spos);
    tmp[1] = line.find(')', spos);
    //epos = tmp[0]<?tmp[1];
    epos = MIN(tmp[0], tmp[1]);
    std::cout << "Stack '" << line.substr(spos, epos - spos) << "'" << std::endl;
    if (line[epos] != ')') {
      // now the registers...
      // registers follow, ',' separated but ends when a ')' is encountered.
      while (1) {
        spos = epos + 1;
        tmp[0] = line.find(',', spos);
        tmp[1] = line.find(')', spos);
        epos = MIN(tmp[0], tmp[1]);
        std::cout << "Register '" << line.substr(spos, epos - spos) << "'";
        if (line[epos] == ')')
          break; // done
      }
    }
    break;
  default:
    return false; // failure
  }
  spos = epos;
  //	std::cout <<"%"<<line[spos-1]<<"%"<<line[spos]<<"%"<<std::endl;
  return line[spos++] == ')';
}

/** Parse a DCL type record that is part of a struct member
	updated the type with the information.
*/
bool CdbFile::parse_struct_member_dcl(std::string line,
                                      int &spos,
                                      std::string name,
                                      SymTypeStruct *t) {
  int epos;
  std::string s = line.substr(spos, 2);
  int32_t array_element_cnt = 1; ///< default to 1 (not an array)
  typedef enum { SP_NORM,
                 SP_ARRAY,
                 SP_BITFIELD } SPECIAL;

  SPECIAL special;

  // DCLTypes with secondary components done first.
  if (s == "DA") {
    // Array of n elements
    spos += 2;
    epos = line.find(',', spos);
    array_element_cnt = strtoul(line.substr(spos, epos - spos).c_str(), 0, 10);
    std::cout << "Array of " << array_element_cnt << " elements" << std::endl;
    spos = epos + 1;
    epos = line.find(')', spos);
    std::cout << "***[" << line.substr(spos, epos - spos) << "]****" << std::endl;
    s = line.substr(spos, 2);
    special = SP_ARRAY;
  } else if (s == "SB") {
    std::cout << "Bit field of <n> bits" << std::endl;
    special = SP_BITFIELD;
  } else
    special = SP_NORM;

  if (s == "DF") {
    std::cout << "Function" << std::endl;
  } else if (s == "DG") {
    std::cout << "Generic pointer" << std::endl;
  } else if (s == "DC") {
    std::cout << "Code pointer" << std::endl;
  } else if (s == "DX") {
    std::cout << "External ram pointer" << std::endl;
  } else if (s == "DD") {
    std::cout << "Internal ram pointer" << std::endl;
  } else if (s == "DP") {
    std::cout << "Paged pointer" << std::endl;
  } else if (s == "DI") {
    std::cout << "Upper 128 byte pointer" << std::endl;
  } else if (s == "SL") {
    std::cout << "long" << std::endl;
    spos += 3; // skip "SL:"
    if (line[spos] == 'S') {
      t->add_member(name, "long", array_element_cnt);
    } else if (line[spos] == 'U') {
      t->add_member(name, "unsigned long", array_element_cnt);
    } else
      std::cout << "ERROR invalid signedness";
  } else if (s == "SI") {
    std::cout << "int" << std::endl;
    spos += 3; // skip "SI:"
    if (line[spos] == 'S') {
      t->add_member(name, "int", array_element_cnt);
    } else if (line[spos] == 'U') {
      t->add_member(name, "unsigned int", array_element_cnt);
    } else
      std::cout << "ERROR invalid signedness";
  } else if (s == "SC") {
    spos += 3; // skip "SI:"
    if (line[spos] == 'S') {
      t->add_member(name, "char", array_element_cnt);
    } else if (line[spos] == 'U') {
      t->add_member(name, "unsigned char", array_element_cnt);
    } else
      std::cout << "ERROR invalid signnedness";
  } else if (s == "SS") {
    spos += 3; // skip "SI:"
    if (line[spos] == 'S') {
      t->add_member(name, "short", array_element_cnt);
    } else if (line[spos] == 'U') {
      t->add_member(name, "unsigned short", array_element_cnt);
    } else
      std::cout << "ERROR invalid signnedness";
  } else if (s == "SV") {
    std::cout << "void" << std::endl;
  } else if (s == "SF") {
    t->add_member(name, "float", array_element_cnt);
  } else if (s == "ST") {
    std::string sname;
    spos += 2; // skip "ST"
    epos = line.find(':', spos);
    sname = line.substr(spos, epos - spos);
    std::cout << "Structure named '" << name << "," << sname << "'" << std::endl;
    t->add_member(name, sname, array_element_cnt);
  } else if (s == "SX") {
    std::cout << "sbit" << std::endl;
    t->add_member(name, "sbit", array_element_cnt);
  } else {
    std::cout << "Error invalid DCL type!" << std::endl;
    return false;
  }
  return true;
}
