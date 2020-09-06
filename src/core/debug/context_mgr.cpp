#include "context_mgr.h"

#include <stdio.h>

#include "breakpoint_mgr.h"
#include "disassembly.h"
#include "log.h"
#include "module.h"
#include "registers.h"
#include "sym_tab.h"
#include "target.h"

namespace debug::core {

  context_mgr::context_mgr(dbg_session *session)
      : session(session) {
  }

  context context_mgr::set_context(ADDR addr) {
    session->bpmgr()->stopped(addr);

    stack.clear();
    stack.push_back(build_context(addr));

    /*
    uint8_t sp = session->regs()->read(cpu_register_names::SP);
    uint8_t stack_size = sp - 0x20;

    uint8_t full_stack[stack_size];
    session->target()->read_data(0x20, stack_size, full_stack);

    while (sp > 0x20) {
      uint16_t last_pc = full_stack[sp - 1 - 0x20];
      last_pc -= 3;
      stack.push_back(build_context(last_pc));
      sp -= 2;
    }
    */

    if (stack[0].module == "") {
      log::print("ERROR: Context corrupt!\n");
    }

    return stack[0];
  }

  context context_mgr::build_context(ADDR addr) {
    context c = {};
    c.addr = addr;

    std::string c_file;
    session->modulemgr()->get_c_addr(addr, c.module, c.c_line);
    session->symtab()->get_c_function(addr, c_file, c.function);
    session->symtab()->get_c_block_level(c_file, c.c_line, c.block, c.level);
    session->modulemgr()->get_asm_addr(addr, c.module, c.asm_line);

    if (c.function != "") {
      symbol *fun = session->symtab()->get_symbol(c, c.function);
      c.in_interrupt_handler = fun->is_type(symbol::INTERRUPT);
    } else {
      c.in_interrupt_handler = false;
    }

    return c;
  }

  context context_mgr::update_context() {
    return set_context(session->target()->read_PC());
  }

  /** Dumps the current context in a form parsable by ddd but also
      in a human readable form
    */
  void context_mgr::dump() {
    auto ctx = get_current();

    log::printf("PC = 0x%04x\n", ctx.addr);
    log::printf("module:\t%s\n", ctx.module.c_str());
    log::printf("Function:\t%s\n", ctx.function.c_str());
    log::printf("C Line:\t%i\n", ctx.c_line);
    log::printf("ASM Line:\t%i\n", ctx.asm_line);
    log::printf("Block:\t%i\n", ctx.block);

    auto &module = session->modulemgr()->module(ctx.module);

    log::printf("%s:%d:1:beg:0x%08x\n",
                module.get_c_file_name().c_str(),
                ctx.c_line,
                ctx.addr);

    if (ctx.c_line > 0 && ctx.c_line <= module.get_c_num_lines())
      log::printf("%s\n", module.get_c_src_line(ctx.c_line).src.c_str());

    log::printf("%s:%d:1:beg:0x%08x\n",
                session->modulemgr()->module(ctx.module).get_asm_file_name().c_str(),
                ctx.asm_line,
                ctx.addr);

    if (ctx.asm_line > 0 && ctx.asm_line <= module.get_asm_num_lines())
      log::printf("%s\n", module.get_asm_src_line(ctx.asm_line).src.c_str());
  }
} // namespace debug::core