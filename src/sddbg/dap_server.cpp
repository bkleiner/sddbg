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
#include "module.h"
#include "sym_tab.h"
#include "sym_type_tree.h"
#include "target.h"

namespace fs = std::filesystem;

namespace dap {

  class LaunchRequestEx : public LaunchRequest {
  public:
    string program;
    optional<string> source_folder;
  };

  DAP_STRUCT_TYPEINFO_EXT(LaunchRequestEx,
                          LaunchRequest,
                          "launch",
                          DAP_FIELD(program, "program"),
                          DAP_FIELD(source_folder, "source_folder"));

} // namespace dap

namespace debug {

  static const dap::integer threadId = 100;
  static const dap::integer variablesReferenceId = 300;
  static const dap::integer disassemblyReferenceId = 1337;

  void Event::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] {
      if (fired) {
        fired = false;
        return true;
      }
      return false;
    });
  }

  void Event::fire() {
    std::unique_lock<std::mutex> lock(mutex);
    fired = true;
    cv.notify_all();
  }

  DapServer::DapServer() {
  }

  bool DapServer::start() {
    auto onError = std::bind(&DapServer::onError, this, std::placeholders::_1);
    auto onConnect = [&](const std::shared_ptr<dap::ReaderWriter> &client) {
      std::shared_ptr<dap::Writer> log = dap::file(stderr, false);
      session->bind(
          dap::spy(std::dynamic_pointer_cast<dap::Reader>(client), log),
          dap::spy(std::dynamic_pointer_cast<dap::Writer>(client), log));
    };

    server = dap::net::Server::create();
    if (!server->start(9543, onConnect, onError)) {
      return false;
    }

    session = dap::Session::create();
    session->onError(onError);

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

    session->registerHandler([&](const dap::ThreadsRequest &) {
      dap::ThreadsResponse response;
      dap::Thread thread;
      thread.id = threadId;
      thread.name = "thread";
      response.threads.push_back(thread);
      return response;
    });

    session->registerHandler([&](const dap::StackTraceRequest &request)
                                 -> dap::ResponseOrError<dap::StackTraceResponse> {
      if (request.threadId != threadId) {
        return dap::Error("Unknown threadId '%d'", int(request.threadId));
      }

      std::unique_lock<std::mutex> lock(mutex);
      auto ctx = gSession.contextmgr()->update_context();

      dap::StackTraceResponse response;

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
      return response;
    });

    session->registerHandler([&](const dap::ScopesRequest &request)
                                 -> dap::ResponseOrError<dap::ScopesResponse> {
      dap::ScopesResponse response;

      dap::Scope scope;
      scope.name = "Locals";
      scope.presentationHint = "locals";
      scope.variablesReference = variablesReferenceId;

      response.scopes.push_back(scope);
      return response;
    });

    session->registerHandler([&](const dap::EvaluateRequest &req) -> dap::ResponseOrError<dap::EvaluateResponse> {
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
      res.result = sym->sprint(0, expr);
      return res;
    });

    session->registerHandler([&](const dap::VariablesRequest &request)
                                 -> dap::ResponseOrError<dap::VariablesResponse> {
      std::unique_lock<std::mutex> lock(mutex);
      dap::VariablesResponse response;
      auto ctx = gSession.contextmgr()->get_current();

      if (request.variablesReference == variablesReferenceId) {
        {
          dap::Variable var;
          var.name = "pc";
          var.value = std::to_string(ctx.addr);
          var.type = "int";

          response.variables.push_back(var);
        }

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

        return response;
      }

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
      return response;
    });

    session->registerHandler([&](const dap::SetBreakpointsRequest &request) {
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
    });

    session->registerHandler([&](const dap::NextRequest &) -> dap::ResponseOrError<dap::NextResponse> {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();

      const auto ctx = gSession.contextmgr()->update_context();

      core::bp_id bp = core::BP_ID_INVALID;
      for (size_t tries = 0; tries < 10; tries++) {
        const auto bp_line = ctx.module + ".c:" + std::to_string(ctx.c_line + tries + 1);
        bp = gSession.bpmgr()->set_breakpoint(bp_line, true);
        if (bp != core::BP_ID_INVALID) {
          break;
        }
      }
      if (bp == core::BP_ID_INVALID) {
        return dap::Error("failed to set breakpoint");
      }
      gSession.bpmgr()->reload_all();

      do_continue.fire();

      return dap::NextResponse();
    });

    session->registerHandler([&](const dap::SourceRequest &req) -> dap::ResponseOrError<dap::SourceResponse> {
      if (req.sourceReference == disassemblyReferenceId) {
        dap::SourceResponse res;
        res.content = gSession.disasm()->get_source();
        return res;
      }
      return dap::Error("not implemented");
    });

    session->registerHandler([&](const dap::LaunchRequestEx &request) -> dap::ResponseOrError<dap::LaunchResponse> {
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

      gSession.target()->load_file(file + ".ihx");

      gSession.bpmgr()->reload_all();
      if (gSession.bpmgr()->set_breakpoint("main", true) == core::BP_ID_INVALID)
        std::cout << " failed to set main breakpoint!" << std::endl;

      return dap::LaunchResponse();
    });

    session->registerHandler([&](const dap::ContinueRequest &) {
      do_continue.fire();
      return dap::ContinueResponse();
    });

    session->registerHandler([&](const dap::StepInRequest &req) {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->step();

      core::ADDR addr = gSession.target()->read_PC();
      gSession.contextmgr()->set_context(addr);
      gSession.contextmgr()->dump();

      dap::StoppedEvent event;
      event.reason = "step";
      event.threadId = threadId;
      session->send(event);

      return dap::StepInResponse();
    });

    session->registerHandler([&](const dap::StepOutRequest &req) {
      std::unique_lock<std::mutex> lock(mutex);
      auto ctx = gSession.contextmgr()->get_current();

      core::LEVEL level = ctx.level;
      core::BLOCK block = ctx.block;
      while (level == ctx.level && block == ctx.block) {
        gSession.target()->step();

        core::ADDR addr = gSession.target()->read_PC();
        gSession.contextmgr()->set_context(addr);
        ctx = gSession.contextmgr()->get_current();
        gSession.contextmgr()->dump();
      }

      dap::StoppedEvent event;
      event.reason = "step";
      event.threadId = threadId;
      session->send(event);

      return dap::StepOutResponse();
    });

    session->registerHandler([&](const dap::PauseRequest &req) {
      std::unique_lock<std::mutex> lock(mutex);
      gSession.target()->stop();

      core::ADDR addr = gSession.target()->read_PC();
      gSession.contextmgr()->set_context(addr);
      gSession.contextmgr()->dump();

      dap::StoppedEvent event;
      event.reason = "pause";
      event.threadId = threadId;
      session->send(event);

      return dap::PauseResponse();
    });

    session->registerHandler([&](const dap::DisconnectRequest &req) {
      {
        std::unique_lock<std::mutex> lock(mutex);
        gSession.target()->stop();
        should_continue = false;
      }

      do_continue.fire();
      return dap::DisconnectResponse();
    });

    session->registerSentHandler([&](const dap::ResponseOrError<dap::DisconnectResponse> &) {
      terminate.fire();
    });

    return true;
  }

  int DapServer::run() {
    configured.wait();

    while (should_continue) {
      gSession.target()->run_to_bp();
      gSession.contextmgr()->update_context();
      gSession.contextmgr()->dump();

      dap::StoppedEvent event;
      event.reason = "breakpoint";
      event.threadId = threadId;
      session->send(event);

      do_continue.wait();
    }

    terminate.wait();
    return 0;
  }

  void DapServer::onError(const char *msg) {
    fmt::print("dap error: {}\n", msg);
  }

} // namespace debug