#pragma once

// C++ standard library headers
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <queue>
#include <mutex>
#include <set>
#include <chrono>

// C system headers
#include <netinet/in.h>
#include <sys/socket.h>

// Project headers
#include "hosts.hpp"
#include "message.hpp"
#include "fair_loss_link.hpp"

using ID = size_t;

/**
 * @brief Perfect link class (using stop-and-wait protocol)
 *
 * Send and receive messages over a network reliably.
 */
class PerfectLink
{
private:
  Host host;
  Hosts hosts;
  FairLossLink link;
  bool continue_sending = true;
  std::map<size_t, size_t> seq_nums; // seq_num from sender host_id
  std::map<size_t, std::set<size_t>> delivered_messages; // Delivered seq_num from sender host_id
  bool has_pending_message = false;
  std::queue<Message> send_queue; // Queue of messages to send
  std::mutex queue_mutex;

public:
  PerfectLink(Host host, Hosts hosts) : host(host), hosts(hosts), link(host) {
    for (const auto &host : hosts.get_hosts()) {
      if (host.get_id() != this->host.get_id()) {
        seq_nums[host.get_id()] = 1;
        delivered_messages[host.get_id()];
      }
    }
  }

  void send(Message message)
  {
    std::lock_guard<std::mutex> lock(queue_mutex);

    Message message_to_send = message;
    message_to_send.set_seq_num(seq_nums[message.get_receiver().get_id()]++);

    send_queue.push(message_to_send);
    std::cout << "Adding message " << message_to_send.get_seq_num() << " to queue (size=" << send_queue.size() << ")" << std::endl;
  }

  std::thread start_sending()
  {
    std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    return std::thread([this]() {
      while (this->continue_sending)
      {
        Message current_message;
        bool should_send = false;
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          if (!send_queue.empty()) {
            current_message = send_queue.front();
            send_queue.pop();
            should_send = true;
            has_pending_message = true;
          }
        }

        if (!should_send) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
        
        // Send and wait for ACK
        while (this->continue_sending && has_pending_message)
        {
          {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (has_pending_message) {  // Check again under lock
              std::cout << "Sending message: " << current_message << std::endl;
              this->link.send(current_message);
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
    });
  }


  std::thread start_receiving(std::function<void(Message)> handler)
  {
    std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    auto receiver_function = [this, handler]() {
      this->link.start_receiving(
        [this, handler](Message message) {
          if (message.get_is_ack())
          {
            std::cout << "Received ACK: " << message << std::endl;
            std::lock_guard<std::mutex> lock(queue_mutex);
            has_pending_message = false;
            return;
          }

          // Send ACK for received message
          std::cout << "Received message: " << message << std::endl;
          Message ack = Message::create_ack(message);
          this->link.send(ack);
          std::cout << "Sending ACK: " << ack << std::endl;

          // Deliver only if not previously delivered
          {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (delivered_messages[message.get_sender().get_id()].insert(message.get_seq_num()).second)
            {
              std::cout << "Delivered message: " << message << std::endl;
              handler(message);
            }
          }
      });
    };
    return std::thread(receiver_function);
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
