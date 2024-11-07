#pragma once

#include "message.hpp"
#include "hosts.hpp"

#define MAX_MESSAGE_COUNT 8

/**
 * @brief Buffer for sending messages to hosts
 * 
 * @details The send buffer is a buffer for sending messages to hosts. It is used to
 *          batch messages to the same host into a single buffer. This should give
 *          a constant performance improvement by MAX_MESSAGE_COUNT.
 */
class SendBuffer {
private:
    Hosts hosts;
    uint64_t capacity;
    std::unordered_map<size_t, uint64_t> sizes;
    std::unordered_map<size_t, std::unique_ptr<char[]>> buffers;
    std::unordered_map<size_t, size_t> message_counts;
    std::mutex lock;
public:
    SendBuffer(Hosts hosts, uint64_t initial_capacity) : hosts(hosts), capacity(initial_capacity) {
        // Initialize buffer sizes, buffers, and message counts for all hosts
        for (auto host : hosts.get_hosts()) {
            size_t host_id = host.get_id();
            this->sizes.emplace(host_id, 0);
            this->buffers.emplace(host_id, std::unique_ptr<char[]>(new char[initial_capacity]));
            this->message_counts.emplace(host_id, 0);
        }
    }            

    std::unique_ptr<char[]> add_message(Host receiver, DataMessage message, uint64_t &payload_length, const bool &reset = false)
    {
        // Serialize the message
        uint64_t serialized_length;
        std::unique_ptr<char[]> serialized_message = message.serialize(serialized_length);

        // Get host id of receiver
        size_t receiver_id = receiver.get_id();

        // Acquire lock to manipulate buffer
        this->lock.lock();

        // Check if buffer has space for the message
        bool has_space = sizes[receiver_id] + serialized_length <= capacity;
        bool has_room = this->message_counts[receiver_id] < MAX_MESSAGE_COUNT;

        // If there is space, add the message
        if (has_space && has_room)
        {
            // Add the message to the buffer, if there is space
            std::memcpy(this->buffers[receiver_id].get() + this->sizes[receiver_id], &serialized_length, sizeof(serialized_length));
            std::memcpy(this->buffers[receiver_id].get() + this->sizes[receiver_id] + sizeof(serialized_length), serialized_message.get(), serialized_length);
            this->sizes[receiver_id] += sizeof(serialized_length) + serialized_length;
            this->message_counts[receiver_id]++;
            this->lock.unlock();
            // std::cout << "Adding message to buffer (" << this->message_counts[receiver_id] << " messages)" << std::endl;
            
            // Return empty buffer (send buffer is not ready yet)
            if (!reset)
            {
                std::unique_ptr<char[]> buffer_payload(new char[0]);
                payload_length = 0;
                return buffer_payload;
            } else {
                // Copy the buffer to return
                std::unique_ptr<char[]> buffer_payload(new char[this->sizes[receiver_id]]);
                std::memcpy(buffer_payload.get(), this->buffers[receiver_id].get(), this->sizes[receiver_id]);
                payload_length = this->sizes[receiver_id];
                this->sizes[receiver_id] = 0;
                this->message_counts[receiver_id] = 0;

                return buffer_payload;
            }
        } else { // Otherwise, return the full buffer and reset with new message
            // Copy the buffer to return
            // std::cout << "Copying buffer to return" << std::endl;
            std::unique_ptr<char[]> buffer_payload(new char[this->sizes[receiver_id]]);
            std::memcpy(buffer_payload.get(), this->buffers[receiver_id].get(), this->sizes[receiver_id]);
            payload_length = this->sizes[receiver_id];

            // std::cout << "Resetting buffer with new message" << std::endl;
            // Reset the buffer with the new message
            std::memcpy(this->buffers[receiver_id].get(), &serialized_length, sizeof(serialized_length));
            std::memcpy(this->buffers[receiver_id].get() + sizeof(serialized_length), serialized_message.get(), serialized_length);
            this->sizes[receiver_id] = sizeof(serialized_length) + serialized_length;
            this->message_counts[receiver_id] = 1;

            this->lock.unlock();
            return buffer_payload;
        }
    }

    static std::vector<DataMessage> deserialize(const char *buffer, size_t received_length) {
        std::vector<DataMessage> messages;
        size_t offset = 0;
        uint64_t message_length;
        while (offset < received_length) {
            std::memcpy(&message_length, buffer + offset, sizeof(message_length));
            DataMessage message = DataMessage::deserialize(buffer + offset + sizeof(message_length), message_length);
            messages.push_back(message);
            offset += sizeof(message_length) + message_length;
        }

        return messages;
    }
};
