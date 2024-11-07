#pragma once

#include "hosts.hpp"
#include "message.hpp"
#include "fair_loss_link.hpp"

#define TIMEOUT_MS 100 /// Milliseconds

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
  std::map<size_t, std::set<size_t>> delivered_messages; // Delivered seq_num from sender host_id
  bool has_pending_message = false;
  std::queue<TransportMessage> send_queue; // Queue of messages to send
  std::mutex queue_mutex;

public:
  PerfectLink(Host host, Hosts hosts) : host(host), hosts(hosts), link(host), send_buffer(hosts, MAX_SEND_BUFFER_SIZE) {
    for (const auto &host : hosts.get_hosts()) {
      if (host.get_id() != this->host.get_id()) {
        seq_nums[host.get_id()] = 1;
        delivered_messages[host.get_id()];
      }
    }
  }

  void send(DataMessage message, Host receiver)
  {
    std::lock_guard<std::mutex> lock(queue_mutex);

    // Add message to send buffer
    uint64_t payload_length;
    auto payload = send_buffer.add_message(receiver, message, payload_length);

    if (payload_length == 0) {
      return;
    }

    // Buffer is ready to send
    size_t id = seq_nums[receiver.get_id()]++;
    auto shared_payload = std::shared_ptr<char[]>(new char[payload_length]);
    std::memcpy(shared_payload.get(), payload.get(), payload_length);
    TransportMessage transport_message(id, host, receiver, shared_payload, payload_length, false);
    send_queue.push(transport_message);
    std::cout << "Added message " << id << " to queue (size=" << send_queue.size() << ")" << std::endl;
  }

  std::thread start_sending()
  {
    std::cout << "Starting sending on " << host.get_address().to_string() << "\n";

    return std::thread([this]() {
      while (this->continue_sending)
      {
        TransportMessage current_message;
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
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
          continue;
        }
        
        // Send and wait for ACK
        while (this->continue_sending && has_pending_message)
        {
          {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (has_pending_message) {  // Check again under lock
              // std::cout << "Sending message: " << current_message << std::endl;
              this->link.send(current_message);
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
        }
      }
    });
  }


  std::thread start_receiving(std::function<void(DataMessage, Host)> handler)
  {
    std::cout << "Starting receiving on " << host.get_address().to_string() << "\n";

    auto receiver_function = [this, handler]() {
      this->link.start_receiving(
        [this, handler](TransportMessage message) {
          if (message.get_is_ack())
          {
            // std::cout << "Received ACK: " << message << std::endl;
            std::lock_guard<std::mutex> lock(queue_mutex);
            has_pending_message = false;
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
            if (delivered_messages[message.get_sender().get_id()].insert(message.get_id()).second)
            {
              // std::cout << "Delivered message: " << message << std::endl;
              std::vector<DataMessage> data_messages = SendBuffer::deserialize(message.get_payload(), message.get_length());
              for (auto data_message : data_messages) {
                handler(data_message, message.get_sender());
              }
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
