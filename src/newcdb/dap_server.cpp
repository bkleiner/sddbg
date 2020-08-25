#include "dap_server.h"

#include <dap/protocol.h>
#include <filesystem>
#include <fmt/format.h>
#include <functional>

#include "newcdb.h"

#include "breakpointmgr.h"
#include "contextmgr.h"
#include "module.h"
#include "symtab.h"
#include "symtypetree.h"
#include "target.h"

namespace fs = std::filesystem;

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
    gSession.target()->stop();
    gSession.target()->disconnect();
    gSession.target()->connect();
    gSession.target()->reset();

    session->send(dap::InitializedEvent());
  });

  session->registerHandler([&](const dap::ConfigurationDoneRequest &) {
    gSession.bpmgr()->reload_all();

    if (gSession.bpmgr()->set_breakpoint("main", true) == BP_ID_INVALID)
      std::cout << " failed to set main breakpoint!" << std::endl;

    configured.fire();
    return dap::ConfigurationDoneResponse();
  });

  session->registerHandler([&](const dap::ThreadsRequest &) {
    dap::ThreadsResponse response;
    dap::Thread thread;
    thread.id = threadId;
    thread.name = "TheThread";
    response.threads.push_back(thread);
    return response;
  });

  session->registerHandler([&](const dap::StackTraceRequest &request) -> dap::ResponseOrError<dap::StackTraceResponse> {
    if (request.threadId != threadId) {
      return dap::Error("Unknown threadId '%d'", int(request.threadId));
    }

    auto ctx = gSession.contextmgr()->get_current();

    dap::Source source;
    source.name = ctx.module + ".c";
    source.path = fs::absolute(ctx.module + ".c");

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

  session->registerHandler([&](const dap::ScopesRequest &request) -> dap::ResponseOrError<dap::ScopesResponse> {
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
        if (sym->is_type(Symbol::ARRAY)) {
          var.variablesReference = int32_t(std::hash<std::string>{}(sym->name()));
          var.type = "array";
          var.indexedVariables = sym->array_size();
        } else if (sym->is_type(Symbol::STRUCT)) {
          var.variablesReference = int32_t(std::hash<std::string>{}(sym->name()));
          auto type = dynamic_cast<SymTypeStruct *>(gSession.symtree()->get_type(sym->type_name(), ctx));
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

    auto symbols = gSession.symtab()->get_symbols(ctx);
    for (auto sym : symbols) {
      if (request.variablesReference != int32_t(std::hash<std::string>{}(sym->name()))) {
        continue;
      }
      if (sym->is_type(Symbol::ARRAY)) {
        for (uint32_t i = 0; i < sym->array_size(); i++) {
          dap::Variable var;
          var.name = std::to_string(i);
          var.value = std::to_string(i);
          var.type = sym->type_name();
          response.variables.push_back(var);
        }
        return response;
      } else if (sym->is_type(Symbol::STRUCT)) {
        auto type = dynamic_cast<SymTypeStruct *>(gSession.symtree()->get_type(sym->type_name(), ctx));
        for (auto &m : type->get_members()) {
          dap::Variable var;
          var.name = m.member_name;
          var.value = std::to_string(m.offset);
          var.type = m.type_name;
          response.variables.push_back(var);
        }
        return response;
      } else {
        dap::Variable var;
        var.variablesReference = int32_t(std::hash<std::string>{}(sym->name()));
        var.name = sym->name();
        var.value = sym->sprint(0);
        response.variables.push_back(var);
        return response;
      }
    }

    return dap::Error("Unknown variablesReference '%d'",
                      int(request.variablesReference));
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
      response.breakpoints[i].verified = bp_id != BP_ID_INVALID;
    }

    gSession.bpmgr()->reload_all();

    return response;
  });

  session->registerHandler([&](const dap::NextRequest &) {
    ADDR addr = gSession.target()->read_PC();

    std::string module;
    LINE_NUM line;
    gSession.modulemgr()->get_c_addr(addr, module, line);
    ContextMgr::Context context = gSession.contextmgr()->get_current();

    // keep stepping over asm instructions until we hit another c line in the current function
    LINE_NUM current_line = line;
    while (line == current_line) {
      addr = gSession.target()->step();
      gSession.bpmgr()->stopped(addr);
      gSession.contextmgr()->set_context(addr);

      LINE_NUM new_line;
      if (gSession.modulemgr()->get_c_addr(addr, module, new_line)) {
        const auto current_context = gSession.contextmgr()->get_current();

        if (current_context.module == context.module && current_context.function == context.function)
          current_line = new_line;
      }
    }

    dap::StoppedEvent event;
    event.reason = "step";
    event.threadId = threadId;
    session->send(event);

    return dap::NextResponse();
  });

  session->registerHandler([&](const dap::SourceRequest &request) -> dap::ResponseOrError<dap::SourceResponse> {
    return dap::Error("not implemented");
  });

  session->registerHandler([&](const dap::LaunchRequest &) {
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
    ADDR addr = gSession.target()->read_PC();
    gSession.bpmgr()->stopped(addr);
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
  fmt::print("dap error: {}", msg);
}