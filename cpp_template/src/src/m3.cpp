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
#include "types.hpp"
#include "parser.hpp"
#include "hosts.hpp"
#include "config.hpp"
#include "output.hpp"
#include "message.hpp"
#include "lattice_agreement.hpp"

// Globals
static std::atomic<bool> should_stop(false);
static LatticeAgreement *global_la = nullptr;
static OutputFile *global_output_file = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Set the stop flag
  should_stop = true;

  // Immediately stop network packet processing
  if (global_la != nullptr)
  {
    std::cout << "\nImmediately stopping network packet processing.\n";
    global_la->shutdown();
  }

  if (global_output_file != nullptr)
  {
    std::cout << "Flushing output.\n";
    global_output_file->flush();
  }

  // Exit directly from signal handler
  exit(0);
}

static void laDecide(Proposal proposal) {
  std::string message;
  for (auto value: proposal) {
      message += std::to_string(value) + " ";
  }
  message += "\n";
  std::cout << "laDecide: " << message << std::endl;
  global_output_file->write(message);
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
  LatticeAgreementConfig config(parser.configPath());
  std::cout << "\nLoaded config (r=" << config.get_num_rounds() << ", vs=" << config.get_max_proposal_size() << ", ds=" << config.get_num_distinct_elements() << ")\n\n";

  // Setup local address
  size_t local_id = static_cast<uint8_t>(parser.id());
  Host local_host(local_id, hosts.get_address(local_id));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  // Open output file
  OutputFile output_file(parser.outputPath());
  global_output_file = &output_file;
  std::cout << "Opened output file at " << parser.outputPath() << "\n\n";

  // Instantiate lattice agreement
  LatticeAgreement la(local_host, hosts, laDecide);
  global_la = &la;

  // Start proposing
  std::cout << "Timestamp: " << std::time(nullptr) * 1000 << "\n\n";
  std::cout << "Proposing...\n\n";

  for (size_t round=0; round<config.get_num_rounds(); round++) {
    auto proposal = config.get_next_proposal();
    la.propose(round, proposal);
  }

  // Infinite loop to keep the program running
  while (!should_stop) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
