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
#include "target.h"

namespace fs = std::filesystem;

static const dap::integer threadId = 100;
const dap::integer variablesReferenceId = 300;

void Event::wait() {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&] { return fired; });
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
    gSession.bpmgr()->reload_all();

    if (gSession.bpmgr()->set_breakpoint("main", true) == BP_ID_INVALID)
      std::cout << " failed to set main breakpoint!" << std::endl;

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
    if (request.variablesReference != variablesReferenceId) {
      return dap::Error("Unknown variablesReference '%d'",
                        int(request.variablesReference));
    }

    dap::VariablesResponse response;

    auto ctx = gSession.contextmgr()->get_current();

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

    auto symbols = gSession.symtab()->getSymbols(ctx);
    for (auto sym : symbols) {
      dap::Variable var;
      var.name = sym->name();
      var.value = sym->sprint(0);
      var.type = sym->type();

      response.variables.push_back(var);
    }

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

  return true;
}

int DapServer::run() {
  configured.wait();

  gSession.target()->run_to_bp();
  ADDR addr = gSession.target()->read_PC();
  gSession.bpmgr()->stopped(addr);
  gSession.contextmgr()->set_context(addr);
  gSession.contextmgr()->dump();

  dap::StoppedEvent event;
  event.reason = "breakpoint";
  event.threadId = threadId;
  session->send(event);

  terminate.wait();
  return 0;
}

void DapServer::onError(const char *msg) {
  fmt::print("dap error: {}", msg);
}