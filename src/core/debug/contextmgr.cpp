#include "contextmgr.h"

#include <stdio.h>

#include "breakpointmgr.h"
#include "module.h"
#include "symtab.h"
#include "target.h"

namespace debug::core {

  context_mgr::context_mgr(dbg_session *session)
      : mSession(session) {
  }

  context_mgr::~context_mgr() {
  }

  /** Set the current context to that of the specified address.
	\param addr	Address of instruction to update the context to.
*/
  void context_mgr::set_context(ADDR addr) {
    /// \TODO integrate checks with the breakpoint manager for special breakpoints used to detect entry / exit of all functions in mode 'b'
    std::string module;
    LINE_NUM line;
    if (mSession->modulemgr()->get_c_addr(addr, module, line)) {
      std::string file;
      mSession->symtab()->get_c_function(addr, file, cur_context.function);
      cur_context.mode = C;
      cur_context.module = module;
      cur_context.line = line;
      cur_context.c_line = line;
      if (mSession->symtab()->get_c_block_level(
              mSession->modulemgr()->module(module).get_c_file_name(),
              cur_context.c_line,
              cur_context.block,
              cur_context.level))
        std::cout << "found block/level " << cur_context.block << std::endl;
      else
        std::cout << "coulden't find block/level, file = '"
                  << mSession->modulemgr()->module(module).get_c_file_name() << "', "
                  << cur_context.c_line << std::endl;
      cur_context.addr = addr; // @FIXME we need this address to be the address of the c line for mapping but the asm addr for asm pc pointer on ddd
      //cur_context.function
      cur_context.asm_addr = addr;
    } else if (mSession->modulemgr()->get_asm_addr(addr, module, line)) {
      cur_context.module = module;
      cur_context.line = line;
      cur_context.addr = addr;
      cur_context.asm_addr = addr;
    } else {
      std::cout << "ERROR: Context corrupt!" << std::endl;
      printf("addr = 0x%04x, module='%s', line=%i, pc=0x%04x\n",
             addr,
             module.c_str(),
             line, mSession->target()->read_PC());
    }

    mSession->bpmgr()->stopped(cur_context.addr);
  }

  /** Dumps the current context in a form parsable by ddd but also
	in a human readable form
*/
  void context_mgr::dump() {
    printf("PC = 0x%04x\n", cur_context.addr);
    printf("module:\t%s\n", cur_context.module.c_str());
    printf("Function:\t%s\n", cur_context.function.c_str());
    printf("Line:\t%i\n", cur_context.line);
    printf("Block:\t%i\n", cur_context.block);

    auto &module = mSession->modulemgr()->module(cur_context.module);

    if (cur_context.mode == C) {

      printf("\032\032%s:%d:1:beg:0x%08x\n",
             module.get_c_file_name().c_str(),
             cur_context.line,
             cur_context.asm_addr);

      if (cur_context.c_line <= module.get_c_num_lines())
        printf("%s\n", module.get_c_src_line(cur_context.c_line).src.c_str());

    } else if (cur_context.mode == ASM) {
      printf("\032\032%s:%d:1:beg:0x%08x\n",
             mSession->modulemgr()->module(cur_context.module).get_asm_file_name().c_str(),
             cur_context.line, // vas c_line
             cur_context.asm_addr);

      if (cur_context.line <= module.get_asm_num_lines())
        printf("%s\n", module.get_asm_src_line(cur_context.line).src.c_str());

    } else
      std::cout << "INVALID mode" << std::endl;

#if 0
	fprintf(stdout,"\032\032%s:%d:1:beg:0x%08x\n",
			currCtxt->func->mod->cfullname,
			currCtxt->cline+1,currCtxt->addr);
	else
		fprintf(stdout,"\032\032%s:%d:1:beg:0x%08x\n",
#endif
  }
} // namespace debug::core