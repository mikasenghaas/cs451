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
#include "fifo_uniform_reliable_broadcast.hpp"

// Globals
static std::atomic<bool> should_stop(false);
static FIFOUniformReliableBroadcast *global_frb = nullptr;
static OutputFile *global_output_file = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Set the stop flag
  should_stop = true;

  // Immediately stop network packet processing
  if (global_frb != nullptr)
  {
    std::cout << "\nImmediately stopping network packet processing.\n";
    global_frb->shutdown();
  }

  if (global_output_file != nullptr)
  {
    std::cout << "Flushing output.\n";
    global_output_file->flush();
  }

  // Exit directly from signal handler
  exit(0);
}

static void frbBroadcast(StringMessage m)
{
  // std::cout << "b " << msg.get_message() << std::endl;
  global_output_file->write("b " + m.get_message() + "\n");
}

static void frbDeliver(BroadcastMessage bm)
{
  StringMessage sm(bm.get_payload());
  std::string message = sm.get_message();
  global_output_file->write("d " + std::to_string(bm.get_source_id()) + " " + message + "\n");
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
  FIFOUniformReliableBroadcastConfig config(parser.configPath());
  std::cout << "\nLoaded config (p=" << config.get_message_count() << ")\n\n";

  // Setup local address
  size_t local_id = static_cast<uint8_t>(parser.id());
  Host local_host(local_id, hosts.get_address(local_id));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  // Open output file
  OutputFile output_file(parser.outputPath());
  global_output_file = &output_file;
  std::cout << "Opened output file at " << parser.outputPath() << "\n\n";

  // Instantiate lattice agreement
  FIFOUniformReliableBroadcast frb(local_host, hosts, frbDeliver);
  global_frb = &frb;

  // Start broadcasting and delivering messages
  std::cout << "Timestamp: " << std::time(nullptr) * 1000 << "\n\n";
  std::cout << "Broadcasting and delivering messages...\n\n";

  for (int i = 1; i <= config.get_message_count(); i++) {
    StringMessage m(std::to_string(i));
    frb.broadcast(m);
    frbBroadcast(m);
  }

  // Infinite loop to keep the program running
  while (!should_stop) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
