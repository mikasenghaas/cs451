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
#include "perfect_link.hpp"

// Globals
static std::atomic<bool> should_stop{false};
static PerfectLink *global_link = nullptr;

static void stop(int)
{
  // Reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  should_stop = true;

  // Immediately stop network packet processing
  if (global_link != nullptr)
  {
    std::cout << "Immediately stopping network packet processing.\n";
    global_link->stop();
  }

  // Write/flush output file if necessary
  if (global_link != nullptr)
  {
    std::cout << "Writing output.\n";
    global_link->write_output();
  }

  // Exit directly from signal handler
  exit(0);
}

static void get_addr(const std::string &ip_or_hostname, unsigned short port, struct sockaddr_in &addr)
{
  // Clear address structure
  std::memset(&addr, 0, sizeof(addr));

  // Setup server address structure
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  // Convert hostname to IP address
  struct hostent *host = gethostbyname(ip_or_hostname.c_str());
  if (host == nullptr)
  {
    throw std::runtime_error("Failed to resolve hostname");
  }

  std::memcpy(&addr.sin_addr, host->h_addr, host->h_length);
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

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts)
  {
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
  size_t send_id;
  size_t recv_id;
  config_file >> num_messages >> recv_id;
  send_id = parser.id();

  config_file.close(); // Close after successful read

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
  PerfectLink link(type, parser.outputPath(), local_addr);
  global_link = &link; // Store pointer to link for signal handler
  std::cout << "Set up " << (type == RECEIVER ? "receiving" : "sending") << " socket at " << local.ipReadable() << ":" << local.portReadable() << "\n\n";

  std::cout << "Broadcasting and delivering messages...\n\n";

  // Send or receive messages
  if (type == SENDER)
  {
    for (size_t message = 1; message <= num_messages && !should_stop; message++)
    {
      uint8_t *bytes = new uint8_t[sizeof(message)];
      memcpy(bytes, &message, sizeof(message));
      Message msg{bytes, sizeof(message), send_id, recv_id};
      link.send(msg, recv_addr);
    }
  }
  else
  {
    while (!should_stop)
    {
      link.receive();
    }
  }

  // Replace infinite loop with a check for shouldStop
  while (!should_stop)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
