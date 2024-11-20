#pragma once

#include "hosts.hpp"
#include "message.hpp"
#include "fair_loss_link.hpp"

/**
 * @brief PerfectLinkClass
 *
 * @details Send and receive messages over a network reliably
 * using the stop-and-wait for ACK protocol.
 */
class PerfectLink
{
private:
  Host host;
  Hosts hosts;
  FairLossLink link;
  SendBuffer send_buffer;
  bool continue_sending = true;
  std::map<size_t, size_t> seq_nums; // seq_num from sender host_id
  std::map<size_t, std::set<size_t>> acked_messages; // Acked set of messages set<message_id> to receiver host_id
  std::map<size_t, std::set<size_t>> delivered_messages; // Delivered set of messages set<message_id> from sender host_id
  bool has_pending_message = false;
  std::queue<TransportMessage> send_queue; // Queue of messages to send
  std::mutex queue_mutex;
  std::thread sender_thread;
  std::thread receiver_thread;

public:
  PerfectLink(Host host, Hosts hosts) : host(host), hosts(hosts), link(host), send_buffer(hosts, MAX_SEND_BUFFER_SIZE) {
    for (const auto &host : hosts.get_hosts()) {
      if (host.get_id() != this->host.get_id()) {
        seq_nums[host.get_id()] = 1;
        acked_messages[host.get_id()];
        delivered_messages[host.get_id()];
      }
    }
  }

  void send(DataMessage message, Host receiver, const bool &immediate = false)
  {
    std::lock_guard<std::mutex> lock(queue_mutex);

    // Add message to send buffer
    uint64_t payload_length;
    auto payload = send_buffer.add_message(receiver, message, payload_length, immediate);

    if (payload_length == 0) {
      return;
    }

    // Buffer is ready to send
    size_t id = seq_nums[receiver.get_id()]++;
    auto shared_payload = std::shared_ptr<char[]>(new char[payload_length]);
    std::memcpy(shared_payload.get(), payload.get(), payload_length);
    TransportMessage transport_message(id, host, receiver, shared_payload, payload_length, false);
    send_queue.push(transport_message);
  }

  void start_sending()
  {
    std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    this->sender_thread = std::thread([this]() {
      while (this->continue_sending)
      {
        TransportMessage message;
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          if (!send_queue.empty()) {
            message = send_queue.front();
            send_queue.pop();

            size_t receiver_id = message.get_receiver().get_id();
            size_t id = message.get_id();

            if (acked_messages[receiver_id].count(id) == 0) {
              // Send message
              // std::cout << "Sending message " << message << std::endl;
              this->link.send(message);
              send_queue.push(message);
            }
          }
        }
      }
    });
  }


  void start_receiving(std::function<void(DataMessage, Host)> handler)
  {
    std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    this->receiver_thread = std::thread([this, handler]() {
      this->link.start_receiving(
        [this, handler](TransportMessage message) {
          // Get sender and message id
          size_t sender_id = message.get_sender().get_id();
          size_t id = message.get_id();
          if (message.get_is_ack())
          {
            // std::cout << "Received ACK: " << message << std::endl;
            acked_messages[sender_id].insert(id);
            return;
          }

          // Send ACK for received message
          // std::cout << "Received message: " << message << std::endl;
          TransportMessage ack = TransportMessage::create_ack(message);
          this->link.send(ack);
          // std::cout << "Sending ACK: " << ack << std::endl;

          // Deliver only if not previously delivered
          {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (delivered_messages[sender_id].insert(id).second)
            {
              std::vector<DataMessage> data_messages = SendBuffer::deserialize(message.get_payload(), message.get_length());
              for (auto data_message : data_messages) {
                // std::cout << "Delivered message: " << data_message.get_message() << std::endl;
                handler(data_message, message.get_sender());
              }
            }
          }
      });
    });
  }

  void shutdown()
  {
    this->link.stop_receiving();
    this->stop_sending();
  }

  void stop_receiving()
  {
    this->link.stop_receiving();
  }

  void stop_sending()
  {
    this->continue_sending = false;
  }
};
