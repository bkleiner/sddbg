#include "cmdcommon.h"

#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fmt/format.h>

#include "breakpoint_mgr.h"
#include "cdb_file.h"
#include "context_mgr.h"
#include "line_spec.h"
#include "module.h"
#include "sddbg.h"
#include "target.h"
#include "types.h"

extern std::string prompt;

namespace debug {

  bool CmdVersion::show(ParseCmd::Args cmd) {
    if (cmd.empty()) {
      std::cout << "\nVersion 0.1 (jelly)\n"
                << "Compiled on " << __DATE__ << " at " << __TIME__ << "\n"
                << std::endl;
      return true;
    }
    return false;
  }

  bool CmdWarranty::show(ParseCmd::Args cmd) {
    if (cmd.empty()) {
      std::cout << "                            NO WARRANTY\n"
                   "\n"
                   "  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
                   "FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
                   "OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
                   "PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
                   "OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
                   "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
                   "TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
                   "PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
                   "REPAIR OR CORRECTION.\n"
                   "\n"
                   "  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
                   "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
                   "REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
                   "INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
                   "OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
                   "TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
                   "YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
                   "PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
                   "POSSIBILITY OF SUCH DAMAGES.\n"
                   "\n";
      return true;
    }
    return false;
  }

  bool CmdCopying::show(ParseCmd::Args cmd) {
    //	std::string s = getenv("_");
    if (cmd.empty()) {
      execl("/usr/bin/less", "less", "copying", NULL);
      return true;
    }
    return false;
  }

  /** Redirect commands directly to the target driver
	Ideaslly not much should be done through this interface in the interest of
	portability to other targer devices
*/
  bool CmdTarget::direct(ParseCmd::Args cmd) {
    return gSession.target()->command(join(cmd));
  }

  bool CmdTarget::set(ParseCmd::Args cmds) {
    std::string cmd = join(cmds);

    if (cmd.find("port ") == 0) {
      std::cout << "set port '" << cmd.substr(5) << "'" << std::endl;
      gSession.target()->set_port(cmd.substr(5));
      return true;
    } else if (cmd.find("device ") == 0) {
      std::cout << "set device" << cmd << std::endl;
      return true;
    } else if (cmd.compare("connect") == 0) {
      if (gSession.target()) {
        gSession.target()->connect();
        return true;
      } else {
        std::cout << "ERROR you must select a target first." << std::endl
                  << "for silicon labs debuggers		     set target SL51" << std::endl
                  << "for the s51 simulator			     set target S51" << std::endl
                  << std::endl;
        return false;
      }
    } else if (cmd.compare("disconnect") == 0) {
      gSession.target()->disconnect();
      return true;
    } else {
      if (gSession.select_target(cmd)) {
        // disconnect from current target and select new one
        // Don't connect yet, user probably needs to setup ports before cmmanding a connect
        return true;
      }
    }
    return false;
  }

  bool CmdTarget::info(ParseCmd::Args cmds) {
    std::string cmd = join(cmds);

    if (cmd.find("port") == 0) {
      std::cout << "port = \""
                << "/dev/ttyS0"
                << "\"." << std::endl;
      return true;
    } else if (cmd.find("device") == 0) {
      std::cout << "device = \""
                << "80C51"
                << "\"." << std::endl;
      return true;
    } else if (cmd.empty()) {
      std::cout << "target = '" << gSession.target()->target_name() << "'\t"
                << "'" << gSession.target()->target_descr() << "'" << std::endl;
      std::cout << "Port = '" << gSession.target()->port() << "'" << std::endl;
      std::cout << "Device = '" << gSession.target()->device() << "'" << std::endl;
      printf("PC = 0x%04x\n", gSession.target()->read_PC());

      gSession.contextmgr()->dump();
      return true;
    }
    return false;
  }

  bool CmdTarget::show(ParseCmd::Args cmd) {
    if (match(cmd.front(), "connect")) {
      std::cout << (gSession.target()->is_connected() ? "Connected." : "Disconnected.") << std::endl;
      return true;
    }
    return false;
  }

