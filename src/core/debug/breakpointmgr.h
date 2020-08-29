#pragma once

#include <list>
#include <string>

#include "dbgsession.h"
#include "types.h"

namespace debug::core {

  typedef int32_t bp_id;
  static constexpr bp_id BP_ID_INVALID = -1;

  struct breakpoint {
    bp_id id;
    ADDR addr;

    bool temporary;
    bool disabled;

    std::string what;
  };

  class breakpoint_mgr {
  public:
    breakpoint_mgr(dbg_session *session);

    bp_id set_bp(ADDR addr, bool temporary = false);
    bp_id set_breakpoint(std::string cmd, bool temporary = false);

    bool enable_bp(bp_id id);
    bool disable_bp(bp_id id);

    void clear_all();
    void reload_all();
    void dump();

    bool clear_breakpoint(std::string cmd);
    bool clear_breakpoint_id(bp_id id);
    bool clear_breakpoint_addr(ADDR addr);

    void stopped(ADDR addr);

    bool active_bp_at(ADDR addr);

  protected:
    dbg_session *session;
    std::list<breakpoint> bplist;

    int next_id();

    bool add_target_bp(ADDR addr);
    bool del_target_bp(ADDR addr);
  };

} // namespace debug::core