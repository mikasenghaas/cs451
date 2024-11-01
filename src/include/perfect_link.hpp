#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * @brief Perfect link class
 *
 * Send and receive messages over a network reliably.
 */
class PerfectLink {
private:
  int sockfd;
  struct sockaddr_in local_addr;

public:
  PerfectLink(const struct sockaddr_in &local_addr);
  ~PerfectLink();

  struct Message {
    std::string data;
    struct sockaddr_in addr;
  };
  void send(const Message &msg);
  Message receive();

};