  /** cause the target to step one source level instruction
*/
  bool CmdStep::directnoarg() {
    std::string module;
    core::LINE_NUM line;
    core::ADDR addr;
    // keep stepping over asm instructions until we hit another c function
    do {
      addr = gSession.target()->step();
      gSession.contextmgr()->set_context(addr);
    } while (!gSession.modulemgr()->get_c_addr(addr, module, line) &&
             !gSession.target()->check_stop_forced());
    gSession.contextmgr()->dump();
    return true;
  }

  /** cause the target to step one assembly level instruction
 */
  bool CmdStepi::directnoarg() {
    core::ADDR addr = gSession.target()->step();
    gSession.contextmgr()->set_context(addr);
    gSession.contextmgr()->dump();
    return true;
  }

  /** Continue to the next source line in the current (innermost) stack frame.
	This is similar to step, but function calls that appear within the line of
	code are executed without stopping.
	Execution stops when control reaches a different line of code at the
	original stack level that was executing when you gave the next command.
	This command is abbreviated n.
*/
  bool CmdNext::directnoarg() {
    core::ADDR addr = gSession.target()->read_PC();

    std::string module;
    core::LINE_NUM line;
    gSession.modulemgr()->get_c_addr(addr, module, line);
    const auto ctx = gSession.contextmgr()->get_current();

    // keep stepping over asm instructions until we hit another c line in the current function
    core::LINE_NUM current_line = line;
    while (line == current_line) {
      addr = gSession.target()->step();
      gSession.contextmgr()->set_context(addr);

      core::LINE_NUM new_line;
      if (gSession.modulemgr()->get_c_addr(addr, module, new_line)) {
        const auto current_context = gSession.contextmgr()->get_current();

        if (current_context.module == ctx.module && current_context.function == ctx.function)
          current_line = new_line;
      }
    }

    gSession.contextmgr()->dump();
    return true;
  }

  /** Execute one machine instruction, but if it is a function call, proceed until
	the function returns.

	@FIXME: change from stepi implementation to proper nexti
*/
  bool CmdNexti::directnoarg() {
    core::ADDR addr = gSession.target()->step();
    gSession.contextmgr()->set_context(addr);
    gSession.contextmgr()->dump();
    return true;
  }

  /**	Continue execution from the current address
	if there is a breakpoint on the current address it is ignored.
	optional parameter specifies a further number of breakpoints to ignore
*/
  bool CmdContinue::direct(ParseCmd::Args cmd) {
    fmt::print("Continuing.\n");
    int i = strtoul(cmd.front().c_str(), 0, 0);

    gSession.target()->run_to_bp(i);
    core::ADDR addr = gSession.target()->read_PC();
    gSession.contextmgr()->set_context(addr);
    gSession.contextmgr()->dump();
    return true;
  }

  /**	Continue execution from the current address and stop at next breakpoint
*/
  bool CmdContinue::directnoarg() {
    fmt::print("Continuing.\n");
    gSession.target()->run_to_bp();

    core::ADDR addr = gSession.target()->read_PC();
    gSession.contextmgr()->set_context(addr);
    gSession.contextmgr()->dump();
    return true;
  }

  /** Reset the target abnd reload the breakpoints as necessary
*/
  bool CmdRun::directnoarg() {
    gSession.target()->stop();
    gSession.target()->disconnect();
    gSession.target()->connect();
    gSession.target()->reset();
    gSession.bpmgr()->reload_all();

    if (gSession.bpmgr()->set_breakpoint("main", true) == core::BP_ID_INVALID)
      std::cout << " failed to set main breakpoint!" << std::endl;

    gSession.target()->run_to_bp();
    core::ADDR addr = gSession.target()->read_PC();
    gSession.contextmgr()->set_context(addr);
    gSession.contextmgr()->dump();
    return true;
  }

  /** open a new cdb file for debugging
	all associated files must be in the same directory
*/
  bool CmdFile::direct(ParseCmd::Args cmd) {
    gSession.modulemgr()->reset();
    gSession.symtab()->clear();
    gSession.symtree()->clear();
    gSession.bpmgr()->clear_all();

    // disconnect and reconnect to make sure data is valid (fixes bug where
    //   data reads are wrong after loading new file)
    gSession.target()->disconnect();
    gSession.target()->connect();
    core::cdb_file cdbfile(&gSession);
    cdbfile.open(cmd.front() + ".cdb");
    return gSession.target()->load_file(cmd.front() + ".ihx");
  }

