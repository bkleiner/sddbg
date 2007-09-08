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
#include "dbgsession.h"
#include "target.h"
#include "symtab.h"
#include "symtypetree.h"
#include "breakpointmgr.h"
#include "module.h"
#include "targets51.h"
#include "targetsilabs.h"


/** Build a debug session.
	if no objects are passed in then the default ones are used.
*/
DbgSession::DbgSession(
				SymTab	*dbg_symtab,
				SymTypeTree *dbg_symtypetree,
				ContextMgr *dbg_contextmgr,
				BreakpointMgr *dbg_bpmgr,
				ModuleMgr *dbg_modulemgr )
	: 	mTarget(0)
{
	mSymTab = dbg_symtab ? dbg_symtab : new SymTab;
	mSymTree = dbg_symtypetree ? dbg_symtypetree : new SymTypeTree(*this);
	mContextMgr = dbg_contextmgr ? dbg_contextmgr : new ContextMgr(*this);
	mBpMgr = dbg_bpmgr ? dbg_bpmgr : new BreakpointMgr(*this);
	mModuleMgr = dbg_modulemgr ? dbg_modulemgr : new ModuleMgr();
	
	Target *target;

	target = new TargetS51();
	mTargetMap[target->target_name()] = target;
	target = new TargetSiLabs();
	mTargetMap[target->target_name()] = target;
}

DbgSession::~DbgSession()
{
}


/** Select the specified target driver.
	This fill clear all exsisting data so ytou will need to reload files
*/
bool DbgSession::SelectTarget( std::string name )
{
	TargetMap::iterator i = mTargetMap.find(name);
	if( i == mTargetMap.end() )
		return false;	// failure

	// clean disconnect
	if(mTarget)	target()->stop();
	if(mTarget)	target()->disconnect();

	// clear out the data structures.
	mSymTab->clear();
	mBpMgr->clear_all();
	mSymTree->clear();
	//mContextMgr->clear()	@FIXME contextmgr needs a clear or reset
	mModuleMgr->reset();

	mTarget = (*i).second;
}


