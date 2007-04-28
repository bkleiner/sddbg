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
#include <assert.h>
#include <iostream>
#include <string>
#include "symbol.h"
#include "symtypetree.h"
#include "contextmgr.h"
#include "memremap.h"

using namespace std;

const char *Symbol::scope_name[] = { "Global","File","Local",0};
const char Symbol::addr_space_map[] = 
{ 'A','B','C','D','E','F','H','I','J','R','Z' };

Symbol::Symbol()
{
	setAddrSpace('Z');	// undefined
	m_name="";
	m_start_addr = 0xffffffff;
	m_end_addr = -1;
	m_length = -1;
	m_bFunction = false;
}


Symbol::~Symbol()
{
}


void Symbol::setScope( string scope )
{
	for(int i=0; i<SCOPE_CNT; i++ )
	{
		
		if( scope.compare(scope_name[i]) )
			setScope( SCOPE(i) );
	}
}

void Symbol::setScope( char scope )
{
	switch( scope )
	{
		case 'G': setScope( SCOPE_GLOBAL );	break;
		case 'F': setScope( SCOPE_FILE );		break;
		case 'L': setScope( SCOPE_LOCAL );		break;
		default:  cout << "ERROR invalid scope" << endl;
	}
}

void Symbol::setAddrSpace( char c )
{
	for(int i=0; i<sizeof(addr_space_map); i++)
	{
		if( addr_space_map[i]==c )
		{
			m_addr_space = ADDR_SPACE(i);
			return;
		}
	}
	setAddrSpace('Z');	// Invalid
}

void Symbol::setAddr( uint32_t addr )
{
	m_start_addr = addr;
	if( m_length!=-1 )
		m_end_addr = m_start_addr + m_length - 1;
//	m_start_addr = addr;
}

void Symbol::setEndAddr( uint32_t addr )
{
	m_end_addr = addr;
	m_length = m_end_addr - m_start_addr + 1;
}

void Symbol::dump()
{
	string name = m_name;
	char buf[255];
	memset(buf,0,sizeof(buf));
	for(int i=0; i<m_array_dim.size(); i++)
	{
		snprintf(buf,sizeof(buf),"[%i]",m_array_dim[i]);
		name += buf;
	}
	printf("%-15s0x%08x  0x%08x  %-9s %-8s %-12s %c %-10s",
		   name.c_str(),
		   m_start_addr,
		   m_end_addr,
		   m_file.c_str(),
		   scope_name[m_scope],
		   m_function.c_str(),
		   addr_space_map[m_addr_space],
		   m_type_name.c_str()
		  );
	list<string>::iterator it;
	if( !m_regs.empty() )
	{
		printf("Regs: ");
		for(it=m_regs.begin(); it!=m_regs.end();++it)
		{
			printf("%s ",it->c_str());
		}
	}
	printf("\n");
}


/** Print the symbol with the specified indent
*/
void Symbol::print( char format )
{
	SymType *type;
	//printf("*** name = %s\n",m_name.c_str());
	cout << m_name << " = ";
//	if( sym_type_tree.get_type( m_type_name, &type ) )
	//sym_type_tree.get_type( m_type_name, file, block, &type ) )
	// FIXME a context woule be usefule here...
	
	ContextMgr::Context context;
	type = sym_type_tree.get_type( m_type_name, context );
	if( type )
	{
		if( type->terminal() )
		{
			// print out data now...
			cout << "type = " << type->name() << endl;
			// @TODO pass flat memory address to the type so it can reterieve the data and print it out.
			
			// @FIXME: need to use the flat addr from remap rather than just the start without an area.
			uint32_t flat_addr = MemRemap::flat( m_start_addr, addr_space_map[m_addr_space] );	// map needs to map to lower case in some cases...!!! maybe fix in memremap
			
			flat_addr = MemRemap::flat( m_start_addr,'d');	// @FIXME remove this hack
//			cout << type->pretty_print( m_name, indent, flat_addr );
			cout << type->pretty_print( format,m_name, flat_addr );
		}
		else
		{
			// its more comples like a structure or typedef
		}
		cout << endl;
	}
//	assert(1==0);	// oops unknown type
}



