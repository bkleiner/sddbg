/***************************************************************************
 *   Copyright (C) 2005 by Ricky White   *
 *   rickyw@neatstuff.co.nz   *
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
#include <string>
#include <iostream>
#include "types.h"
#include "module.h"
#include "symtab.h"
#include "linespec.h"
#include "cmddisassemble.h"

static void print_asm_line( ADDR start, ADDR end, string function );


/** Disassemble commend
	disassemble [startaddr [endaddress]]
*/
bool CmdDisassemble::direct( string cmd )
{
	vector<string> tokens;
	vector<string>::iterator it;
	Tokenize(cmd, tokens);
	ADDR start=-1, end=-1;
	
	if( tokens.size()==1 )
	{
		// start only
		start = strtoul(tokens[0].c_str(),0,0);
		/// @FIXME: need a way to get a symbols address, given the symbol and module and vice versa, give an address and get a symbol
		string file, func;
		symtab.get_c_function( start, file, func );
		cout << "Dump of assembler code for function "<<func<<":" << endl;
		print_asm_line( start, end, func );
		cout << "End of assembler dump." << endl;
		return true;
	}
	else if( tokens.size()==2 )
	{
		// start and end
		start = strtoul(tokens[0].c_str(),0,0);
		end = strtoul(tokens[1].c_str(),0,0);
//		printf("start=0x%04x, end=0x%04x\n",start,end);
		string file, func;
		symtab.get_c_function( start, file, func );
		cout << "Dump of assembler code for function "<<func<<":" << endl;
		print_asm_line( start, end, func );
		cout << "End of assembler dump." << endl;
		return true;
	}
	else
		return false;
}

static void print_asm_line( ADDR start, ADDR end, string function )
{
	uint32_t asm_lines;
	ADDR delta;
	ADDR sym_addr;
	ADDR last_addr;
	string sym_name;
	
	string module;
	LINE_NUM line;
	mod_mgr.get_asm_addr( start,module, line );
	Module &m = mod_mgr.module(module);
	
	asm_lines = m.get_asm_num_lines();
	last_addr = start+1;
	sym_addr = start;
	sym_name.clear();

	sym_addr = start;
	sym_name = function;
	int32_t i,j;
	for ( j=0, i=1; i <= asm_lines; i++ ) 
	{
		if ( start >= 0 && m.get_asm_addr(i) < start)
		{
			continue;
		}
		if ( end >= 0 && m.get_asm_addr(i) > end)
		{
			continue;
		}
		if( !function.empty() )
		{
			ADDR sfunc,efunc;
			symtab.get_addr( function, sfunc, efunc );
			if( m.get_asm_addr(i) < sfunc ||
				m.get_asm_addr(i) > efunc )
				continue;
		}
		delta = m.get_asm_addr(i) - sym_addr;
		if ( delta >= 0 )
		{
			j++;
			last_addr = m.get_asm_addr(i);
			printf( "0x%08x <%s", last_addr, sym_name.c_str() );
			if( delta > 0 ) printf( "+%d", delta );
			printf( ">:\t%s\n", m.get_asm_src(i).c_str() );
		}
	}
}



/** examine command imlementation.
	
	Examine memory: x/FMT ADDRESS.
	ADDRESS is an expression for the memory address to examine.
	FMT is a repeat count followed by a format letter and a size letter.
	Format letters are o(octal), x(hex), d(decimal), u(unsigned decimal),
	t(binary), f(float), a(address), i(instruction), c(char) and s(string).
	Size letters are b(byte), h(halfword), w(word), g(giant, 8 bytes).
	The specified number of objects of the specified size are printed
	according to the format.
	
	Defaults for format and size letters are those previously used.
	Default count is 1.  Default address is following last thing printed
	with this command or "print".

	example format strings
*/
bool CmdX::direct( string cmd )
{
	vector<string> tokens;
	vector<string>::iterator it;
	Tokenize(cmd, tokens);
	LineSpec ls;	// not really suitable, nned a different but similar function
	string fmt;
	int fmt_cnt;
	char fmt_letter, fmt_size;
	
	if( tokens.size()==2 )
	{
#if 0
	// work in progress
		// should be <format><address>
		if( token[0][0]!='/' )
			return false;	// expected format
		fmt = token[0].substr(1);
		fmt_cnt = strtoul(fmt.substr(1),0,0);
		if( is_letter(fmt[fmt.size()-1])
		
		
		switch( fmt_letter )
		{
			case 'i':	// instruction
				break;
			case o:	// octal
				break;
			case 'x':	// hex
				break;
			case 'd':	// decimal
				break;
			case 'u':	// unsigned decimal
				break;
			case 't':	// binary
				break;
			case 'f':	// float
				break;
			case 'a':	// address
				break;
			case 'c':	// char
				break;
			case 's':	// string
				break;	
		}
#endif
	}
	
	// simple hack for now /i address is the only acceptable form
	if( tokens.size()==2 && tokens[0]=="/i"  )
	{
		ADDR a = strtoul(tokens[1].c_str(),0,0);
		print_asm_line( a, a+1, string());
		return true;
	}
	return false;
}

