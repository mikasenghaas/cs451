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
    size_t seq_num;
    bool is_ack;

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
    // Empty constructor
    Message() : sender(), receiver(), length(0), seq_num(0), is_ack(false) {}
    
    // Host and sender constructor
    Message(Host s, Host r) : sender(s), receiver(r), length(0) {}

    // Transport message constructor
    template <typename T>
    Message(Host sender, Host receiver, const T &value)
        : sender(sender), receiver(receiver), length(sizeof(T)), is_ack(false)
    {
        // Create payload
        payload = encode_payload(value, length);
    }

    std::unique_ptr<char[]> serialize(uint64_t &total_length) const
    {
        // Calculate total serialized length
        total_length = sizeof(Host) * 2 + sizeof(length) + sizeof(seq_num) + sizeof(is_ack) + length;

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
        std::memcpy(buffer.get() + offset, &seq_num, sizeof(seq_num));
        offset += sizeof(seq_num);
        std::memcpy(buffer.get() + offset, &is_ack, sizeof(is_ack));
        offset += sizeof(is_ack);

        if (length > 0)
        {
            std::memcpy(buffer.get() + offset, payload.get(), length);
        }

        return buffer;
    }

    static Message deserialize(const char *buffer, size_t buffer_size)
    {
        size_t offset = 0;
        size_t minimum_size = sizeof(Host) * 2 + sizeof(length) + sizeof(seq_num) + sizeof(is_ack);

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

        size_t seq_num;
        std::memcpy(&seq_num, buffer + offset, sizeof(seq_num));
        offset += sizeof(seq_num);

        bool is_ack;
        std::memcpy(&is_ack, buffer + offset, sizeof(is_ack));
        offset += sizeof(is_ack);

        // Validate remaining buffer size
        if (buffer_size < minimum_size + payload_length)
        {
            throw std::runtime_error("Buffer too small for payload");
        }

        // Create a message with the payload
        Message msg(sender, receiver);
        msg.length = payload_length;
        msg.seq_num = seq_num;
        msg.is_ack = is_ack;

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
    size_t get_seq_num() const { return seq_num; }
    bool get_is_ack() const { return is_ack; }

    // Setters
    void set_seq_num(size_t seq_num) { this->seq_num = seq_num; }

    // Static methods
    static Message create_ack(const Message msg)
    {
        Message ack;
        ack.sender = msg.receiver;
        ack.receiver = msg.sender;
        ack.is_ack = true;
        return ack;
    }

    std::string to_string() const {
        std::string result = "Message(";
        result += "sender=" + std::to_string(sender.get_id());
        result += ", receiver=" + std::to_string(receiver.get_id()); 
        result += ", seq_num=" + std::to_string(seq_num);
        result += ", is_ack=" + std::string(is_ack ? "true" : "false");
        result += ")";
        return result;
    }

    // Add this operator overload as a friend function
    friend std::ostream& operator<<(std::ostream& os, const Message& msg) {
        return os << msg.to_string();
    }
};
