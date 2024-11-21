#pragma once

#include <atomic>
#include <thread>

// Project files
#include "host.hpp"

enum class MessageType {
    String,
    Broadcast
};

class Message {
protected:
    MessageType type;
    Message(const MessageType type) : type(type) {}

public:
    virtual std::unique_ptr<char[]> serialize(uint64_t &length) = 0;
    virtual std::string to_string() const = 0;
    friend std::ostream& operator<<(std::ostream& os, const Message& msg) { return os << msg.to_string(); }
};

class StringMessage : public Message {
private:
    std::string message;

public:
    StringMessage(std::string message) : Message(MessageType::String), message(message) {}
    StringMessage(std::unique_ptr<char[]> payload, size_t length) : StringMessage(std::string(payload.get(), length)) {}

    std::string get_message() const { return message; }

    std::unique_ptr<char[]> serialize(uint64_t &total_length) {
        total_length = sizeof(MessageType) + message.length();
        auto buffer = std::unique_ptr<char[]>(new char[total_length]);
        
        // Write the message type first
        std::memcpy(buffer.get(), &type, sizeof(MessageType));
        std::memcpy(buffer.get() + sizeof(MessageType), message.c_str(), message.length());
        
        return buffer;
    }

    std::string to_string() const { return "StringMessage(" + message + ")"; }
}; 

class BroadcastMessage : public Message {
private:
    static std::atomic_uint32_t next_id;
    size_t seq_number;
    size_t source_id;
    size_t length;
    std::unique_ptr<char[]> payload;

public:
    BroadcastMessage(Message &m, size_t source_id) : Message(MessageType::Broadcast), seq_number(next_id++), source_id(source_id) {
        this->payload = m.serialize(this->length);
    }

    BroadcastMessage(std::shared_ptr<char[]> payload, const uint64_t length)
        : Message(MessageType::Broadcast), seq_number(next_id++) {
        // We can skip the type
        ssize_t offset = sizeof(this->type);

        std::memcpy(&this->seq_number, payload.get() + offset, sizeof(this->seq_number));
        offset += sizeof(this->seq_number);

        std::memcpy(&this->source_id, payload.get() + offset, sizeof(this->source_id));
        offset += sizeof(this->source_id);

        std::memcpy(&this->length, payload.get() + offset, sizeof(this->length));
        offset += sizeof(this->length);

        this->payload = std::unique_ptr<char[]>(new char[this->length]);
        std::memcpy(this->payload.get(), payload.get() + offset, this->length);
    }

    std::unique_ptr<char[]> serialize(uint64_t &total_length) {
        total_length = sizeof(MessageType) + sizeof(seq_number) + sizeof(source_id) + sizeof(length) + this->length;

        std::unique_ptr<char[]> payload(new char[total_length]);
        size_t offset = 0;

        std::memcpy(payload.get() + offset, &type, sizeof(MessageType));
        offset += sizeof(MessageType);

        std::memcpy(payload.get() + offset, &this->seq_number, sizeof(this->seq_number));
        offset += sizeof(this->seq_number);

        std::memcpy(payload.get() + offset, &this->source_id, sizeof(this->source_id));
        offset += sizeof(this->source_id);

        std::memcpy(payload.get() + offset, &this->length, sizeof(this->length));
        offset += sizeof(this->length);

        std::memcpy(payload.get() + offset, this->payload.get(), this->length);

        return payload;
    }

    size_t get_seq_number() const { return this->seq_number; }
    size_t get_source_id() const { return this->source_id; }
    size_t get_length() const { return this->length; }
    std::unique_ptr<Message> get_message() {
        // Get message type
        MessageType message_type;
        std::memcpy(&message_type, this->payload.get(), sizeof(message_type));

        // Skip the message type
        size_t payload_offset = sizeof(MessageType);
        size_t length = this->length - payload_offset;

        // Get payload
        auto payload = std::make_unique<char[]>(length);
        std::memcpy(payload.get(), this->payload.get() + payload_offset, length);

        switch (message_type) {
        case MessageType::String: {
            return std::make_unique<StringMessage>(std::move(payload), length);
        }
        case MessageType::Broadcast: {
            return std::make_unique<BroadcastMessage>(std::move(payload), length);
        }
        default:
            throw std::runtime_error("Unknown message type: " + std::to_string(static_cast<int>(message_type)));
        }
    }

