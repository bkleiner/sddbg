#include <iostream>
#include <stdint.h>

#include "breakpointmgr.h"
#include "dbgsession.h"
#include "module.h"
#include "symtab.h"
#include "symtypetree.h"
#include "target-dummy.h"

#include "targetcc.h"
#include "targets51.h"
#include "targetsilabs.h"

namespace debug::core {

  /** Build a debug session.
	if no objects are passed in then the default ones are used.
*/
  dbg_session::dbg_session(
      sym_tab *dbg_symtab,
      sym_type_tree *dbg_symtypetree,
      context_mgr *dbg_contextmgr,
      breakpoint_mgr *dbg_bpmgr,
      module_mgr *dbg_modulemgr)
      : mTarget(0) {

    std::cout << "====================== dbg_session Constructor =========================" << std::endl;
    mSymTab = dbg_symtab ? dbg_symtab : new sym_tab(this);
    mSymTree = dbg_symtypetree ? dbg_symtypetree : new sym_type_tree(this);
    mcontext_mgr = dbg_contextmgr ? dbg_contextmgr : new context_mgr(this);
    mBpMgr = dbg_bpmgr ? dbg_bpmgr : new breakpoint_mgr(this);
    mModuleMgr = dbg_modulemgr ? dbg_modulemgr : new module_mgr();
    std::cout << "constructor this=" << this << std::endl;

    mTarget = add_target(new target_cc());
    add_target(new target_s51());
    add_target(new target_dummy());
    add_target(new target_silabs());
  }

  /** Select the specified target driver.
	This fill clear all exsisting data so ytou will need to reload files
*/
  bool dbg_session::SelectTarget(std::string name) {
    TargetMap::iterator i = mTargetMap.find(name);
    if (i == mTargetMap.end())
      return false; // failure
    if (target()) {
      std::cout << "current target " << target()->target_name() << std::endl;
      if (target()->is_connected()) {
        mBpMgr->clear_all();
        // clean disconnect
        if (mTarget)
          target()->stop();
        if (mTarget)
          target()->disconnect();
      }
      // clear out the data structures.
      mSymTab->clear();
      mSymTree->clear();
      //mcontext_mgr->clear()	@FIXME contextmgr needs a clear or reset
      mModuleMgr->reset();
    }

    // select new target
    mTarget = (*i).second;
    std::cout << "selecting target " << target()->target_name() << std::endl;

    return true;
  }

  debug::core::target *dbg_session::add_target(debug::core::target *t) {
    mTargetMap[t->target_name()] = t;

    TargetInfo ti;
    ti.name = t->target_name();
    ti.descr = t->target_descr();
    mTargetInfoVec.push_back(ti);

    return t;
  }
} // namespace debug::core