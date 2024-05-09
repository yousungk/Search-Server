/*
 * Copyright ©2024 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <cstdlib>

#include "./ServerSocket.hpp"
#include "./HttpUtils.hpp"
#include "./ThreadPool.hpp"
#include "./catch.hpp"

using std::cout;
using std::endl;
using std::string;

using searchserver::ServerSocket;
using searchserver::connect_to_server;
using searchserver::rand_port;
using searchserver::ThreadPool;

static uint16_t portnum;

// A task object created to hold the necessary information
// of what information is retrived from the server when
// Accept is run.
class TestServerTask : public ThreadPool::Task {
 public:
  explicit TestServerTask(ThreadPool::thread_task_fn f)
    : ThreadPool::Task(f), task_done(false) { }

  // If the client_fd is open, close it.
  virtual ~TestServerTask() {
    if (accept_fd >= 0) {
      close(accept_fd);
    }
  }

  // public fields to store conneciton information.
  int accept_fd;
  uint16_t cport;
  std::string caddr, cdns, saddr, sdns;
  bool task_done;
};

void TestSSThreadFn(ThreadPool::Task *t) {
  TestServerTask *task = static_cast<TestServerTask *>(t);

  // Create the server socket.
  int listen_fd;
  cout << "Creating ServerSocket on " << portnum << endl;
  ServerSocket ss(portnum);
  cout << "Doing BindAndListen" << endl;
  // note how we are always using AF_INET6
  REQUIRE(ss.bind_and_listen(&listen_fd));

  // Accept a connection.
  cout << "Doing accept..." << endl;
  REQUIRE(ss.accept_client(&task->accept_fd, &task->caddr, &task->cport,
                        &task->cdns, &task->saddr, &task->sdns));

  // It worked!
  cout << "Accept succeeded." << endl;
  task->task_done = true;

  return;
}

TEST_CASE("Basic", "[Test_ServerSocket]") {
  // Create a threadpool, and dispatch a thread to go listen on a
  // server socket on a random port.

  portnum = rand_port();
  ThreadPool tp(1);
  TestServerTask tsk(&TestSSThreadFn);
  tp.dispatch(static_cast<ThreadPool::Task*>(&tsk));

  // Give the thread a chance to create the socket.
  sleep(1);

  // Connect to the socket
  cout << "Attempting to connect to 127.0.0.1 port "
       << portnum << endl;
  int cfd = -1;
  REQUIRE(connect_to_server("127.0.0.1", portnum, &cfd));

  // Make sure that the server had a chance to get client & server info
  while (!tsk.task_done) {
    sleep(1);
  }

  // verify that the file descriptor is valid.
  // cfd needs to be non-negative
  REQUIRE(0 <= cfd );



  // Check that the output params
  // (caddr, cport, cdns, saddr, sdns)
  // are set correctly.
  cout << "Checking output params from Accept..." << endl;

  // check the port...
  REQUIRE(0 < tsk.cport );

  // check the fd...
  REQUIRE(0 < tsk.accept_fd);

  // check the dns results...

  // note that since the server & client are the same machine
  // both values should be localhost
  REQUIRE("localhost" ==  tsk.sdns);
  REQUIRE(tsk.sdns ==  tsk.cdns);

  // check the addresses...

  // in this case, saddr should be ::ffff:127.0.0.1 since this is our
  // local machine and because TestSSThreadFn() sets up server to use AF_INET6
  REQUIRE("::ffff:127.0.0.1" ==  tsk.saddr);
  // client address could be in either ipv4 or ipv6,
  // depending on implementation and ConnectToServer()
  REQUIRE(((tsk.caddr == "::ffff:127.0.0.1") || (tsk.caddr == "127.0.0.1")));

  close(cfd);

}


