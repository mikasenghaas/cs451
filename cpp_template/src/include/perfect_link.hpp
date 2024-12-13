#pragma once

#include "hosts.hpp"
#include "message.hpp"
#include "message_set.hpp"
#include "concurrent_queue.hpp"
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
  MessageSet acked_messages; // Acked set of messages set<message_id> to receiver host_id
  MessageSet delivered_messages; // Delivered set of messages set<message_id> from sender host_id
  ConcurrentQueue<TransportMessage> queue; // Queue of messages to send
  std::thread sending_thread;
  std::thread receiving_thread;
  bool continue_sending = true;

  std::thread start_sending()
  {
    // std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    return std::thread([this]() {
      while (this->continue_sending) {
        TransportMessage tm;
        if (!this->queue.empty()) {
          tm = this->queue.pop();

          size_t receiver_id = tm.get_receiver().get_id();
          size_t seq_number = tm.get_seq_number();

          if (!this->acked_messages.contains(receiver_id, seq_number)) {
            // std::cout << "plSend: " << tm << std::endl;
            this->link.send(tm);
            this->queue.push(tm);
          }
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
  }

  std::thread start_receiving(std::function<void(TransportMessage)> plDeliver)
  {
    // std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    return std::thread([this, plDeliver]() {
      this->link.start_receiving(
        [this, plDeliver](TransportMessage tm) {
          // Get sender and message id
          size_t sender_id = tm.get_sender().get_id();
          size_t seq_number = tm.get_seq_number();
          if (tm.is_ack())
          {
            if (!this->acked_messages.contains(sender_id, seq_number)) {
              this->acked_messages.insert(sender_id, seq_number);
            }
            return;
          }

          // Send ACK for received message
          TransportMessage ack = TransportMessage::create_ack(tm);
          this->link.send(ack);

          // Deliver only if not previously delivered
          if (!this->delivered_messages.contains(sender_id, seq_number))
          {
            this->delivered_messages.insert(sender_id, seq_number);
            // std::cout << "plDeliver: " << tm << std::endl;
            plDeliver(tm);
          }
      });
    });
  }


public:
  PerfectLink(Host host, Hosts hosts, std::function<void(TransportMessage)> plDeliver) : 
    host(host), hosts(hosts), link(host, hosts), acked_messages(hosts), delivered_messages(hosts) {
    this->receiving_thread = start_receiving(plDeliver);
    this->sending_thread = start_sending();
  }

  void send(Message &m, Host receiver) {
    // Serialize message
    size_t length = 0;
    auto payload = m.serialize(length);

    // Create transport message
    TransportMessage tm(host, receiver, std::move(payload), length);

    // std::cout << "plEnqueue: " << tm << std::endl;
    queue.push(tm);
  }

  void shutdown()
  {
    this->link.shutdown();
    this->continue_sending = false;
  }
};
