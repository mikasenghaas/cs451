#pragma once

// Project files
#include "host.hpp"

/**
 * @brief Message
 *
 * @details Serialized size:
 * Host+Receiver: 2*(ID: 8B + Address: 8B) = 32B
 * Length: 8B
 * Sequence Number: 8B
 * Ack: 1B
 * 
 * Total: 49B
*/
class Message
{
private:
    Host sender;
    Host receiver;
    std::shared_ptr<char[]> payload;
    size_t length{0};
    size_t seq_num{0};
    bool is_ack{false};

    // Encode payload
    template <typename T>
    static std::shared_ptr<char[]> encode_payload(const T &value, size_t &length)
    {
        length = sizeof(T);
        auto buffer = std::shared_ptr<char[]>(new char[length]);
        std::memcpy(buffer.get(), &value, length);
        return buffer;
    }

    // Decode payload
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

    // Get minimum serialized size
    static size_t get_min_serialized_size()
    {
        return sizeof(Host) * 2 + sizeof(length) + sizeof(seq_num) + sizeof(is_ack);
    }


public:
    // Default constructor
    Message() = default;

    // Main constructor
    Message(Host sender, Host receiver): sender(sender), receiver(receiver) {}

    // Serialize message
    std::unique_ptr<char[]> serialize(uint64_t &total_length) const
    {
        // Calculate total serialized length
        total_length = get_min_serialized_size() + length;

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
        size_t minimum_size = get_min_serialized_size();

        if (buffer_size < minimum_size)
        {
            throw std::runtime_error("Buffer too small for deserialization");
        }

        // Deserialize sender
        Host sender;
        std::memcpy(&sender, buffer + offset, sizeof(sender));
        offset += sizeof(sender);

        // Deserialize receiver
        Host receiver;
        std::memcpy(&receiver, buffer + offset, sizeof(receiver));
        offset += sizeof(receiver);

        // Deserialize payload length
        size_t payload_length;
        std::memcpy(&payload_length, buffer + offset, sizeof(payload_length));
        offset += sizeof(payload_length);

        // Deserialize seq_num
        size_t seq_num;
        std::memcpy(&seq_num, buffer + offset, sizeof(seq_num));
        offset += sizeof(seq_num);

        // Deserialize is_ack
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

    // Getters
    Host get_sender() const { return sender; }
    Host get_receiver() const { return receiver; }
    const char *get_payload() const { return payload.get(); }
    size_t get_length() const { return length; }
    size_t get_seq_num() const { return seq_num; }
    bool get_is_ack() const { return is_ack; }

    // Setters
    void set_seq_num(size_t seq_num) { this->seq_num = seq_num; }
    template <typename T>
    void set_payload(const T &value)
    {
        payload = encode_payload(value, length);
    }

    // Static methods
    static Message create_ack(const Message msg)
    {
        Message ack;
        ack.sender = msg.receiver;
        ack.receiver = msg.sender;
        ack.is_ack = true;
        return ack;
    }

    // Get payload string
    std::string get_payload_string() const
    {
        // Note: Use for  payloads (Milestone 1)
        if (length == sizeof(int))
        {
            return std::to_string(decode_payload<int>());
        }
        std::cout << "Unknown payload type\n";
        return "Unknown payload type";
    }

    // String representation
    std::string to_string() const {
        std::string result = "Message(";
        result += "sender=" + std::to_string(sender.get_id());
        result += ", receiver=" + std::to_string(receiver.get_id()); 
        result += ", seq_num=" + std::to_string(seq_num);
        result += ", is_ack=" + std::string(is_ack ? "true" : "false");
        result += ")";
        return result;
    }
    friend std::ostream& operator<<(std::ostream& os, const Message& msg) { return os << msg.to_string(); }

};
