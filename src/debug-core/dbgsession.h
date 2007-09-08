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
#ifndef DBGSESSION_H
#define DBGSESSION_H

#include <map>
#include <string>

class Target;
class SymTab;
class SymTypeTree;
class ContextMgr;
class BreakpointMgr;
class ModuleMgr;

/**
This class holds data about a single debug session

	@author Ricky White <rickyw@neatstuff.co.nz>
*/
class DbgSession
{
public:
	DbgSession( SymTab	*dbg_symtab = 0,
				SymTypeTree *dbg_symtypetree = 0,
				ContextMgr *dbg_contextmgr = 0,
				BreakpointMgr *dbg_bpmgr = 0,
				ModuleMgr *dbg_modulemgr = 0 );
    ~DbgSession();

	Target *target()			{ return mTarget; }
	SymTab *symtab()			{ return mSymTab; }
	SymTypeTree *symtree()		{ return mSymTree; }
	ContextMgr	*contextmgr()	{ return mContextMgr; }
	BreakpointMgr *bpmgr()		{ return mBpMgr; }
	ModuleMgr *modulemgr()		{ return mModuleMgr; }

	bool SelectTarget( std::string name );

private:
	Target			*mTarget;
	SymTab			*mSymTab;
	SymTypeTree		*mSymTree;
	ContextMgr		*mContextMgr;
	BreakpointMgr	*mBpMgr;
	ModuleMgr		*mModuleMgr;
	typedef std::map<std::string,Target*>	TargetMap;
	TargetMap		mTargetMap;
};

#endif