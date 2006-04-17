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
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include "cdbfile.h"
#include "symbol.h"
#include "module.h"
using namespace std;

CdbFile::CdbFile( SymTab *stab )
{
	m_symtab = stab;
}


CdbFile::~CdbFile()
{
}

bool CdbFile::open( string filename )
{
#if 0
	// test code
	ModuleMgr mmgr;
	Module &m = mmgr.add_module("test");
	m.load_c_file("/home/ricky/projects/ec2drv/debug/src/newcdb/test.c");	// test code only
	cout << " file '"<<m.get_c_file_name()<<"'"<<endl;
	m.load_asm_file("/home/ricky/projects/ec2drv/debug/src/newcdb/test.asm");	//
	mmgr.add_module("crap");
	mmgr.add_module("junk");
	mmgr.dump();
#endif
	
	
	cout << "Loading "<<filename<<endl;
	
	ifstream in;
	string line;
	int i=0;
	
	in.open( filename.c_str() );
	if( in.is_open() )
	{
		while( !in.eof() )
		{
			getline( in, line );
//			cout <<"Line "<<i<<" : "<<line<<endl;
			parse_record( line );
		//	m_symtab->dump();
			i++;
		}
		in.close();
	}
	else
		return false;	// failed to open file
	cout << "module dump:"<<endl;
	mod_mgr.dump();
}

bool CdbFile::parse_record( string line )
{
	int pos=0, npos=0;

	if( line[1]!=':' )
		return false;	// invalid record
	Symbol sym;
	string tmp;
	Symbol *pSym;
	switch( line[pos++] )
	{
		case 'M' :
			pos++;
			cur_module = line.substr(2);//,line.length()-2);
//			cout <<"module '"<<cur_module<<"'"<<endl;
			break;
		case 'F' :
			// <F><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
			// <$><Name><$><Level><$><Block><(><TypeRecord><)><,><AddressSpace>
			// <,><OnStack><,><Stack><,><Interrupt><,><Interrupt Num>
			// <,><Register Bank>
			pos++;	// skip ':'
			parse_scope_name( line, sym, pos );
			pos++;
			npos = line.find('$',pos);
			sym.setLevel( strtoul( line.substr( pos, npos-pos ).c_str(),0,16) );
			pos = npos+1;
			npos = line.find('(',pos);
			sym.setBlock( strtoul( line.substr( pos, npos-pos ).c_str(),0,16) );
			pos = npos;
//			cout <<"level="<<sym.level()<<", block="<<sym.block()<<endl;
//			cout <<"at pos = "<<line[pos]<<endl;
			pos++;
			
			// check if it already exsists
			pSym = symtab.getSymbol( sym );

			
			parse_type_chain_record( line, *pSym, pos ); 
			pos++;	// skip ','
//			cout <<"addr space = "<<line[pos]<<endl;
			pSym->setAddrSpace( line[pos] );
			pos+=2;
//			cout <<"on stack = "<<line[pos]<<endl;
			pos+=2;
			npos = line.find(',',pos);
//			cout <<"stack = "<<line.substr(pos,npos-pos)<<endl;
			pos = npos;		//','
			pos++;
			// <Interrupt><,><Interrupt Num><,><Register Bank>
			npos = line.find(',',pos);
			pSym->set_interrupt( line.substr(pos,npos-pos)=="1" );
//			cout <<"Interrupt = "<<sym.is_int_handler()<<endl;
			pos = npos;		//','
			pos++;
			npos = line.find(',',pos);
			pSym->set_interrupt_num( strtoul( line.substr(pos,npos-pos).c_str(),0,10 ) );
//			cout <<"Interrupt number = "<<sym.interrupt_num()<<endl;
			pos = npos;		//','
			pos++;
			npos = line.length();
			pSym->set_reg_bank( strtoul( line.substr(pos,npos-pos).c_str(),0,10 ) );
//			cout <<"register bank = "<<sym.reg_bank()<<endl;
			pSym->setIsFunction( true );
			pSym->setFile( cur_module+".c" );
			// not needed now its part of the normal symbol table m_symtab->add_function_file_entry(pSym->file(),pSym->name(),pSym->line(),pSym->addr());
			break;
		case 'S' :
			// <S><:>{ G | F<Filename> | L { <function> | ``-null-`` }}
			// <$><Name><$><Level><$><Block><(><TypeRecord><)>
			// <,><AddressSpace><,><OnStack><,><Stack><,><[><Reg><,>{<Reg><,>}<]> 
			pos++;	// skip ':'
			parse_scope_name( line, sym, pos );
			pos++;
			npos = line.find('$',pos);
//			cout <<"level["<<line.substr( pos, npos-pos )<<"]"<<endl;
			sym.setLevel( strtoul( line.substr( pos, npos-pos ).c_str(),0,16) );
			pos = npos+1;
			npos = line.find('(',pos);
//			cout <<"block["<<line.substr( pos, npos-pos )<<"]"<<endl;
			sym.setBlock( strtoul( line.substr( pos, npos-pos ).c_str(),0,16) );
			pos = npos;
//			cout <<"level="<<sym.level()<<", block="<<sym.block()<<endl;
//			cout <<"at pos = "<<line[pos]<<endl;
			pos++;
			
			// check if it already exsists
			pSym = symtab.getSymbol( sym );
			
			
			parse_type_chain_record( line, *pSym, pos ); 
			pos++;	// skip ','
//			cout <<"["<<line.substr(pos)<<"]"<<endl;
//			cout <<"addr space = "<<line[pos]<<endl;
			pSym->setAddrSpace( line[pos] );
			pos+=2;
//			cout <<"on stack = "<<line[pos]<<endl;
			pos+=2;
			npos = line.find(',',pos);
//			cout <<"stack = "<<line.substr(pos,npos-pos)<<endl;
			pos = npos;		//','
			pos++;
//			cout <<"line[pos] = "<<line[pos]<<endl;
			if( line[pos]=='[')
			{
				// looks like there are some registers
				pos++;	// skip '['
				do
				{
					npos = line.find(',',pos);
					if(npos==-1)
						npos = line.find(']',pos);
//					cout <<"reg="<<line.substr(pos,npos-pos)<<endl;
					pSym->addReg( line.substr(pos,npos-pos) );
					pos = npos + 1;
				} while( line[npos]!=']' );
			}
//			cout <<"DONE"<<endl;
// 			m_symtab->addSymbol( sym );	// @FIXME: shoulden't do this if the symbol aready exsists such as a function
			break;
		case 'T' :
//			cout <<"IGNORING structure, must go in a structure table..."<<endl;
			break;
		case 'L' :
			parse_linker( line );
			break;
		default:
//			cout << "unsupported record type '"<<line[0]<<"'"<<endl;
			break;
	}
}


