#pragma once

#include <cstring>
#include <cstdint>
#include <memory>
#include "address.hpp"
#include "host.hpp"

class Message
{
private:
    Host sender;
    Host receiver;
    std::shared_ptr<char[]> payload;
    uint64_t length;

    // Constructor for deserialization
    Message(Host s, Host r) : sender(s), receiver(r), length(0) {}

    // Helper methods for payload handling
    template <typename T>
    static std::shared_ptr<char[]> encode_payload(const T &value, uint64_t &length)
    {
        length = sizeof(T);
        auto buffer = std::shared_ptr<char[]>(new char[length]);
        std::memcpy(buffer.get(), &value, length);
        return buffer;
    }

    template <typename T>
    T decode_payload() const
    {
        T value;
        if (length != sizeof(T))
        {
            throw std::runtime_error("Payload size mismatch during decoding");
        }
        std::memcpy(&value, payload.get(), length);
        return value;
    }

public:
    // Constructor for data messages
    template <typename T>
    Message(Host sender, Host receiver, const T &value)
        : sender(sender), receiver(receiver), length(sizeof(T))
    {
        // Create payload
        payload = encode_payload(value, length);
    }

    std::unique_ptr<char[]> serialize(uint64_t &total_length) const
    {
        // Calculate total serialized length
        total_length = sizeof(sender) + sizeof(receiver) + sizeof(length) + length;

        // Create buffer and offset
        std::unique_ptr<char[]> buffer(new char[total_length]);
        size_t offset = 0;

        // Serialize
        std::memcpy(buffer.get() + offset, &sender, sizeof(sender));
        offset += sizeof(sender);
        std::memcpy(buffer.get() + offset, &receiver, sizeof(receiver));
        offset += sizeof(receiver);
        std::memcpy(buffer.get() + offset, &length, sizeof(length));
        offset += sizeof(length);

        if (length > 0)
        {
            std::memcpy(buffer.get() + offset, payload.get(), length);
        }

        return buffer;
    }

    static Message deserialize(const char *buffer, size_t buffer_size)
    {
        size_t offset = 0;
        size_t minimum_size = sizeof(Host) * 2 + sizeof(uint64_t);

        if (buffer_size < minimum_size)
        {
            throw std::runtime_error("Buffer too small for deserialization");
        }

        // Deserialize header components
        Host sender;
        std::memcpy(&sender, buffer + offset, sizeof(sender));
        offset += sizeof(sender);

        Host receiver;
        std::memcpy(&receiver, buffer + offset, sizeof(receiver));
        offset += sizeof(receiver);

        uint64_t payload_length;
        std::memcpy(&payload_length, buffer + offset, sizeof(payload_length));
        offset += sizeof(payload_length);

        // Validate remaining buffer size
        if (buffer_size < minimum_size + payload_length)
        {
            throw std::runtime_error("Buffer too small for payload");
        }

        // Create a message with the payload
        Message msg(sender, receiver);
        msg.length = payload_length;
        if (payload_length > 0)
        {
            msg.payload = std::shared_ptr<char[]>(new char[payload_length]);
            std::memcpy(msg.payload.get(), buffer + offset, payload_length);
        }

        return msg;
    }

    std::string get_payload_string() const
    {
        if (length == sizeof(size_t))
        {
            return std::to_string(decode_payload<size_t>());
        }
        // Add more type handlers as needed
        return "Unknown payload type";
    }

    // Getters
    Host get_sender() const { return sender; }
    Host get_receiver() const { return receiver; }
    const char *get_payload() const
    {
        return payload.get();
    }
    uint64_t get_length() const { return length; }
};
