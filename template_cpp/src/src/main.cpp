#include <chrono>
#include <iostream>
#include <thread>

#include "parser.hpp"
#include "hello.h"
#include "broadcast_app.hpp"

#include <signal.h>

static BroadcastApp *broadcast_app;

static void stop(int)
{
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // set stop_flag to true
  BroadcastApp::stop_flag = true;
  broadcast_app->log_state();

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv)
{
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv);
  parser.parse();

  hello();
  std::cout << std::endl;

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts)
  {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n\n";

  std::cout << "Path to config:\n";
  std::cout << "===============\n";
  std::cout << parser.configPath() << "\n\n";

  std::cout << "Doing some initialization...\n\n";

  std::vector<Process> processes;
  for (auto &h : hosts)
  {
    Process p;
    p.id = static_cast<int>(h.id);
    p.ip = h.ip;
    p.port = h.port;

    processes.push_back(p);
  }

  int nb_msg = 0, dest_id = 0, loc_id = 0;
  parser.confs(nb_msg, dest_id);
  Config confs;
  confs.nb_msgs = nb_msg;
  // confs.dest_id = dest_id; //needed for perfect link
  loc_id = static_cast<int>(parser.id());

  broadcast_app = new BroadcastApp(confs, processes, loc_id, parser.outputPath());

  std::cout << "Broadcasting and delivering messages...\n\n";

  broadcast_app->start();

  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
