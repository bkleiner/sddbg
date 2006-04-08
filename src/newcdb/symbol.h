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
#ifndef SYMBOL_H
#define SYMBOL_H
#include <string>
#include <stdint.h>
#include <iostream>
#include <list>
using namespace std;

/**	base class for all types of symbols

	@author Ricky White <ricky@localhost.localdomain>
*/
class Symbol
{
public:
    Symbol();
    ~Symbol();
	
	typedef enum { SYM_CFUNC, SYM_VAR, SYM_ASM } SYMTYPE;
	
	typedef enum { SCOPE_GLOBAL=0, SCOPE_FILE, SCOPE_LOCAL, SCOPE_CNT } SCOPE;
	static const char *scope_name[];
	
	typedef enum
	{
		AS_XSTACK,			///< External stack
		AS_ISTACK,			///< Internal stack
		AS_CODE,			///< Code memory
		AS_CODE_STATIC,		///< Code memory, static segment
		AS_IRAM_LOW,		///< Internal RAM (lower 128 bytes)
		AS_EXT_RAM,			///< External data RAM
		AS_INT_RAM,			///< Internal data RAM
		AS_BIT,				///< Bit addressable area
		AS_SFR,				///< SFR space
		AS_SBIT,			///< SBIT space
		AS_REGISTER,		///< Register space
		AS_UNDEF			///< Used for function records, or any undefined space code
	} ADDR_SPACE;
	static const char addr_space_map[];
	
	void setName( string name )		{ m_name = name;cout <<"{{"<<name<<"}}"<<endl; }
	void setFile( string name )		{ m_file = name; }
	void setAddr( uint32_t addr );
	void setEndAddr( uint32_t addr );
	void setScope( SCOPE scope )	{ m_scope = scope; }
	void setScope( string scope );
	void setScope( char scope );
	void setLevel( int level )		{ m_level = level; }
	void setLine( int i )			{ m_line = i; }
	void setBlock( int block )		{ m_block = block; }
	void setLength( int len )		{ m_length = len; m_end_addr = m_start_addr+m_length; }
	void setFunction( string func )	{ m_function = func; }
	void setAddrSpace( char c );
	void addReg( string reg )		{ m_regs.push_back(reg); }
	// function symbol specific values
	void setIsFunction( bool bfunc=true )	{ m_bFunction = bfunc; }
	void set_interrupt( bool intr=true )	{ m_is_int = intr; }
	int set_interrupt_num( int i )			{ m_int_num = i; }
	int set_reg_bank( int bank )			{ m_reg_bank = bank; }
	
	
	string		name()				{ return m_name; }
	string		file()				{ return m_file; }
	uint32_t	addr()				{ return m_start_addr; }
	int			line()				{ return m_line; }
	int			level()				{ return m_level; }
	int			block()				{ return m_block; }
	SCOPE		scope()				{ return m_scope; }
	string		function()			{ return m_function; }
	uint32_t	endAddr()			{ return m_end_addr; }
	// function symbol specific values
	bool		isFunction()		{ return m_bFunction; }
	bool		is_int_handler()	{ return m_is_int; }
	int			interrupt_num()		{ return m_int_num; }
	int			reg_bank()			{ return m_reg_bank; }
	
	
	void dump();

protected:
	string		m_name;
	string		m_file;
	uint32_t	m_start_addr;
//	uint32_t	end_addr;
	SCOPE		m_scope;
	int			m_line;
	int			m_level;
	int			m_block;
	int			m_length;
	int			m_end_addr;
	string		m_function;
	ADDR_SPACE	m_addr_space;
	list<string> m_regs;
	bool		m_bFunction;
	
	bool	m_is_int;
	int		m_int_num;
	int		m_reg_bank;
};

#endif
