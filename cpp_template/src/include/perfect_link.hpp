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
  std::queue<TransportMessage> send_queue; // Queue of messages to send
  std::mutex queue_mutex;
  std::thread sending_thread;
  std::thread receiving_thread;

  std::thread start_sending()
  {
    // std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    return std::thread([this]() {
      while (this->continue_sending)
      {
        TransportMessage message;
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          if (!send_queue.empty()) {
            message = send_queue.front();
            send_queue.pop();

            size_t receiver_id = message.get_receiver().get_id();
            size_t seq_number = message.get_seq_number();

            if (acked_messages[receiver_id].count(seq_number) == 0) {
              // Send message
              // std::cout << "plSend: " << message << std::endl;
              this->link.send(message);
              send_queue.push(message);
            }
          }
        }
      }
    });
  }


  std::thread start_receiving(std::function<void(TransportMessage)> deliver)
  {
    // std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    return std::thread([this, deliver]() {
      this->link.start_receiving(
        [this, deliver](TransportMessage message) {
          // Get sender and message id
          size_t sender_id = message.get_sender().get_id();
          size_t seq_number = message.get_seq_number();
          if (message.get_is_ack())
          {
            // std::cout << "plRecvAck: " << message << std::endl;
            acked_messages[sender_id].insert(seq_number);
            return;
          }

          // Send ACK for received message
          // std::cout << "Received message: " << message << std::endl;
          TransportMessage ack = TransportMessage::create_ack(message);
          this->link.send(ack);
          // std::cout << "plSendACK: " << ack << std::endl;

          // Deliver only if not previously delivered
          {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (delivered_messages[sender_id].insert(seq_number).second)
            {
              // std::cout << "plDeliver: " << message << std::endl;
              deliver(message);
            }
          }
      });
    });
  }


public:
  PerfectLink(Host host, Hosts hosts, std::function<void(TransportMessage)> deliver) : host(host), hosts(hosts), link(host, hosts), send_buffer(hosts, MAX_SEND_BUFFER_SIZE) {
    std::cout << "Setting up perfect link at " << host.get_address().to_string() << std::endl;
    for (const auto &host : hosts.get_hosts()) {
      if (host.get_id() != this->host.get_id()) {
        seq_nums[host.get_id()] = 1;
        acked_messages[host.get_id()];
        delivered_messages[host.get_id()];
      }
    }

    this->receiving_thread = start_receiving(deliver);
    this->sending_thread = start_sending();
  }

  void send(Host receiver, DataMessage message) {
    uint64_t length = 0;
    auto payload = message.serialize(length);

    size_t seq_number = seq_nums[receiver.get_id()]++;
    TransportMessage transport_message(seq_number, host, receiver, std::move(payload), length, false);

    send_queue.push(transport_message);
  }
  
  void broadcast(DataMessage message) {
    for (const auto &host : hosts.get_hosts()) {
      send(host, message);
    }
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
