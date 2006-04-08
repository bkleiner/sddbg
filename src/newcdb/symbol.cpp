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
#include <string>
#include "symbol.h"

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
	if( m_length!=-1 )
		m_end_addr = m_start_addr + m_length;
	m_start_addr = addr;
}

void Symbol::setEndAddr( uint32_t addr )
{
	m_end_addr = addr;
	m_length = m_end_addr - m_start_addr + 1;
}

void Symbol::dump()
{
	printf("%-15s0x%08x  0x%08x  %-9s %-8s %-12s %c ",
		   m_name.c_str(),
		   m_start_addr,
		   m_end_addr,
		   m_file.c_str(),
		   scope_name[m_scope],
		   m_function.c_str(),
		   addr_space_map[m_addr_space]
		  );
	list<string>::iterator it;
	for(it=m_regs.begin(); it!=m_regs.end();++it)
	{
		printf("%s ",it->c_str());
	}
	printf("\n");
}