  /** open a new cdb file for debugging, WITHOUT loading the firmware to the device
	all associated files must be in the same directory
*/
  bool CmdDFile::direct(ParseCmd::Args cmd) {
    gSession.modulemgr()->reset();
    gSession.symtab()->clear();
    gSession.symtree()->clear();
    gSession.bpmgr()->clear_all();

    core::cdb_file cdbfile(&gSession);
    cdbfile.open(cmd.front() + ".cdb");
    return true;
  }

  /** list a section of the program
	list linenum
list function
list
list -
list linespec
list first,last
list ,last
list first,
list +
list -

linespec:
	number
	+offset
	-offset
	filename:number 
	function
	filename:function 
	*address
*/
  bool CmdList::direct(ParseCmd::Args cmd) {
    std::cout << "NOT implemented [" << join(cmd) << "]" << std::endl;
    return true;
  }

  bool CmdList::directnoarg() {
    std::cout << "NOT implemented" << std::endl;
    return true;
  }

  bool CmdPWD::directnoarg() {
    printf("Working directory %s.\n", "dir here"); // @TODO replace "dir here with current path"
    return true;
  }

  /** info files and info target are synonymous; both print the current target
*/
  bool CmdFiles::info(ParseCmd::Args cmd) {
    std::cout << "Symbols from \"/home/ricky/projects/ec2cdb/debug/src/test\"." << std::endl; // @TODO put correct pathe in here
    return true;
  }

  bool CmdSource::info(ParseCmd::Args cmd) {
    std::string file = "";
    if (cmd.empty()) {
      auto ctx = gSession.contextmgr()->get_current();
      if (ctx.module == "") {
        fmt::print("no current module\n");
        return true;
      }
      file = ctx.module;
    } else {
      file = cmd[0];
    }

    core::module &module = gSession.modulemgr()->module(file);
    for (size_t i = 1; i < module.get_c_num_lines() + 1; i++) {
      auto line = module.get_c_src_line(i);
      fmt::print(
          "{: >4} {:#06x} {: >2} {: >2}: {}\n",
          i,
          line.addr > 0 ? line.addr : 0x0,
          line.block,
          line.level,
          line.src);
    }
    return true;
  }

  bool CmdSources::info(ParseCmd::Args cmd) {
    if (cmd.empty()) {
      std::cout << "Current source file is test.c" << std::endl;
      std::cout << "Located in test.c" << std::endl;
      std::cout << "Contains 11 lines." << std::endl;
      std::cout << "Source language is c." << std::endl;
      return true;
    }
    return false;
  }

  /**  	
	Examples

	(gdb) info line m4_changequote
	Line 895 of "builtin.c" starts at pc 0x634c and ends at 0x6350.
	
	We can also inquire (using *addr as the form for linespec) what source line covers a particular address:

	(gdb) info line *0x63ff
	Line 926 of "builtin.c" starts at pc 0x63e4 and ends at 0x6404.
*/
  bool CmdLine::info(ParseCmd::Args cmd) {
    //	if( cmd.find(' ')>=0 || cmd.length()==0 )
    //		return false;	// cmd must be just one word
    if (cmd.empty()) {
      /// @FIXME need a current context for this one...
      return true;
    }

    const core::line_spec ls = core::line_spec::create(&gSession, join(cmd));
    if (!ls.valid()) {
      return false;
    }

    printf("Line %i of \"%s\" starts at pc 0x%04x and ends at 0x%04x.\n",
           ls.line,
           ls.file.c_str(),
           ls.addr,
           ls.end_addr);
    // test.c:19:1:beg:0x000000f8
    printf("\032\032%s:%i:%i:beg:0x%08x\n",
           ls.file.c_str(),
           ls.line,
           1, // what should this be?
           ls.addr);
    return true;
  }

  bool CmdPrompt::set(ParseCmd::Args cmd) {
    prompt = join(cmd) + " ";
    return true;
  }

  bool CmdStop::directnoarg() {
    std::cout << "Stopping target" << std::endl;
    gSession.target()->stop();
    return true;
  }

  bool CmdFinish::directnoarg() {
    std::cout << "Finishing current function" << std::endl;
    // @fixme set a breakpoint at the end of the current function
    //bp_mgr.set_breakpoint(
    return true;
  }

