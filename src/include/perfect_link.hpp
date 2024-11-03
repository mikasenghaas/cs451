#pragma once

// C++ standard library headers
#include <string>
#include <vector>
#include <map>

// C system headers
#include <netinet/in.h>
#include <sys/socket.h>

// Project headers
#include "message.hpp"

struct ConnectionState
{
  bool current_seq_num = 0;
  bool expected_seq_num = 0;
};

using ID = size_t;

/**
 * @brief Perfect link class
 *
 * Send and receive messages over a network reliably.
 */
class PerfectLink
{
private:
  int sockfd;                                           // Socket file descriptor
  const char *output_path;                              // Output path
  std::vector<Message> output = {};                     // Sent/ Received messages
  static constexpr size_t BUFFER_SIZE = 65536;          // Buffer size (UDP MTU)
  std::map<ID, ConnectionState> connection_states = {}; // Connection states (ID -> ConnectionState)

public:
  PerfectLink(const struct sockaddr_in &local_addr, const char *output_path)
  {
    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
      throw std::runtime_error("Failed to create socket");
    }

    // Bind socket to local address
    const struct sockaddr *local_addr_ptr = reinterpret_cast<const struct sockaddr *>(&local_addr);
    if (bind(sockfd, local_addr_ptr, sizeof(local_addr)) < 0)
    {
      close(sockfd);
      throw std::runtime_error(std::string("Failed to bind socket: ") + strerror(errno));
    }
  }

  ~PerfectLink()
  {
    stop();

    // Clear output
    for (auto &msg : output)
    {
      delete[] msg.payload;
    }
    output.clear();
  }

  void stop()
  {
    if (sockfd >= 0)
    {
      close(sockfd);
    }
  }

  void send(const Message &msg, const struct sockaddr_in &recv_addr)
  {
    char buffer[BUFFER_SIZE];
    msg.serialize(buffer);

    ssize_t sent = sendto(sockfd, buffer, msg.serialized_size(), 0,
                          reinterpret_cast<const struct sockaddr *>(&recv_addr),
                          sizeof(recv_addr));
    if (sent < 0)
    {
      throw std::runtime_error("Failed to send data");
    }
    Message msg_copy = msg;
    msg_copy.payload = new uint8_t[msg.payload_size];
    memcpy(msg_copy.payload, msg.payload, msg.payload_size);
    output.push_back(msg_copy);
  }

  void receive()
  {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (true)
    {
      // Set up for select()
      fd_set readfds;
      struct timeval tv;

      // Initialize the file descriptor set
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      // Check if there's data to read
      int ready = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);

      if (ready > 0 && FD_ISSET(sockfd, &readfds))
      {
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                          reinterpret_cast<struct sockaddr *>(&sender_addr), &sender_addr_len);

        if (bytes_received > 0)
        {
          Message msg;
          msg.deserialize(buffer);
          Message msg_copy = msg;
          msg_copy.payload = new uint8_t[msg.payload_size];
          memcpy(msg_copy.payload, msg.payload, msg.payload_size);
          output.push_back(msg_copy);
        }
      }
    }
  }
  void write_output()
  {
    // Open output file
    std::ofstream outfile(output_path);
    if (!outfile.is_open())
    {
      throw std::runtime_error("Failed to open output file");
    }

    // Write each message to the output file
    for (const auto &msg : output)
    {
      // TODO: Don't assume type
      size_t message;
      memcpy(&message, msg.payload, msg.payload_size);
      if (type == SENDER)
      {
        outfile << "b " << message << std::endl;
      }
      else
      {
        outfile << "d " << msg.send_id << " " << message << std::endl;
      }
    }

    // Close output file
    outfile.close();
  }
};