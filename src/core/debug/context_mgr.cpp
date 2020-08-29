#include "context_mgr.h"

#include <stdio.h>

#include "breakpoint_mgr.h"
#include "module.h"
#include "sym_tab.h"
#include "target.h"

namespace debug::core {

  context_mgr::context_mgr(dbg_session *session)
      : mSession(session) {
  }

  void context_mgr::set_context(ADDR addr) {
    mSession->bpmgr()->stopped(addr);

    context c = {};
    c.addr = addr;

    std::string c_file;
    mSession->modulemgr()->get_c_addr(addr, c.module, c.c_line);
    mSession->symtab()->get_c_function(addr, c_file, c.function);
    mSession->symtab()->get_c_block_level(c_file, c.c_line, c.block, c.level);
    mSession->modulemgr()->get_asm_addr(addr, c.module, c.asm_line);

    if (c.module == "")
      std::cout << "ERROR: Context corrupt!" << std::endl;

    cur_context = c;
  }

  /** Dumps the current context in a form parsable by ddd but also
	in a human readable form
*/
  void context_mgr::dump() {
    printf("PC = 0x%04x\n", cur_context.addr);
    printf("module:\t%s\n", cur_context.module.c_str());
    printf("Function:\t%s\n", cur_context.function.c_str());
    printf("C Line:\t%i\n", cur_context.c_line);
    printf("ASM Line:\t%i\n", cur_context.asm_line);
    printf("Block:\t%i\n", cur_context.block);

    auto &module = mSession->modulemgr()->module(cur_context.module);

    printf("\032\032%s:%d:1:beg:0x%08x\n",
           module.get_c_file_name().c_str(),
           cur_context.c_line,
           cur_context.addr);

    if (cur_context.c_line > 0 && cur_context.c_line <= module.get_c_num_lines())
      printf("%s\n", module.get_c_src_line(cur_context.c_line).src.c_str());

    printf("\032\032%s:%d:1:beg:0x%08x\n",
           mSession->modulemgr()->module(cur_context.module).get_asm_file_name().c_str(),
           cur_context.asm_line,
           cur_context.addr);

    if (cur_context.asm_line > 0 && cur_context.asm_line <= module.get_asm_num_lines())
      printf("%s\n", module.get_asm_src_line(cur_context.asm_line).src.c_str());
  }
} // namespace debug::core