  /** `print EXPR'
	`print /F EXPR'
		EXPR is an expression (in the source language).  By default the
		value of EXPR is printed in a format appropriate to its data type;
		you can choose a different format by specifying `/F', where F is a
		letter specifying the format; see  Output formats Output
		Formats.

	`print'
	`print /F'
		If you omit EXPR, GDB displays the last value again (from the
		"value history";  Value history Value History.).  This
		allows you to conveniently inspect the same value in an
		alternative format.
	
		A more low-level way of examining data is with the `x' command.  It
	examines data in memory at a specified address and prints it in a
	specified format.   Examining memory Memory.
	
		If you are interested in information about types, or about how the
	fields of a struct or a class are declared, use the `ptype EXP' command
	rather than `print'.   Examining the symbol Table Symbols.


	\NOTE expr currently must be a variable name.  indexing into arrays is not yet supported, just displaying the entire array.

	\param expr	expression to display
*/
  bool CmdPrint::direct(ParseCmd::Args cmd) {
    char format = 0;
    if (cmd[0][0] == '/') {
      format = cmd[0][1];
      cmd.pop_front();
    }

    std::string expr = cmd.front();
    std::string sym_name = expr;
    int seperator_pos = expr.find_first_of(".[");
    if (seperator_pos != -1)
      sym_name = expr.substr(0, seperator_pos);

    // figure out where we are
    const auto ctx = gSession.contextmgr()->get_current();
    core::symbol *sym = gSession.symtab()->get_symbol(ctx, sym_name);
    if (sym == nullptr) {
      std::cout << "No symbol \"" << sym_name << "\" in current context." << std::endl;
    } else {
      sym->print(format, expr);
    }
    return true;
  }

  bool CmdRegisters::info(ParseCmd::Args cmd) {
    if (cmd.empty()) {
      /*
		info registers
				PC  : 0x0031  RegisterBank 0:
				R0-7: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
				ACC : 0x00 0 .
				B   : 0x00 0 .
				DPTR: 0x0000 0
				SP  : 0x07
				PSW : 0x00 | CY : 0 | AC : 0 | OV : 0 | P : 0
		*/
      unsigned char reg_psw, reg_acc, reg_b, reg_dpl, reg_dph, reg_sp;
      unsigned char reg_set[8], reg_bank;
      uint16_t reg_dptr;
      gSession.target()->read_sfr(0xd0, 1, &reg_psw);
      reg_bank = (reg_psw >> 3) & 0x03;
      printf("PC  : 0x%04x  RegisterBank %i:\n",
             gSession.target()->read_PC(), reg_bank);

      // dump the regs
      gSession.target()->read_data(reg_bank * 8, 8, reg_set);
      printf("R0-7:");
      for (int i = 0; i < 8; i++)
        printf(" 0x%02x", reg_set[i]);
      printf("\n");

      // ACC
      gSession.target()->read_sfr(0xe0, 1, &reg_acc);
      printf("ACC : 0x%02x %i %c\n", reg_acc, reg_acc, isprint(reg_acc) ? reg_acc : '.');

      // B
      gSession.target()->read_sfr(0xf0, 1, &reg_b);
      printf("B   : 0x%02x %i %c\n", reg_b, reg_b, isprint(reg_b) ? reg_b : '.');

      // DPTR
      gSession.target()->read_sfr(0x82, 1, &reg_dpl);
      gSession.target()->read_sfr(0x83, 1, &reg_dph);
      reg_dptr = (uint16_t(reg_dph) << 8) | reg_dpl;
      printf("DPTR: 0x%04x %i\n", reg_dptr, reg_dptr);

      // SP
      gSession.target()->read_sfr(0x81, 1, &reg_sp);
      printf("SP  : 0x%02x\n", reg_sp);

      printf("PSW : 0x%02x | CY : %i | AC : %i | OV : %i | P : %i\n",
             reg_psw,
             (reg_psw >> 7) & 1, // CY
             (reg_psw >> 6) & 1, // AC
             (reg_psw >> 2) & 1, // OV
             reg_psw & 1);       // P
      return true;
    }
    return false;
  }
} // namespace debug