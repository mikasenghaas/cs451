#pragma once

// C++ standard library headers
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdlib>

// C system headers
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// C++ standard library headers
#include <fstream>

// Project headers
#include "address.hpp"
#include "message.hpp"

#define MAX_RECEIVE_BUFFER_SIZE 65535
#define MAX_SEND_BUFFER_SIZE 65536

/**
 * @brief Fair loss link class
 *
 * Send and receive messages over a network with fair loss (UDP)
 */
class FairLossLink
{
private:
  int sockfd; // Socket file descriptor
  // SendBuffer send_buffer;         // Buffer for sending messages
  bool continue_receiving = true; // Whether to continue receiving

public:
  const Host host;

  FairLossLink(Host host) : host(host)
  {
    // Create socket
    this->sockfd = create_socket();
  }

  void send(const Message &message)
  {
    // Serialize message
    uint64_t serialized_length = 0;
    std::unique_ptr<char[]> payload = message.serialize(serialized_length);

    // Get address
    sockaddr_in address = message.get_receiver().get_address().to_sockaddr();

    // Send message
    ssize_t sent = sendto(sockfd, payload.get(), serialized_length, 0,
                          reinterpret_cast<const struct sockaddr *>(&address),
                          sizeof(address));
  }

  void start_receiving(std::function<void(Message)> handler)
  {
    // Create receive buffer
    char buffer[MAX_RECEIVE_BUFFER_SIZE];

    // Setup source address
    sockaddr_in source;
    socklen_t source_length = sizeof(source);

    // Receive message
    auto received_length = recvfrom(this->sockfd, buffer, MAX_RECEIVE_BUFFER_SIZE, 0,
                                    reinterpret_cast<sockaddr *>(&source), &source_length);

    while (received_length >= 0)
    {
      // Deserialize message
      Message message = Message::deserialize(buffer, received_length);

      // Handle message
      handler(message);

      // Stop receiving if flag is set
      if (!this->continue_receiving)
      {
        break;
      }

      // Receive next message
      received_length =
          recvfrom(this->sockfd, buffer, MAX_RECEIVE_BUFFER_SIZE, 0,
                   reinterpret_cast<sockaddr *>(&source), &source_length);
    }

    // Close socket
    close(this->sockfd);
    std::cout << "Stopped receiveing on address: " << this->host.get_address().to_string() << std::endl;
  }

  void stop_receiving()
  {
    this->continue_receiving = false;
    std::cout << "Stopping receiving on address: " << this->host.get_address().to_string() << std::endl;
  }

private:
  int create_socket()
  {
    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
      throw std::runtime_error("Failed to create socket at " + this->host.get_address().to_string());
    }

    // Bind socket
    auto sock_addr = this->host.get_address().to_sockaddr();
    if (bind(sockfd, reinterpret_cast<sockaddr *>(&sock_addr), sizeof(sock_addr)) == -1)
    {
      throw std::runtime_error("Failed to bind socket at " + host.get_address().to_string());
    }

    return sockfd;
  }
};
