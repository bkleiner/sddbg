#include "dap_server.h"

#include <dap/protocol.h>
#include <filesystem>
#include <fmt/format.h>
#include <functional>

#include "sddbg.h"

#include "breakpoint_mgr.h"
#include "cdb_file.h"
#include "context_mgr.h"
#include "module.h"
#include "sym_tab.h"
#include "sym_type_tree.h"
#include "target.h"

namespace fs = std::filesystem;

namespace dap {

  class LaunchRequestEx : public LaunchRequest {
  public:
    string program;
  };

  DAP_STRUCT_TYPEINFO_EXT(LaunchRequestEx,
                          LaunchRequest,
                          "launch",
                          DAP_FIELD(program, "program"));

} // namespace dap

namespace debug {

  static const dap::integer threadId = 100;
  const dap::integer variablesReferenceId = 300;

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
      session->bind(client);
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
      gSession.bpmgr()->reload_all();

      if (gSession.bpmgr()->set_breakpoint("main", true) == core::BP_ID_INVALID)
        std::cout << " failed to set main breakpoint!" << std::endl;

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

      auto ctx = gSession.contextmgr()->get_current();

      dap::Source source;
      source.name = ctx.module + ".c";
      source.path = fs::absolute(fs::path(base_dir).append(ctx.module + ".c"));

      dap::StackFrame frame;
      frame.line = ctx.c_line;
      frame.column = 1;
      frame.name = ctx.function;
      frame.id = ctx.level;
      frame.source = source;

      dap::StackTraceResponse response;
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

    session->registerHandler([&](const dap::VariablesRequest &request)
                                 -> dap::ResponseOrError<dap::VariablesResponse> {
      dap::VariablesResponse response;
      auto ctx = gSession.contextmgr()->get_current();

      if (request.variablesReference == variablesReferenceId) {
        {
          dap::Variable var;
          var.name = "ctx_block";
          var.value = std::to_string(ctx.block);
          var.type = "int";

          response.variables.push_back(var);
        }

        {
          dap::Variable var;
          var.name = "ctx_level";
          var.value = std::to_string(ctx.level);
          var.type = "int";

          response.variables.push_back(var);
        }

        auto symbols = gSession.symtab()->get_symbols(ctx);
        for (auto sym : symbols) {
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
      dap::SetBreakpointsResponse response;

      auto breakpoints = request.breakpoints.value({});
      response.breakpoints.resize(breakpoints.size());

      auto ctx = gSession.contextmgr()->get_current();
      gSession.bpmgr()->clear_all();

      for (size_t i = 0; i < breakpoints.size(); i++) {
        const auto &bp = breakpoints[i];
        auto bp_id = gSession.bpmgr()->set_breakpoint(ctx.module + ".c:" + std::to_string(bp.line));
        response.breakpoints[i].verified = bp_id != core::BP_ID_INVALID;
      }

      gSession.bpmgr()->reload_all();

      return response;
    });

    session->registerHandler([&](const dap::NextRequest &) {
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

      dap::StoppedEvent event;
      event.reason = "step";
      event.threadId = threadId;
      session->send(event);

      return dap::NextResponse();
    });

    session->registerHandler([&](const dap::SourceRequest &request)
                                 -> dap::ResponseOrError<dap::SourceResponse> {
      return dap::Error("not implemented");
    });

    session->registerHandler([&](const dap::LaunchRequestEx &request) {
      gSession.target()->stop();

      gSession.modulemgr()->reset();
      gSession.symtab()->clear();
      gSession.symtree()->clear();
      gSession.bpmgr()->clear_all();

      gSession.target()->disconnect();
      gSession.target()->connect();

      gSession.target()->reset();

      auto file = request.program;
      base_dir = fs::path(file).parent_path().string();

      core::cdb_file cdbfile(&gSession);
      cdbfile.open(file + ".cdb");

      gSession.target()->load_file(file + ".ihx");

      return dap::LaunchResponse();
    });

    session->registerHandler([&](const dap::ContinueRequest &) {
      do_continue.fire();
      return dap::ContinueResponse();
    });

    return true;
  }

  int DapServer::run() {
    configured.wait();

    while (should_continue) {
      gSession.target()->run_to_bp();
      core::ADDR addr = gSession.target()->read_PC();
      gSession.contextmgr()->set_context(addr);
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