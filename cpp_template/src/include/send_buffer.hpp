#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "host_lookup.hpp"
#include "message.hpp"

#define MAX_MESSAGE_COUNT 8

/*
 * @brief Send buffer class
 *
 * Buffer for sending messages to hosts
 *
 * TODO: Make thread safe
 */
class SendBuffer
{
private:
    uint64_t capacity;                                           // Capacity of the buffer
    std::unordered_map<size_t, uint64_t> sizes;                  // Sizes of the messages in the buffer
    std::unordered_map<size_t, std::unique_ptr<char[]>> buffers; // Buffers for the messages
    std::unordered_map<size_t, uint8_t> message_counts;          // Number of messages in the buffer
    HostLookup host_lookup;                                      // Host lookup object

public:
    SendBuffer(HostLookup host_lookup, uint64_t initial_capacity)
        : capacity(initial_capacity), host_lookup(host_lookup)
    {
        // Initialise buffer sizes, buffers and message counts for each host
        for (auto host : host_lookup.get_hosts())
        {
            this->sizes.emplace(host, 0);
            this->buffers.emplace(
                host, std::unique_ptr<char[]>(new char[initial_capacity]));
            this->message_counts.emplace(host, 0);
        }
    }

    std::unique_ptr<char[]> add_message(Message message, uint64_t &payload_length)
    {
        // Serialize message
        uint64_t serialized_length = 0;
        std::unique_ptr<char[]> payload = message.serialize(serialized_length);

        // Get host ID
        uint8_t host_id = host_lookup.get_host(message.get_address());

        // Check if max messages reached or buffer is full
        bool max_messages = this->message_counts[host_id] == MAX_MESSAGE_COUNT;
        bool buffer_full = serialized_length + this->sizes[host_id] + sizeof(serialized_length) > this->capacity;

        if (max_messages || buffer_full)
        {
            // Copy buffer to payload
            std::unique_ptr<char[]> buffer_payload(
                new char[this->sizes[host_id]]);
            std::memcpy(buffer_payload.get(), this->buffers[host_id].get(),
                        this->sizes[host_id]);
            payload_length = this->sizes[host_id];

            // Copy serialized message to buffer
            std::memcpy(this->buffers[host_id].get(), &serialized_length,
                        sizeof(serialized_length));
            std::memcpy(this->buffers[host_id].get() +
                            sizeof(serialized_length),
                        payload.get(), serialized_length);
            this->sizes[host_id] =
                sizeof(serialized_length) + serialized_length;
            this->message_counts[host_id] = 1;

            // Return payload
            return buffer_payload;
        }

        // Copy serialized message to buffer
        std::memcpy(this->buffers[host_id].get() + this->sizes[host_id],
                    &serialized_length, sizeof(serialized_length));
        std::memcpy(this->buffers[host_id].get() + this->sizes[host_id] +
                        sizeof(serialized_length),
                    payload.get(), serialized_length);
        this->sizes[host_id] += sizeof(serialized_length) + serialized_length;
        this->message_counts[host_id]++;

        // Return empty payload
        payload_length = 0;
        return std::unique_ptr<char[]>(new char[0]);
    }

    std::vector<Message> deserialize(Address address, char *buffer,
                                     ssize_t received_length)
    {
        // Create vector of messages
        std::vector<Message> messages;
        ssize_t start = 0;
        uint64_t message_length;
        while (start < received_length)
        {
            // Deserialize message
            std::memcpy(&message_length, buffer + start,
                        sizeof(message_length));
            Message msg(address, buffer + start + sizeof(message_length),
                        message_length);
            messages.push_back(msg);
            start += sizeof(message_length) + message_length;
        }

        return messages;
    }
};