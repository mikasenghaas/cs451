#pragma once

#include "message.hpp"
#include "send_buffer.hpp"

#define MAX_RECEIVE_BUFFER_SIZE 65535
#define MAX_SEND_BUFFER_SIZE 65536

/**
 * @brief FairLossLink (UDP)
 *
 * @details Send and receive messages over a network with fair loss (UDP)
 */
class FairLossLink
{
private:
  Host host;
  // SendBuffer send_buffer;
  bool continue_receiving = true;
  int sockfd;

public:

  FairLossLink(Host host, Hosts hosts) : host(host)
  {
    // std::cout << "Setting up fair loss link at " << host.get_address().to_string() << std::endl;
    // Create socket
    this->sockfd = create_socket();
  }

  void send(TransportMessage tm)
  {
    // std::cout << "flSend: " << tm << std::endl;
    
    // Serialize message
    uint64_t payload_length;
    auto payload = tm.serialize(payload_length);

    // Send message
    sockaddr_in address = tm.get_receiver().get_address().to_sockaddr();
    sendto(this->sockfd, payload.get(), payload_length, 0,
                reinterpret_cast<sockaddr *>(&address), sizeof(address));
  }

  void start_receiving(std::function<void(TransportMessage)> handler)
  {
    // std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    char buffer[MAX_RECEIVE_BUFFER_SIZE];
    sockaddr_in source;
    socklen_t source_length = sizeof(source);

    while (this->continue_receiving)
    {
      // Receive message
      auto num_bytes = recvfrom(this->sockfd, buffer, MAX_RECEIVE_BUFFER_SIZE, 0,
                               reinterpret_cast<sockaddr *>(&source), &source_length);
      
      if (num_bytes < 0) {
        break;
      }

      // Deserialize message
      TransportMessage tm = TransportMessage::deserialize(buffer, num_bytes);
      // std::cout << "flDeliver: " << tm << std::endl;
      handler(tm);
    }

    // std::cout << "Closing socket at " << host.get_address().to_string() << "\n";

    // Close socket
    close(this->sockfd);
  }

  void stop_receiving()
  {
    this->continue_receiving = false;
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
