#pragma once

#include <condition_variable>
#include <mutex>

#include <dap/network.h>
#include <dap/session.h>

namespace debug {

  class Event {
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

  class DapServer {
  public:
    DapServer();

    bool start();
    int run();

  private:
    Event terminate;
    Event configured;

    std::string base_dir;

    bool should_continue = true;
    Event do_continue;

    std::unique_ptr<dap::Session> session;
    std::unique_ptr<dap::net::Server> server;

    void onError(const char *msg);
  };

} // namespace debug