    std::string to_string() const { return "BroadcastMessage(seq_number=" + std::to_string(this->seq_number) + ", source_id=" + std::to_string(this->source_id) + ")"; }
};

// Next sequence number
std::atomic_uint32_t BroadcastMessage::next_id{1};

/**
 * @brief TransportMessage
 *
 * @details Serialized size:
 * Host+Receiver: 2*(ID: 8B + Address: 8B) = 32B
 * Length: 8B
 * Sequence Number: 8B
 * Ack: 1B
 * 
 * Total: 49B
*/
class TransportMessage {
private:
    size_t seq_number;
    Host sender;
    Host receiver;
    std::shared_ptr<char[]> payload;
    size_t length;
    bool is_ack;
    
    // Next sequence number
    static std::atomic_uint32_t next_id; 

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
        return sizeof(seq_number) + sizeof(Host) * 2 + sizeof(length) + sizeof(is_ack);
    }


public:
    // Default constructor
    TransportMessage() = default;
    
    // Main constructor (from serialized DataMessage)
    TransportMessage(Host sender, Host receiver, std::shared_ptr<char[]> payload, size_t length) : seq_number(next_id++), sender(sender), receiver(receiver), payload(payload), length(length), is_ack(false) {}

    // Serialize message
    std::unique_ptr<char[]> serialize(uint64_t &total_length) const
    {
        // Calculate total serialized length
        total_length = get_min_serialized_size() + length;

        // Create buffer and offset
        std::unique_ptr<char[]> buffer(new char[total_length]);
        size_t offset = 0;

        // Serialize
        std::memcpy(buffer.get() + offset, &seq_number, sizeof(seq_number));
        offset += sizeof(seq_number);
        std::memcpy(buffer.get() + offset, &sender, sizeof(sender));
        offset += sizeof(sender);
        std::memcpy(buffer.get() + offset, &receiver, sizeof(receiver));
        offset += sizeof(receiver);
        std::memcpy(buffer.get() + offset, &length, sizeof(length));
        offset += sizeof(length);
        std::memcpy(buffer.get() + offset, &is_ack, sizeof(is_ack));
        offset += sizeof(is_ack);

        if (length > 0)
        {
            std::memcpy(buffer.get() + offset, payload.get(), length);
        }

        return buffer;
    }

    static TransportMessage deserialize(const char *buffer, size_t buffer_size)
    {
        size_t offset = 0;
        size_t minimum_size = get_min_serialized_size();

        if (buffer_size < minimum_size)
        {
            throw std::runtime_error("Buffer too small for deserialization");
        }
        // Deserialize seq_number
        size_t seq_number;
        std::memcpy(&seq_number, buffer + offset, sizeof(seq_number));
        offset += sizeof(seq_number);

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
        TransportMessage tm;
        tm.seq_number = seq_number;
        tm.sender = sender;
        tm.receiver = receiver;
        tm.length = payload_length;
        tm.is_ack = is_ack;

        if (payload_length > 0)
        {
            tm.payload = std::shared_ptr<char[]>(new char[payload_length]);
            std::memcpy(tm.payload.get(), buffer + offset, payload_length);
        }

        return tm;
    }

    // Getters
    size_t get_seq_number() const { return seq_number; }
    Host get_sender() const { return sender; }
    Host get_receiver() const { return receiver; }
    std::shared_ptr<char[]> get_payload() const { return payload; }
    size_t get_length() const { return length; }
    bool get_is_ack() const { return is_ack; }

    template <typename T>
    void set_payload(const T &value) { payload = encode_payload(value, length); }

    // Static methods
    static TransportMessage create_ack(const TransportMessage tm)
    {
        TransportMessage ack;
        ack.seq_number = tm.seq_number;
        ack.sender = tm.receiver;
        ack.receiver = tm.sender;
        ack.is_ack = true;
        ack.length = 0;
        ack.payload = nullptr;
        
        return ack;
    }

    // String representation
    std::string to_string() const {
        std::string result = "TransportMessage(";
        result += "seq_number=" + std::to_string(seq_number);
        result += ", sender=" + std::to_string(sender.get_id());
        result += ", receiver=" + std::to_string(receiver.get_id()); 
        result += ", is_ack=" + std::string(is_ack ? "true" : "false");
        result += ")";
        return result;
    }
    friend std::ostream& operator<<(std::ostream& os, const TransportMessage& msg) { return os << msg.to_string(); }

};

// Next sequence number
std::atomic_uint32_t TransportMessage::next_id{0};
