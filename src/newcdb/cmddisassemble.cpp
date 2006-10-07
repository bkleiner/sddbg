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
#include "memremap.h"
#include "target.h"
extern Target *target;

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





	@TOD add support for $sp $pc $fp and $ps
*/
bool CmdX::direct( string cmd )
{
	vector<string> tokens;
	vector<string>::iterator it;
	Tokenize(cmd, tokens);
	uint32_t flat_addr;
	if( tokens.size()<1)
		return false;
	if( tokens[0][0]!='/' )
	{
		// no format or size information, use defaults.
		flat_addr = strtoul(tokens[0].c_str(),0,0);
	}
	else if( parseFormat( tokens[0] ) )
	{
		flat_addr = strtoul(tokens[1].c_str(),0,0);
	}
	else
		return false;
	
	for( int num=0; num<num_units; num++ )
	{
		char area;
		ADDR addr = MemRemap::target( flat_addr, area );
		
		switch(format)
		{
			case 'i':	// instruction
				if( area !='c' )
				{
					printf("ERROR: can't print out in instruction format for non code memory areas\n");
				}
				else
					print_asm_line( addr, addr+1, string());
				break;
			case 'x':	// hex
				printf("0x");
				for(int i=(unit_size-1); i>=0; i-- )
				{
					printf("%02x",(unsigned char)readMem( flat_addr+i ));
				}
				printf("\n");
				break;
			case 's':	// string
				printf("string here\n");
				break;
		}
		flat_addr += unit_size;
	}
}

/** parse the /nfu token
	must begin with '/' or it isn't a format specifier and we return false.

	n = number of unit outputs to show
	f = 's' / 'i' / 'x'	or not present for default
	u = 'b' / 'h' / 'w' / 'g'	number of bytes in word, optional
*/
bool CmdX::parseFormat(string token)
{
	token = token.substr(1);
	for( int i=0; i<token.size(); i++)
	{
		// for each char
		if( isdigit(token[i]) )
			num_units = token[i] - '0';
		else
		{
			switch(token[i])
			{
				case 's':
				case 'i':
				case 'x':
					format = token[i];
					break;
				case 'b': unit_size = 1; break;
				case 'h': unit_size = 2; break;
				case 'w': unit_size = 4; break;
				case 'g': unit_size = 8; break;
				default:
					return false;	// invalid format
			}
		}
	}
	return true;
}


uint8_t CmdX::readMem( uint32_t flat_addr )
{
	unsigned char b;
	char area;
	ADDR addr = MemRemap::target( flat_addr, area );
	switch( area)
	{
		case 'c':
			target->read_code( addr, 1, &b );
			return b;
		case 'd':
			target->read_data( addr, 1, &b );
			return b;
		case 'x':
			target->read_xdata( addr, 1, &b );
			return b;
		case 'i':
			target->read_data( addr+0x100, 1, &b );	// @FIXME: the offset is incorrect and we probably need a target function for accessing idata
			return b;
		case 's':
			target->read_sfr( addr, 1, &b );
			return b;
		default:
			printf("ERROR: invalid memory area '%c'\n",area);
			return 0;
	}
}



