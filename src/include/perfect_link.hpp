#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

/**
 * @brief Message struct
 *
 * A message to be sent or received over the network.
 */
struct Message {
  uint8_t* payload;
  size_t payload_size;
  size_t send_id;
  size_t recv_id;

  // ~Message() {
  //   delete[] payload;
  // }

  // Get the total size needed for serialization
  size_t serialized_size() const {
    return sizeof(payload_size) + sizeof(send_id) + sizeof(recv_id) + payload_size;
  }

  // Serialize the message into a buffer
  void serialize(char* buffer) const {
    size_t offset = 0;
    memcpy(buffer + offset, &payload_size, sizeof(payload_size)); offset += sizeof(payload_size);
    memcpy(buffer + offset, &send_id, sizeof(send_id)); offset += sizeof(send_id);
    memcpy(buffer + offset, &recv_id, sizeof(recv_id)); offset += sizeof(recv_id);
    memcpy(buffer + offset, payload, payload_size);
  }

  // Deserialize from a buffer into this message
  void deserialize(const char* buffer) {
    size_t offset = 0;
    memcpy(&payload_size, buffer + offset, sizeof(payload_size)); offset += sizeof(payload_size);
    memcpy(&send_id, buffer + offset, sizeof(send_id)); offset += sizeof(send_id);
    memcpy(&recv_id, buffer + offset, sizeof(recv_id)); offset += sizeof(recv_id);
    payload = new uint8_t[payload_size];
    memcpy(payload, buffer + offset, payload_size);
  }
};

enum LinkType {
  SENDER,
  RECEIVER
};

/**
 * @brief Perfect link class
 *
 * Send and receive messages over a network reliably.
 */
class PerfectLink {
private:
  LinkType type; // Link type (SENDER or RECEIVER)
  int sockfd; // Socket file descriptor
  struct sockaddr_in local_addr; // Local address
  std::vector<Message> output = {}; // Send/ Received messages
  static constexpr size_t BUFFER_SIZE = 65536; // Buffer size

public:
  PerfectLink(LinkType type, const struct sockaddr_in &local_addr);
  ~PerfectLink();
  
  void send(const Message &msg, const struct sockaddr_in &recv_addr);
  void receive();
  void write_output();
};