int CdbFile::parse_type_chain_record( string s )
{
	int pos=0, npos=0;
//	cout << "parse_type_record( \""<<s<<"\" )"<<endl;
	int size;
	char *endptr;
	
	if( s[0]!='(' )
		return -1;	// failure
	
	pos = s.find('{',0)+1;
	npos = s.find('}',pos);
	istringstream m(s.substr(pos,npos-pos));
	if( !(m >> size) )
		return -1;	// bad format
//	cout <<"size = "<<size<<endl;
	
	string DCLtype;
	pos = npos + 1;
	// loop through grabbing <DCLType> until we hit a ':'
	int limit = s.find(':',pos);
	while( npos < limit )
	{
		pos = npos + 1;
		npos = s.find(',',pos);
		npos = (npos>limit) ? limit : npos;
//		cout << "DCLTYPE = ["<<s.substr(pos,npos-pos)<<"]"<<endl;
	}
	if(s[npos++]!=':')
		return -1;	// failure
//	cout <<"Signed = "<<s[npos]<<endl;
	npos++;
	if(s[npos++]!=')')
		return -1;	// failure
	return npos;
	
	
//	cout << "DCLTYPE = ["<<s.substr(pos,npos-pos)<<"]"<<endl;
	
	
	
	// pull apart DCL typeCdbFile
//	if( 
	
	
}


