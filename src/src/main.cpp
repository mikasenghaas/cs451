#include <chrono>
#include <iostream>
#include <thread>

#include "parser.hpp"
#include "utils.hpp"
#include "perfect_link.hpp"
#include <signal.h>

// Add global variable to access link from signal handler
static PerfectLink* globalLink = nullptr;

static void stop(int) {
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // Immediately stop network packet processing
  if (globalLink != nullptr) {
    std::cout << "\nImmediately stopping network packet processing.\n";
    // TODO: Figure out how to immediately stop network packet processing, but not deconstruct the whole object
  }

  // Write/flush output file if necessary
  if (globalLink != nullptr) {
    std::cout << "\nWriting output.\n";
    globalLink->write_output();
  }

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
  unsigned long send_id;
  unsigned long recv_id;
  config_file >> num_messages >> recv_id;
  send_id = parser.id();

  std::cout << "Number of messages to send: " << num_messages << "\n";
  std::cout << "Sender ID: " << send_id << "\n";
  std::cout << "Receiver ID: " << recv_id << "\n\n";

  // Setup local address
  struct sockaddr_in local_addr; // Local address
  Parser::Host local = parser.hosts()[send_id - 1];
  get_addr(local.ipReadable(), local.portReadable(), local_addr);

  // Setup receiver address
  struct sockaddr_in recv_addr; // Receiver address
  Parser::Host receiver = parser.hosts()[recv_id - 1];
  get_addr(receiver.ipReadable(), receiver.portReadable(), recv_addr);

  // Setup perfect link
  LinkType type = send_id == recv_id ? RECEIVER : SENDER;
  PerfectLink link(type, local_addr);
  globalLink = &link;  // Store pointer to link for signal handler
  std::cout << "Set up " << (type == RECEIVER ? "receiving" : "sending") << " socket at " << local.ipReadable() << ":" << local.portReadable() << "\n\n";

  std::cout << "Broadcasting and delivering messages...\n\n";

  // Send or receive messages
  if (type == SENDER) {
    for (size_t message = 1; message <= num_messages; message++) {

      // TODO: Use utils
      uint8_t* bytes = new uint8_t[sizeof(message)];
      memcpy(bytes, &message, sizeof(message));
      Message msg { bytes, sizeof(message), send_id, recv_id };
      link.send(msg, recv_addr);
    }
  } else {
    link.receive();
  }

  // After a process finishes sending, it waits forever for the delivery of messages.
  while (true) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
