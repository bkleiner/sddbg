#include "dap_server.h"

#include <dap/protocol.h>
#include <filesystem>
#include <fmt/format.h>
#include <functional>

#include "sddbg.h"

#include "breakpoint_mgr.h"
#include "cdb_file.h"
#include "context_mgr.h"
#include "disassembly.h"
#include "log.h"
#include "module.h"
#include "registers.h"
#include "sym_tab.h"
#include "sym_type_tree.h"
#include "target.h"

namespace fs = std::filesystem;

namespace dap {

  DAP_STRUCT_TYPEINFO_EXT(LaunchRequestEx,
                          LaunchRequest,
                          "launch",
                          DAP_FIELD(program, "program"),
                          DAP_FIELD(source_folder, "source_folder"));

} // namespace dap

namespace debug {

  static const dap::integer threadId = 100;
  static const int64_t localVariablesReferenceId = 100;
  static const int64_t registerVariablesReferenceId = 200;
  static const dap::integer disassemblyReferenceId = 1337;

  void event::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] {
      if (fired) {
        fired = false;
        return true;
      }
      return false;
    });
  }

  void event::fire() {
    std::unique_lock<std::mutex> lock(mutex);
    fired = true;
    cv.notify_all();
  }

  state_event::states state_event::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    states s = IDLE;
    cv.wait(lock, [&] {
      if (s != event) {
        s = event;
        event = IDLE;
        return true;
      }
      return false;
    });
    return s;
  }

  void state_event::fire(states s) {
    std::unique_lock<std::mutex> lock(mutex);
    event = s;
    cv.notify_all();
  }

  class dap_logger : public core::log::logger {
  public:
    dap_logger(std::weak_ptr<dap::Session> session)
        : session(session) {}

    void write(std::string str) override {
      std::fputs(str.c_str(), stdout);
      if (auto s = session.lock()) {
        dap::OutputEvent evt;
        evt.output = str;
        s->send(evt);
      }
    }

  private:
    std::weak_ptr<dap::Session> session;
  };

  dap_server::dap_server() {
  }

  dap::ResponseOrError<dap::VariablesResponse> dap_server::handle(const dap::VariablesRequest &request) {
    std::unique_lock<std::mutex> lock(mutex);

    dap::VariablesResponse response;
    auto ctx = gSession.contextmgr()->get_current();

    switch (request.variablesReference) {
    case localVariablesReferenceId: {
      auto symbols = gSession.symtab()->get_symbols(ctx);
      for (auto sym : symbols) {
        if (sym->get_scope().typ == core::symbol_scope::GLOBAL) {
          continue;
        }

        dap::Variable var;
        var.name = sym->name();
        var.value = sym->sprint(0);
        if (sym->is_type(core::symbol::ARRAY)) {
          var.variablesReference = sym->short_hash();
          var.type = "array";
          var.indexedVariables = sym->array_size();
        } else if (sym->is_type(core::symbol::STRUCT)) {
          var.variablesReference = sym->short_hash();
          auto type = dynamic_cast<core::sym_type_struct *>(gSession.symtree()->get_type(sym->type_name(), ctx));
          if (type) {
            var.type = "struct";
            var.namedVariables = type->get_members().size();
          }
        } else {
          var.type = sym->type_name();
        }

        response.variables.push_back(var);
      }
      break;
    }

    case registerVariablesReferenceId: {
      for (auto &reg : gSession.regs()->get_registers()) {
        dap::Variable var;
        var.name = reg.str;
        var.value = gSession.regs()->print(reg.name);
        var.type = "char";

        response.variables.push_back(var);
      }
      break;
    }

    default: {
      auto sym = gSession.symtab()->get_symbol(request.variablesReference);
      if (sym == nullptr) {
        return dap::Error("Unknown variablesReference '%d'", int(request.variablesReference));
      }

      if (sym->is_type(core::symbol::ARRAY)) {
        auto type = gSession.symtree()->get_type(sym->type_name(), ctx);
        for (uint32_t i = 0; i < sym->array_size(); i++) {
          dap::Variable var;
          var.name = std::to_string(i);
          var.value = type->pretty_print(0, sym->addr() + core::ADDR(i * type->size()));
          var.type = sym->type_name();
          response.variables.push_back(var);
        }
      } else if (sym->is_type(core::symbol::STRUCT)) {
        auto type = dynamic_cast<core::sym_type_struct *>(gSession.symtree()->get_type(sym->type_name(), ctx));
        for (auto &m : type->get_members()) {
          core::sym_type *member_type = type->get_member_type(m.member_name);
          dap::Variable var;
          var.name = m.member_name;
          var.value = member_type->pretty_print(0, sym->addr() + m.offset);
          var.type = m.type_name;
          response.variables.push_back(var);
        }
      } else {
        dap::Variable var;
        var.variablesReference = sym->short_hash();
        var.name = sym->name();
        var.value = sym->sprint(0);
        response.variables.push_back(var);
      }
      break;
    }
    }

    return response;
  }

  dap::ResponseOrError<dap::ThreadsResponse> dap_server::handle(const dap::ThreadsRequest &) {
    dap::ThreadsResponse response;
    dap::Thread thread;
    thread.id = threadId;
    thread.name = "thread";
    response.threads.push_back(thread);
    return response;
  };

  dap::ResponseOrError<dap::StackTraceResponse> dap_server::handle(const dap::StackTraceRequest &request) {
    if (request.threadId != threadId) {
      return dap::Error("Unknown threadId '%d'", int(request.threadId));
    }

    std::unique_lock<std::mutex> lock(mutex);
    gSession.contextmgr()->update_context();

    dap::StackTraceResponse response;

    for (auto &ctx : gSession.contextmgr()->get_stack()) {
      dap::Source source;
      dap::StackFrame frame;

      if (ctx.c_line) {
        source.name = ctx.module + ".c";
        source.path = fs::absolute(fs::path(src_dir).append(ctx.module + ".c"));
        frame.line = ctx.c_line;
      } else if (ctx.asm_line) {
        source.name = ctx.module + ".asm";
        source.path = fs::absolute(fs::path(base_dir).append(ctx.module + ".asm"));
        frame.line = ctx.asm_line;
      } else {
        source.name = "full.asm";
        source.sourceReference = disassemblyReferenceId;
        frame.line = gSession.disasm()->get_line_number(ctx.addr);
      }

      frame.column = 1;
      frame.name = ctx.function;
      frame.id = ctx.block * 100 + ctx.level;
      frame.source = source;

      response.stackFrames.push_back(frame);
    }

    return response;
  };

  dap::ResponseOrError<dap::ScopesResponse> dap_server::handle(const dap::ScopesRequest &request) {
    dap::ScopesResponse response;
    {
      dap::Scope scope;
      scope.name = "Locals";
      scope.presentationHint = "locals";
      scope.variablesReference = localVariablesReferenceId;
      response.scopes.push_back(scope);
    }
    {
      dap::Scope scope;
      scope.name = "Registers";
      scope.presentationHint = "registers";
      scope.variablesReference = registerVariablesReferenceId;
      response.scopes.push_back(scope);
    }
    return response;
  };

  dap::ResponseOrError<dap::EvaluateResponse> dap_server::handle(const dap::EvaluateRequest &req) {
    std::string expr = req.expression;

    std::string sym_name = expr;
    int seperator_pos = expr.find_first_of(".[");
    if (seperator_pos != -1)
      sym_name = expr.substr(0, seperator_pos);

    // figure out where we are
    const auto ctx = gSession.contextmgr()->get_current();
    core::symbol *sym = gSession.symtab()->get_symbol(ctx, sym_name);
    if (sym == nullptr) {
      return dap::Error("No symbol \"" + sym_name + "\" in current context.");
    }

    dap::EvaluateResponse res;
    if (sym_name == expr && sym->is_type(core::symbol::ARRAY)) {
      res.variablesReference = sym->short_hash();
      res.type = "array";
      res.indexedVariables = sym->array_size();
    } else {
      res.result = sym->sprint(0, expr);
    }

    return res;
  };

  dap::ResponseOrError<dap::SetBreakpointsResponse> dap_server::handle(const dap::SetBreakpointsRequest &request) {
    std::unique_lock<std::mutex> lock(mutex);
    dap::SetBreakpointsResponse response;

    auto breakpoints = request.breakpoints.value({});
    response.breakpoints.resize(breakpoints.size());

    gSession.bpmgr()->clear_all();

    auto ctx = gSession.contextmgr()->get_current();
    for (size_t i = 0; i < breakpoints.size(); i++) {
      const auto &bp = breakpoints[i];
      const std::string module = request.source.name.value(ctx.module + ".c");

      auto bp_id = gSession.bpmgr()->set_breakpoint(module + ":" + std::to_string(bp.line));
      response.breakpoints[i].verified = bp_id != core::BP_ID_INVALID;
    }

    gSession.bpmgr()->reload_all();

    return response;
  };

  dap::ResponseOrError<dap::SourceResponse> dap_server::handle(const dap::SourceRequest &req) {
    if (req.sourceReference == disassemblyReferenceId) {
      dap::SourceResponse res;
      res.content = gSession.disasm()->get_source();
      return res;
    }
    return dap::Error("not implemented");
  };

  dap::ResponseOrError<dap::LaunchResponse> dap_server::handle(const dap::LaunchRequestEx &request) {
    std::unique_lock<std::mutex> lock(mutex);
    try {
      gSession.target()->reset();
    } catch (std::runtime_error e) {
      return dap::Error(e.what());
    }

    gSession.modulemgr()->reset();
    gSession.symtab()->clear();
    gSession.symtree()->clear();

    auto file = request.program;

    src_dir = base_dir = fs::path(file).parent_path().string();
    if (request.source_folder.has_value())
      src_dir = request.source_folder.value();

    if (!gSession.load(file, src_dir)) {
      return dap::Error("error opening " + file + ".cdb");
    }

    if (!gSession.target()->load_file(file + ".ihx")) {
      return dap::Error("target flash failed");
    }

    gSession.bpmgr()->reload_all();
    if (gSession.bpmgr()->set_breakpoint("main", true) == core::BP_ID_INVALID)
      core::log::print("failed to set main breakpoint!\n");

    return dap::LaunchResponse();
  };

  dap::ResponseOrError<dap::ContinueResponse> dap_server::handle(const dap::ContinueRequest &) {
    do_continue.fire(state_event::CONTINUE);
    return dap::ContinueResponse();
  };

  dap::ResponseOrError<dap::NextResponse> dap_server::handle(const dap::NextRequest &) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();
      gSession.target()->check_stop_forced();
    }
    do_continue.fire(state_event::NEXT);
    return dap::NextResponse();
  };

  dap::ResponseOrError<dap::StepInResponse> dap_server::handle(const dap::StepInRequest &req) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();
      gSession.target()->check_stop_forced();
    }
    do_continue.fire(state_event::STEP_IN);
    return dap::StepInResponse();
  };

  dap::ResponseOrError<dap::StepOutResponse> dap_server::handle(const dap::StepOutRequest &req) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();
      gSession.target()->check_stop_forced();
    }
    do_continue.fire(state_event::STEP_OUT);
    return dap::StepOutResponse();
  };

  dap::ResponseOrError<dap::PauseResponse> dap_server::handle(const dap::PauseRequest &req) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();
    }
    do_continue.fire(state_event::PAUSE);
    return dap::PauseResponse();
  };

  void dap_server::on_connect(const std::shared_ptr<dap::ReaderWriter> &client) {
    session = dap::Session::create();

    core::log::log_ouput = std::unique_ptr<core::log::logger>(new dap_logger(session));

    registerHandler<dap::VariablesRequest>();
    registerHandler<dap::ThreadsRequest>();
    registerHandler<dap::StackTraceRequest>();
    registerHandler<dap::ScopesRequest>();
    registerHandler<dap::EvaluateRequest>();
    registerHandler<dap::SetBreakpointsRequest>();
    registerHandler<dap::SourceRequest>();
    registerHandler<dap::LaunchRequestEx>();
    registerHandler<dap::ContinueRequest>();
    registerHandler<dap::NextRequest>();
    registerHandler<dap::StepInRequest>();
    registerHandler<dap::StepOutRequest>();
    registerHandler<dap::PauseRequest>();

    session->registerHandler([](const dap::InitializeRequest &) {
      dap::InitializeResponse response;
      response.supportsConfigurationDoneRequest = true;
      return response;
    });

    session->registerSentHandler([&](const dap::ResponseOrError<dap::InitializeResponse> &) {
      session->send(dap::InitializedEvent());
    });

    session->registerHandler([&](const dap::ConfigurationDoneRequest &) {
      configured.fire();
      return dap::ConfigurationDoneResponse();
    });

    session->registerHandler([&](const dap::DisconnectRequest &req) {
      configured.fire();

      {
        std::unique_lock<std::mutex> lock(mutex);
        should_continue = false;
        gSession.target()->stop();
      }

      do_continue.fire(state_event::EXIT);
      return dap::DisconnectResponse();
    });

    session->registerSentHandler([&](const dap::ResponseOrError<dap::DisconnectResponse> &) {
      terminate.fire();
    });

    if (log_traffic) {
      std::shared_ptr<dap::Writer> log = dap::file(stderr, false);
      session->bind(
          dap::spy(std::dynamic_pointer_cast<dap::Reader>(client), log),
          dap::spy(std::dynamic_pointer_cast<dap::Writer>(client), log));
    } else {
      session->bind(client);
    }
  }

  bool dap_server::start() {
    auto on_error = std::bind(&dap_server::on_error, this, std::placeholders::_1);
    auto on_connect = std::bind(&dap_server::on_connect, this, std::placeholders::_1);

    server = dap::net::Server::create();
    fmt::print("starting dap server at localhost:9543\n");
    if (!server->start(9543, on_connect, on_error)) {
      return false;
    }

    return true;
  }

  int dap_server::run() {
    configured.wait();

    state_event::states state = state_event::CONTINUE;
    while (should_continue) {

      switch (state) {
      case state_event::CONTINUE: {
        gSession.target()->run_to_bp();
        gSession.contextmgr()->update_context();
        gSession.contextmgr()->dump();

        dap::StoppedEvent event;
        event.reason = "breakpoint";
        event.threadId = threadId;
        session->send(event);
        break;
      }
      case state_event::NEXT: {
        const auto ctx = gSession.contextmgr()->update_context();
        auto curr_ctx = ctx;

        core::symbol *function = gSession.symtab()->get_symbol(ctx, ctx.function);
        if (function && function->end_addr() == ctx.addr) {
          // we are already at the end of current function,
          // step and update context to reach the next line
          gSession.target()->step();
          curr_ctx = gSession.contextmgr()->update_context();
          gSession.contextmgr()->dump();
        }

        while (curr_ctx.c_line == ctx.c_line) {
          if (gSession.target()->check_stop_forced()) {
            break;
          }

          gSession.target()->step();
          const auto new_ctx = gSession.contextmgr()->update_context();
          gSession.contextmgr()->dump();

          if (new_ctx.c_line == core::INVALID_LINE) {
            continue;
          }

          if (new_ctx.in_interrupt_handler != ctx.in_interrupt_handler) {
            // bail if we suddently step in into isr
            break;
          }

          if (new_ctx.module == ctx.module && new_ctx.function == ctx.function) {
            curr_ctx = new_ctx;
          }
        }

        dap::StoppedEvent event;
        event.reason = "step";
        event.threadId = threadId;
        session->send(event);
        break;
      }
      case state_event::STEP_IN: {
        gSession.target()->step();
        gSession.contextmgr()->update_context();
        gSession.contextmgr()->dump();

        dap::StoppedEvent event;
        event.reason = "step";
        event.threadId = threadId;
        session->send(event);
        break;
      }
      case state_event::STEP_OUT: {
        auto stack = gSession.contextmgr()->get_stack();
        if (stack.size() <= 1) {
          // no function to step out of, do normal step
          do_continue.fire(state_event::STEP_IN);
          break;
        }

        auto current_ctx = stack[0];
        const auto target_ctx = stack[1];

        while (target_ctx.module != current_ctx.module || target_ctx.function != current_ctx.function) {
          if (gSession.target()->check_stop_forced()) {
            break;
          }

          gSession.target()->step();
          current_ctx = gSession.contextmgr()->update_context();
          gSession.contextmgr()->dump();

          if (target_ctx.in_interrupt_handler != current_ctx.in_interrupt_handler) {
            // bail if we suddently step in into isr
            break;
          }
        }

        dap::StoppedEvent event;
        event.reason = "step";
        event.threadId = threadId;
        session->send(event);
        break;
      }
      case state_event::PAUSE: {
        gSession.contextmgr()->update_context();
        gSession.contextmgr()->dump();

        dap::StoppedEvent event;
        event.reason = "pause";
        event.threadId = threadId;
        session->send(event);
        break;
      }
      default:
        break;
      }

      state = do_continue.wait();
    }

    terminate.wait();
    return 0;
  }

  void dap_server::on_error(const char *msg) {
    fmt::print("dap error: {}\n", msg);
  }

} // namespace debug