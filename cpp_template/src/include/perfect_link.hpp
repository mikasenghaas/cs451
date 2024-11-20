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
  std::map<size_t, std::set<size_t>> acked_messages; // Acked set of messages set<message_id> to receiver host_id
  std::map<size_t, std::set<size_t>> delivered_messages; // Delivered set of messages set<message_id> from sender host_id
  std::queue<TransportMessage> send_queue; // Queue of messages to send
  std::mutex queue_mutex;
  std::mutex ack_mutex;
  std::thread sending_thread;
  std::thread receiving_thread;
  bool continue_sending = true;

  std::thread start_sending()
  {
    // std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    return std::thread([this]() {
      while (this->continue_sending)
      {
        TransportMessage tm;
        {
          std::lock_guard<std::mutex> queue_lock(queue_mutex);
          if (!send_queue.empty()) {
            tm = send_queue.front();
            send_queue.pop();
            std::cout << "plDequeue: " << tm << " (size: " << send_queue.size() << ")" << std::endl;

            size_t receiver_id = tm.get_receiver().get_id();
            size_t seq_number = tm.get_seq_number();

            bool already_acked;
            {
              std::lock_guard<std::mutex> ack_lock(ack_mutex);
              already_acked = acked_messages[receiver_id].count(seq_number) > 0;
            }

            if (!already_acked) {
              std::cout << "plSend: " << tm << std::endl;
              this->link.send(tm);
              send_queue.push(tm);
            }
          }
        }
      }
    });
  }


  std::thread start_receiving(std::function<void(TransportMessage)> handler)
  {
    // std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    return std::thread([this, handler]() {
      this->link.start_receiving(
        [this, handler](TransportMessage tm) {
          std::cout << "plRecv: " << tm << std::endl;
          // Get sender and message id
          size_t sender_id = tm.get_sender().get_id();
          size_t seq_number = tm.get_seq_number();
          if (tm.get_is_ack())
          {
            std::cout << "Got ACK: " << tm << std::endl;
            {
              std::lock_guard<std::mutex> ack_lock(ack_mutex);
              acked_messages[sender_id].insert(seq_number);
            }
            return;
          }

          // Send ACK for received message
          TransportMessage ack = TransportMessage::create_ack(tm);
          std::cout << "Send ACK: " << ack << std::endl;
          this->link.send(ack);

          // Deliver only if not previously delivered
          if (delivered_messages[sender_id].insert(seq_number).second)
          {
            std::cout << "plDeliver: " << tm << std::endl;
            handler(tm);
          }
      });
    });
  }


public:
  PerfectLink(Host host, Hosts hosts, std::function<void(TransportMessage)> deliver) : host(host), hosts(hosts), link(host, hosts), send_buffer(hosts, MAX_SEND_BUFFER_SIZE) {
    std::cout << "Setting up perfect link at " << host.get_address().to_string() << std::endl;
    for (const auto &host : hosts.get_hosts()) {
      if (host.get_id() != this->host.get_id()) {
        acked_messages[host.get_id()];
        delivered_messages[host.get_id()];
      }
    }

    this->receiving_thread = start_receiving(deliver);
    this->sending_thread = start_sending();
  }

  void send(Message &m, Host receiver) {
    // Serialize data message
    uint64_t length = 0;
    auto payload = m.serialize(length);

    // Create transport message
    TransportMessage tm(host, receiver, std::move(payload), length);

    std::cout << "plEnqueue: " << tm << std::endl;
    {
      std::lock_guard<std::mutex> queue_lock(queue_mutex);
      send_queue.push(tm);
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