bool CdbFile::parse_type_chain_record( string line, Symbol &sym, int &pos  )
{
	int npos;
//	cout << "parse_type_chain_record( \""<<line<<"\" )"<<endl;
	int size;
	char *endptr;
	
	pos = line.find('{',0)+1;
	npos = line.find('}',pos);
	istringstream m(line.substr(pos,npos-pos));
	if( !(m >> size) )
		return false;	// bad format
//	cout <<"size = "<<size<<endl;
	sym.setLength(size);
	
	string DCLtype;
	pos = npos + 1;
	// loop through grabbing <DCLType> until we hit a ':'
	int limit = line.find(':',pos);
	while( npos < limit )
	{
		pos = npos + 1;
		npos = line.find(',',pos);
		npos = (npos>limit) ? limit : npos;
//		cout << "DCLTYPE = ["<<line.substr(pos,npos-pos)<<"]"<<endl;
	}
	if(line[npos++]!=':')
	{
//		cout << "FAIL";
		return false;	// failure
	}
//	cout <<"Signed = "<<line[npos]<<endl;
	npos++;
	if(line[npos++]!=')')
		return false;	// failure
	pos = npos;
//	cout <<"DONE DONE"<<endl;
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
bool CdbFile::parse_linker( string line )
{
//	cout <<"parsing linker record \""<<line<<"\""<<endl;
	int pos,npos;
	string filename;
	Symbol sym, *pSym;
	SymTab::SYMLIST::iterator it;

	pos = 2;	
	// <L><:>{ <G> | F<filename> | L<function> }<$><name>
	// <$><level><$><block><:><address> 	
	switch( line[pos++] )
	{
		case 'G':	// Global
		case 'F':	// File
		case 'L':	// Function
			// <L><:>{ <G> | F<filename> | L<function> }<$><name>
			// <$><level><$><block><:><address>
			pos--;	// parse_scope needs to see G F or L
			parse_scope_name( line, sym, pos );
		case '$':	// fallthrough
			pos++;			// this seems necessary for local function vars, what about the rest?
			parse_level_block_addr( line, sym, pos, true );
//			printf("addr=0x%08x\n",sym.addr());
			pSym = symtab.getSymbol( sym );
			pSym->setAddr(sym.addr());
//			cout << "??linker record"<<endl;
//			cout << "\tscope = "<<pSym->scope()<<endl;
//			cout << "\tname = "<<pSym->name()<<endl;
//			cout << "\tlevel = "<<pSym->level()<<endl;
//			cout << "\tblock = "<<pSym->block()<<endl;
			
			break;
		case 'A':
			// Linker assembly line record
			// <L><:><A><$><Filename><$><Line><:><EndAddress>
//			cout <<"Assembly line record"<<endl;
			if( line[pos++]!='$' )
				return false;
			// grab the filename
			npos = line.find('$',pos);
			sym.setFile( line.substr(pos,npos-pos) );
			pos = npos+1;
			// line
			npos = line.find(':',pos);
			sym.setLine( strtoul(line.substr(pos,npos-pos).c_str(),0,10) );
			pos = npos+1;
			npos = line.length();
			// @FIXME: there is some confusion over the end address / start address thing
//			cout <<"??endaddr= ["<<line.substr(pos,npos-pos)<<"]"<<endl;
			sym.setAddr( strtoul(line.substr(pos,npos-pos).c_str(),0,16) );
			symtab.add_asm_file_entry( sym.file(), sym.line(), sym.addr() );
			break;
		case 'C':
			// Linker C record
			// This isn't a symbol in the normal sense,  we will use a separate table for crecords as thaaea are line to c code mappings
			// <L><:><C><$><Filename><$><Line><$><Level><$><Block><:><EndAddress>
			if( line[pos++]!='$' )
				return false;
			// grab the filename
			npos = line.find('$',pos);
			sym.setFile( line.substr(pos,npos-pos) );
//			cout << "test filemane = "<<sym.file()<<endl;
			pos = npos+1;
			npos = line.find('$',pos);
			sym.setLine( strtoul(line.substr(pos,npos-pos).c_str(),0,10) );
			pos = npos+1;
			parse_level_block_addr( line, sym, pos, true );
			
//			cout << "C linker record"<<endl;
//			cout << "\tscope = "<<sym.scope()<<endl;
//			cout << "\tname = "<<sym.name()<<endl;
//			cout << "\tlevel = "<<sym.level()<<endl;
//			cout << "\tblock = "<<sym.block()<<endl;
			// @FIXME: need to handle block
			symtab.add_c_file_entry( sym.file(), sym.line(), sym.level(), sym.block(), sym.addr() );
			break;
		case 'X':
			// linker symbol end address record
			// <L><:><X>{ <G> | F<filename> | L<functionName> }
			// <$><name><$><level><$><block><:><Address> 
//			cout <<"linker symbol end address record"<<endl;
			parse_scope_name( line, sym, pos );
			parse_level_block_addr( line, sym, pos, false );
			
			pSym = symtab.getSymbol( sym );
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
			pSym->setEndAddr( sym.endAddr() );
			break;
	}
	
	//	npos = line.find( 'pos
	
//	m_symtab->addSymbol( sym );
}

bool CdbFile::parse_level_block_addr( string line, Symbol &sym, int &pos, bool bStartAddr )
{
	int npos;
	
	// level
	npos = line.find('$',pos);
	sym.setLevel( strtoul(line.substr(pos,npos-pos).c_str(),0,10) );
	pos = npos+1;
	// block
	npos = line.find(':',pos);
	sym.setBlock( strtoul(line.substr(pos,npos-pos).c_str(),0,10) );
	pos = npos+1;
	npos = line.length();
	
	if( bStartAddr )
		sym.setAddr( strtoul(line.substr(pos,npos-pos).c_str(),0,16) );
	else
		sym.setEndAddr( strtoul(line.substr(pos,npos-pos).c_str(),0,16) );
	return line.length();
}

// parse { <G> | F<filename> | L<function> }<$><name> 
bool CdbFile::parse_scope_name( string data, Symbol &sym, int &pos )
{
	int npos;
//	cout <<"int CdbFile::parse_scope_name( "<<data<<", &sym, "<<pos<<" )"<<endl;
	switch( data[pos++] )
	{
		case 'G':
			pos++;	// skip $
			sym.setScope( Symbol::SCOPE_GLOBAL );
			npos = data.find('$',pos);
			sym.setName( data.substr(pos,npos-pos) );
			pos=npos;
			break;
		case 'F':
			sym.setScope( Symbol::SCOPE_FILE );
			npos = data.find('$',pos);
			sym.setFile( data.substr(pos,npos-pos) );
			pos = npos+1;	// +1 = skip '$'
			sym.setName( data.substr(pos,npos-pos) );
			pos=npos;
			break;
		case 'L':
			sym.setScope( Symbol::SCOPE_LOCAL );
			npos = data.find('$',pos);
			sym.setFunction( data.substr(pos,npos-pos) );
			pos = npos+1;	// +1 = skip '$'
			npos = data.find('$',pos);
			sym.setName( data.substr(pos,npos-pos) );
			pos=npos;
			break;
		default:
			// optional section not matched
			return false;
	}
	return true;
}
