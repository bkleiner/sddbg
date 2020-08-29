#pragma once

#include <assert.h>
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

namespace debug::core {

  class target;
  class sym_tab;
  class sym_type_tree;
  class context_mgr;
  class breakpoint_mgr;
  class module_mgr;

  class dbg_session {
  public:
    dbg_session(sym_tab *dbg_symtab = 0,
                sym_type_tree *dbg_symtypetree = 0,
                context_mgr *dbg_contextmgr = 0,
                breakpoint_mgr *dbg_bpmgr = 0,
                module_mgr *dbg_modulemgr = 0);

    debug::core::target *target() {
      assert(mTarget);
      return mTarget;
    }
    sym_tab *symtab() {
      assert(mSymTab);
      return mSymTab;
    }
    sym_type_tree *symtree() {
      assert(mSymTree);
      return mSymTree;
    }
    context_mgr *contextmgr() {
      assert(mcontext_mgr);
      return mcontext_mgr;
    }
    breakpoint_mgr *bpmgr() {
      assert(mBpMgr);
      return mBpMgr;
    }
    module_mgr *modulemgr() {
      assert(mModuleMgr);
      return mModuleMgr;
    }

    bool SelectTarget(std::string name);
    typedef std::map<std::string, debug::core::target *> TargetMap;

    typedef struct
    {
      std::string name;
      std::string descr;
    } TargetInfo;
    typedef std::vector<TargetInfo> TargetInfoVec;
    TargetInfoVec get_target_info() { return mTargetInfoVec; }

  private:
    debug::core::target *mTarget;

    sym_tab *mSymTab;
    sym_type_tree *mSymTree;
    context_mgr *mcontext_mgr;
    breakpoint_mgr *mBpMgr;
    module_mgr *mModuleMgr;

    TargetMap mTargetMap;
    TargetInfoVec mTargetInfoVec;

    debug::core::target *add_target(debug::core::target *t);
  };

} // namespace debug::core