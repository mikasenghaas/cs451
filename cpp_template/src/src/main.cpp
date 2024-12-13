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

// Globals
static std::atomic<bool> should_stop(false);
// static FIFOUniformReliableBroadcast *global_frb = nullptr;
static OutputFile *global_output_file = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Set the stop flag
  should_stop = true;

  // Immediately stop network packet processing
  // if (global_frb != nullptr)
  // {
  //   std::cout << "\nImmediately stopping network packet processing.\n";
  //   global_frb->shutdown();
  // }

  if (global_output_file != nullptr)
  {
    std::cout << "Flushing output.\n";
    global_output_file->flush();
  }

  // Exit directly from signal handler
  exit(0);
}

static void deliver_handler(ProposalMessage pm)
{
  std::string message;
  for (auto value: pm.get_proposal()) {
      message += std::to_string(value) + " ";
  }
  message += "\n";
  // std::cout << "Delivered message " << message << " from " << tm.get_sender().get_id() << std::endl;
  global_output_file->write(message);
}

int main(int argc, char **argv)
{
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
  std::cout << "\nLoaded config (p=" << config.get_num_proposals() << ", vs=" << config.get_max_proposal_size() << ", ds=" << config.get_max_distinct_elements() << ")\n\n";

  auto proposals = config.get_proposals();
  for (uint i=0; i<proposals.size(); i++) {
    std::cout << "Proposal " << i+1 << ": { ";
    for (auto value: proposals.at(i)) {
      std::cout << value << " "; 
    }
    std::cout << "}\n";
  }
  std::cout << "\n";

  // Setup local address
  size_t local_id = static_cast<uint8_t>(parser.id());
  Host local_host(local_id, hosts.get_address(local_id));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  // Create a proposal message
  ProposalMessage p(ProposalMessage::Type::Propose, 0, 0, {1, 2, 3});
  std::cout << p << "\n";

  // Create broadcast message
  BroadcastMessage b(p, local_id);
  std::cout << b << std::endl;

  // Create transport message (in BEB)
  size_t length;
  auto payload = b.serialize(length);
  TransportMessage t(local_host, local_host, payload, length);
  std::cout << t << std::endl;

  // Unpack transport message
  auto b2 = BroadcastMessage(t.get_payload());
  std::cout << b2 << "\n";

  // Unpack broadcast message
  auto p2 = ProposalMessage(b2.get_payload());
  std::cout << p2 << "\n";

  exit(0);

  // Open output file
  // OutputFile output_file(parser.outputPath());
  // global_output_file = &output_file;
  // std::cout << "Opened output file at " << parser.outputPath() << "\n\n";

  // Infinite loop to keep the program running
  while (!should_stop) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
