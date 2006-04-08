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
#ifndef BREAKPOINTMGR_H
#define BREAKPOINTMGR_H
#include <list>
using namespace std;

/**
Manager class for breakpoints
handles temporary and normal breakpoints
supports emulater limits such as limited number of hardware breakpoints etc

	@author Ricky White <ricky@localhost.localdomain>
*/
class BreakpointMgr
{
public:
    BreakpointMgr();
    ~BreakpointMgr();
	
	/** set a normal breakpoint at the specified address
		\param addr	address to set breakpoint on
		\returns true if the breakpoint was set, otherwise false
	*/
	bool set_bp( uint16_t addr, bool temporary=false );
	
	bool set_bp( string file, unsigned int line );

	/** Set a temporary breakpoint.
		Temporary breakpoints will be deleted after the target stops on them
		\param addr	address to set breakpoint on
		\returns true if the breakpoint was set, otherwise false
	*/
	bool set_temp_bp( uint16_t addr );

	bool set_temp_bp( string file, unsigned int line );
	
	bool enable_bp( int id );
	bool disable_bp( int id );
	
	/** call when execution stops to check of a temporary breakpoint caused it
		if so this will delete the breakpoint, freeing its resources up
		\param addr Address where the target stopped
	*/
	void stopped_at( uint16_t addr );
	
	/** Clear all breakpoints.
	*/
	void clear_all();
	
	/** Dump a list of all breakpoints to the console
	*/
	void dump();
	
	bool set_breakpoint( string cmd, bool temporary=false );
	bool clear_breakpoint( string cmd );
	bool clear_breakpoint_id( uint16_t id  );
	bool clear_breakpoint_addr( uint16_t addr );
	
	/** indicates to the breakpoint manager that we have stopped.
		This is needed to temporary breakpoints can be modified as necessary
		\param addr The attreas at which the target stopped
	*/
	void stopped( uint16_t addr);

	/** returns the file we are currently stopped in
		used by LineSpec
	*/
	string current_file();
	
protected:
	typedef struct
	{
		uint16_t	id;				///< id trackng number
		uint16_t	addr;			///< Address of BP
		bool		bTemp;			///< true if a temporary BP
		bool 		bDisabled;		///< Indicates if the breaakpoint is currently disabled
		string		file;
		int			line;
		string		what;			///< identification of the breakpoint (file/function/line/addr etc)
	} BP_ENTRY;
	typedef list<BP_ENTRY> BP_LIST;
	BP_LIST	bplist;
	uint16_t		cur_addr;		///< address we last stopped at,  this reflects the address we are currently at at any point where we are stopped
	
	int next_id();
	
};

extern BreakpointMgr bp_mgr;

#endif
