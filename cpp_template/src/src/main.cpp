// C++ standard library headers
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <stdexcept>
#include <netdb.h>

// C system headers
#include <signal.h>

// Project headers
#include "parser.hpp"
#include "hosts.hpp"
#include "host_lookup.hpp"
#include "config.hpp"
#include "output.hpp"
#include "message.hpp"
#include "perfect_link.hpp"

// Globals
static std::atomic<bool> should_stop(false);
static PerfectLink *global_link = nullptr;
static OutputFile *global_output_file = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Set the stop flag
  should_stop = true;

  // Immediately stop network packet processing
  if (global_link != nullptr)
  {
    std::cout << "\nImmediately stopping network packet processing.\n";
    global_link->shutdown();
  }

  if (global_output_file != nullptr)
  {
    std::cout << "Flushing output.\n";
    global_output_file->flush();
  }

  // Exit directly from signal handler
  exit(0);
}

// Define message handler before main
static void write_message(Message msg)
{
  std::cout << "Received message from " << msg.get_sender().get_id() << ": " << msg.get_payload_string() << std::endl;
  global_output_file->write("d " + std::to_string(msg.get_sender().get_id()) + " " + msg.get_payload_string() + "\n");
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

  // std::cout << "My ID: " << parser.id() << "\n\n";

  // std::cout << "List of resolved hosts is:\n";
  // std::cout << "==========================\n";
  // auto hosts = parser.hosts();
  // for (auto &host : hosts)
  // {
  //   std::cout << host.id << "\n";
  //   std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
  //   std::cout << "Machine-readable IP: " << host.ip << "\n";
  //   std::cout << "Human-readable Port: " << host.portReadable() << "\n";
  //   std::cout << "Machine-readable Port: " << host.port << "\n";
  //   std::cout << "\n";
  // }
  // std::cout << "\n";

  // std::cout << "Path to output:\n";
  // std::cout << "===============\n";
  // std::cout << parser.outputPath() << "\n\n";

  // std::cout << "Path to config:\n";
  // std::cout << "===============\n";
  // std::cout << parser.configPath() << "\n\n";

  std::cout << "Doing some initialization...\n\n";

  // Load the hosts file
  Hosts hosts(parser.hostsPath());
  std::cout << "Loaded hosts (" << hosts.get_hosts().size() << " hosts)\n";

  // Configure host lookup
  HostLookup host_lookup(hosts);
  std::cout << "Set up host lookup\n\n";

  // Load the config file
  Config config(parser.configPath());
  std::cout << "Loaded config file (" << config.get_message_count() << " messages to send to " << config.get_receiver_id() << ")\n\n";

  // Open output file
  OutputFile output_file(parser.outputPath());
  global_output_file = &output_file;

  // Setup local address
  size_t local_id = parser.id();
  Host local_host(local_id, host_lookup.get_address(local_id));
  std::cout << "Local address: " << local_host.get_address().to_string() << "\n\n";

  size_t receiver_id = config.get_receiver_id();
  Host receiver_host(receiver_id, host_lookup.get_address(receiver_id));
  std::cout << "Receiver address: " << receiver_host.get_address().to_string() << "\n\n";

  // Setup fair loss link
  PerfectLink pl(local_host, hosts);
  global_link = &pl;
  std::cout << "Set up perfect link at " << local_host.get_address().to_string() << "\n\n";

  // Check if this is the receiver
  bool is_receiver = parser.id() == config.get_receiver_id();
  std::cout << "Is receiver: " << (is_receiver ? "Yes" : "No") << "\n\n";

  std::cout << "Broadcasting and delivering messages...\n\n";

  // Start receiving and sending thread
  auto receiver_thread = pl.start_receiving(write_message);
  auto sender_thread = pl.start_sending();

  // Send messages if sender
  if (!is_receiver)
  {
    for (size_t i = 1; i <= config.get_message_count(); i++)
    {
      Message message(local_host, receiver_host, i);
      pl.send(message);
      output_file.write("b " + message.get_payload_string() + "\n");
    }
  }

  // Infinite loop to keep the program running
  while (!should_stop)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  // Join threads
  receiver_thread.join();
  sender_thread.join();

  return 0;
}
