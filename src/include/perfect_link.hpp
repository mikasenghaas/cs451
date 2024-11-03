#pragma once

// C++ standard library headers
#include <string>
#include <vector>

// C system headers
#include <netinet/in.h>
#include <sys/socket.h>

// Project headers
#include "globals.hpp"
#include "message.hpp"

/**
 * @brief Perfect link class
 *
 * Send and receive messages over a network reliably.
 */
class PerfectLink
{
private:
  LinkType type;                               // Link type (SENDER or RECEIVER)
  int sockfd;                                  // Socket file descriptor
  struct sockaddr_in local_addr;               // Local address
  std::vector<Message> output = {};            // Sent/ Received messages
  static constexpr size_t BUFFER_SIZE = 65536; // Buffer size
  const char *output_path;                     // Output path

public:
  PerfectLink(LinkType type, const char *output_path, const struct sockaddr_in &local_addr);
  ~PerfectLink();

  void stop();
  void send(const Message &msg, const struct sockaddr_in &recv_addr);
  void receive();
  void write_output();
};