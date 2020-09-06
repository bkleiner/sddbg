#pragma once

#include <condition_variable>
#include <mutex>

#include <dap/network.h>
#include <dap/protocol.h>
#include <dap/session.h>

#include "context_mgr.h"

namespace dap {
  class LaunchRequestEx : public LaunchRequest {
  public:
    string program;
    optional<string> source_folder;
    optional<boolean> attach;
  };

} // namespace dap

namespace debug {

  namespace core {
    class symbol;
  }

  class event {
  public:
    // wait() blocks until the event is fired.
    void wait();

    // fire() sets signals the event, and unblocks any calls to wait().
    void fire();

  private:
    std::mutex mutex;
    std::condition_variable cv;
    bool fired = false;
  };

  class state_event {
  public:
    enum states {
      IDLE,
      CONTINUE,
      PAUSE,
      NEXT,
      STEP_IN,
      STEP_OUT,
      EXIT
    };

    // wait() blocks until the event is fired.
    states wait();

    // fire() sets signals the event, and unblocks any calls to wait().
    void fire(states s);

  private:
    std::mutex mutex;
    std::condition_variable cv;
    states event = IDLE;
  };

  class dap_server {
  public:
    dap_server();

    bool start();
    int run();

  private:
    bool log_traffic = false;

    std::mutex mutex;

    event terminate;
    event configured;

    std::string src_dir;
    std::string base_dir;

    bool should_continue = true;
    state_event do_continue;

    std::shared_ptr<dap::Session> session;
    std::unique_ptr<dap::net::Server> server;

    template <typename T>
    void registerHandler() {
      using ResponseType = typename T::Response;
      session->registerHandler([&](const T &req) -> dap::ResponseOrError<ResponseType> {
        return this->handle(req);
      });
    }

    void on_error(const char *msg);
    void on_connect(const std::shared_ptr<dap::ReaderWriter> &client);

    dap::Variable variable_from_symbol(core::context ctx, core::symbol *sym);

    dap::ResponseOrError<dap::VariablesResponse> handle(const dap::VariablesRequest &request);
    dap::ResponseOrError<dap::ThreadsResponse> handle(const dap::ThreadsRequest &);
    dap::ResponseOrError<dap::StackTraceResponse> handle(const dap::StackTraceRequest &request);
    dap::ResponseOrError<dap::ScopesResponse> handle(const dap::ScopesRequest &request);
    dap::ResponseOrError<dap::EvaluateResponse> handle(const dap::EvaluateRequest &req);
    dap::ResponseOrError<dap::SetBreakpointsResponse> handle(const dap::SetBreakpointsRequest &request);
    dap::ResponseOrError<dap::SourceResponse> handle(const dap::SourceRequest &req);
    dap::ResponseOrError<dap::LaunchResponse> handle(const dap::LaunchRequestEx &request);
    dap::ResponseOrError<dap::ContinueResponse> handle(const dap::ContinueRequest &);
    dap::ResponseOrError<dap::NextResponse> handle(const dap::NextRequest &);
    dap::ResponseOrError<dap::StepInResponse> handle(const dap::StepInRequest &req);
    dap::ResponseOrError<dap::StepOutResponse> handle(const dap::StepOutRequest &req);
    dap::ResponseOrError<dap::PauseResponse> handle(const dap::PauseRequest &req);
  };

} // namespace debug