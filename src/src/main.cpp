#include <chrono>
#include <iostream>
#include <thread>

#include "parser.hpp"
#include "utils.hpp"
#include "perfect_link.hpp"
#include <signal.h>


static void stop(int) {
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // Write/flush output file if necessary
  std::cout << "Writing output.\n";

  // Exit directly from signal handler
  exit(0);
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

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts) {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readable Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readable Port: " << host.port << "\n";
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

  // Load the number of messages to send
  std::string config_path = parser.configPath();
  std::ifstream config_file(config_path);
  size_t num_messages; // TODO: Check what type max messages is
  unsigned long receiver_id;
  config_file >> num_messages >> receiver_id;

  std::cout << "Number of messages to send: " << num_messages << "\n";
  std::cout << "Receiver ID: " << receiver_id << "\n\n";

  // Setup address
  struct sockaddr_in local_addr; // Local address
  struct sockaddr_in recv_addr; // Receiver address

  // Setup local address
  Parser::Host local = parser.hosts()[parser.id()-1];
  get_addr(local.ipReadable(), local.portReadable(), local_addr);

  // Setup receiver address as last host
  Parser::Host receiver = parser.hosts()[parser.hosts().size()-1];
  get_addr(receiver.ipReadable(), receiver.portReadable(), recv_addr);

  // Check if this is the receiver
  bool is_receiver = parser.id() == receiver_id;

  // Setup stubborn link
  std::cout << "Setting up " << (is_receiver ? "receiving" : "sending") << " stubborn link with local address " << local.ipReadable() << ":" << local.portReadable() << "\n\n";
  PerfectLink link(local_addr);

  std::cout << "Broadcasting and delivering messages...\n\n";

  if (is_receiver) {
    std::cout << "Receiving message on " << local.ipReadable() << ":" << local.portReadable() << "\n";
    // Setup buffer with fixed size
    PerfectLink::Message msg = link.receive();
    std::cout << "Received message: " << msg.data << "\n";
  } else {
    for (size_t message = 1; message <= num_messages; message++) {
      std::cout << "Sending message " << message << " to " << inet_ntoa(recv_addr.sin_addr) << "\n";
      PerfectLink::Message msg {
        std::to_string(message),
        recv_addr
      };
      link.send(msg);
    }
  }

  // After a process finishes sending, it waits forever for the delivery of messages.
  while (true) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  std::cout << "Exiting.\n";

  return 0;
}
