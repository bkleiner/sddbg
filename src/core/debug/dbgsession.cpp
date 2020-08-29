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

namespace debug {

  dbg_session::dbg_session()
      : current_target("")
      , sym_tab(std::make_unique<core::sym_tab>(this))
      , sym_type_tree(std::make_unique<core::sym_type_tree>(this))
      , context_mgr(std::make_unique<core::context_mgr>(this))
      , breakpoint_mgr(std::make_unique<core::breakpoint_mgr>(this))
      , module_mgr(std::make_unique<core::module_mgr>()) {

    current_target = add_target(new core::target_cc())->target_name();
    add_target(new core::target_s51());
    add_target(new core::target_dummy());
    add_target(new core::target_silabs());
  }

  dbg_session::~dbg_session() {}

  core::target *dbg_session::target() {
    return targets[current_target].get();
  }

  core::sym_tab *dbg_session::symtab() {
    return sym_tab.get();
  }

  core::sym_type_tree *dbg_session::symtree() {
    return sym_type_tree.get();
  }

  core::context_mgr *dbg_session::contextmgr() {
    return context_mgr.get();
  }

  core::breakpoint_mgr *dbg_session::bpmgr() {
    return breakpoint_mgr.get();
  }

  core::module_mgr *dbg_session::modulemgr() {
    return module_mgr.get();
  }

  bool dbg_session::select_target(std::string name) {
    auto it = targets.find(name);
    if (it == targets.end())
      return false;

    if (target()) {
      std::cout << "current target " << target()->target_name() << std::endl;

      if (target()->is_connected()) {
        bpmgr()->clear_all();

        target()->stop();
        target()->disconnect();
      }

      // clear out the data structures.
      symtab()->clear();
      symtree()->clear();
      //mcontext_mgr->clear()	@FIXME contextmgr needs a clear or reset
      modulemgr()->reset();
    }

    // select new target
    current_target = it->first;
    std::cout << "selecting target " << target()->target_name() << std::endl;

    return true;
  }

  debug::core::target *dbg_session::add_target(debug::core::target *t) {
    targets[t->target_name()] = std::unique_ptr<core::target>(t);
    return t;
  }
} // namespace debug