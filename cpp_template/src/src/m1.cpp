// C++ standard library headers
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <algorithm>

// C system headers
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Project headers
#include "parser.hpp"
#include "hosts.hpp"
#include "config.hpp"
#include "output.hpp"
#include "message.hpp"
#include "perfect_link.hpp"

// Globals
static std::atomic<bool> should_stop(false);
static PerfectLink *global_pl = nullptr;
static OutputFile *global_output_file = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Set the stop flag
  should_stop = true;

  // Immediately stop network packet processing
  if (global_pl != nullptr)
  {
    std::cout << "\nImmediately stopping network packet processing.\n";
    global_pl->shutdown();
  }

  if (global_output_file != nullptr)
  {
    std::cout << "Flushing output.\n";
    global_output_file->flush();
  }

  // Exit directly from signal handler
  exit(0);
}

static void plSend(StringMessage sm) {
  global_output_file->write("b " + sm.get_message() + "\n");
}

static void plDeliver(TransportMessage tm) {
  auto sender_id = tm.get_sender().get_id();
  auto message = StringMessage(tm.get_payload()).get_message();
  global_output_file->write("d " + std::to_string(sender_id) + " " + message + "\n");
}

int main(int argc, char **argv) {
  // Set up signal handlers
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // Whether a config file is required
  bool requireConfig = true;

  // Parse command line arguments
  Parser parser(argc, argv);
  parser.parse();

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "Doing some initialization...\n\n";

  // Load the hosts file
  Hosts hosts(parser.hostsPath());
  std::string result;
  result = "Loaded hosts (";
  for (auto host : hosts.get_hosts()) {
    result += host.to_string() + ", ";
  }
  result.replace(result.end() - 2, result.end(), ")");
  std::cout << result << "\n";

  // Load the config file
  PerfectLinkConfig config(parser.configPath());
  std::cout << "\nLoaded config (p=" << config.get_message_count() << ", vs=" << config.get_receiver_id() << ")\n\n";

  // Setup local
  Host local_host(parser.id(), hosts.get_address(parser.id()));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  // Setup receiver
  Host receiver_host(config.get_receiver_id(), hosts.get_address(config.get_receiver_id()));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  // Open output file
  OutputFile output_file(parser.outputPath());
  global_output_file = &output_file;
  std::cout << "Opened output file at " << parser.outputPath() << "\n\n";

  // Instantiate perfect link
  PerfectLink pl(local_host, hosts, plDeliver);
  global_pl = &pl;

  // Start broadcasting and delivering messages
  std::cout << "Timestamp: " << std::time(nullptr) * 1000 << "\n\n";
  std::cout << "Broadcasting and delivering messages...\n\n";

  // Send messages
  if (local_host.get_id() != receiver_host.get_id()) {
    for (int i=1; i<=config.get_message_count(); i++) {
      StringMessage sm(std::to_string(i));
      plSend(sm);
      pl.send(sm, receiver_host);
    }
  }

  // Infinite loop to keep the program running
  while (!should_stop) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
