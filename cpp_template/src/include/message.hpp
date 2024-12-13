#pragma once

#include <atomic>
#include <thread>
#include <variant>

// Project files
#include "host.hpp"
#include "serialize.hpp"


/** @brief Base Message Class
 * 
 * @details Includes functionality:
 * - Protected helpers for serializing class fields
 * - Abstract serialize() and to_string() class
 * - Helper to use std::cout streaming functionality
 */
class Message {
public:
    enum class Type { Transport, String, Broadcast, Proposal };
protected:
    Type message_type;
    Message(Type message_type) : message_type(message_type) {}

    template<typename T>
    static void serialize_field(char* buffer, size_t& offset, const T& value) {
        Serializable<T>::serialize(buffer, offset, value);
    }

    template<typename T>
    static T deserialize_field(const char* buffer, size_t& offset) {
        return Serializable<T>::deserialize(buffer, offset);
    }

public:
    virtual std::shared_ptr<char[]> serialize(size_t &length) = 0;
    virtual std::string to_string() const = 0;
    friend std::ostream& operator<<(std::ostream& os, const Message& msg) { return os << msg.to_string(); }
};

class StringMessage : public Message {
private:
    std::string message;

public:
    StringMessage(std::string message) : Message(Message::Type::String), message(message) {}
    StringMessage(std::shared_ptr<char[]> payload) : Message(Message::Type::String) { 
        auto offset = sizeof(Message::Type);
        auto msg_length = deserialize_field<size_t>(payload.get(), offset);
        message = std::string(payload.get() + offset, msg_length);
    }

    std::shared_ptr<char[]> serialize(size_t &length) {
        length = sizeof(this->message_type) + sizeof(size_t) + message.length();
        size_t offset = 0; auto payload = std::shared_ptr<char[]>(new char[length]);
        serialize_field(payload.get(), offset, message_type);
        size_t msg_length = message.length();
        serialize_field(payload.get(), offset, msg_length);
        std::memcpy(payload.get() + offset, message.c_str(), msg_length);
        
        return payload;
    }

    std::string get_message() const { return message; }
    std::string to_string() const { return "StringMessage(" + message + ")"; }
}; 

class ProposalMessage : public Message {
public:
    enum class Type { Propose, Ack, Nack };
private:
    Type proposal_type;
    int round;
    int proposal_number;
    std::set<int> proposal;

public:
    ProposalMessage(int round, int proposal_number, std::set<int> proposal) : 
        Message(Message::Type::Proposal), round(round), proposal_number(proposal_number), proposal(proposal) {}

    ProposalMessage(ProposalMessage::Type proposal_type, int round, int proposal_number, std::set<int> proposal) :
        ProposalMessage(round, proposal_number, proposal) {
            this->proposal_type = proposal_type;
        }

    ProposalMessage(std::shared_ptr<char[]> payload) : Message(Message::Type::Proposal) { 
        size_t offset = sizeof(Message::Type);
        this->proposal_type = deserialize_field<ProposalMessage::Type>(payload.get(), offset);
        this->round = deserialize_field<int>(payload.get(), offset);
        this->proposal_number = deserialize_field<int>(payload.get(), offset);
        size_t proposal_size = deserialize_field<size_t>(payload.get(), offset);
        this->proposal = std::set<int>();
        for (size_t i = 0; i < proposal_size; i++) {
            this->proposal.insert(deserialize_field<int>(payload.get(), offset));
        }
    }

    static ProposalMessage create_ack(ProposalMessage p) {
        return ProposalMessage(ProposalMessage::Type::Ack, p.round, p.proposal_number, p.proposal);
    }
    static ProposalMessage create_nack(ProposalMessage p, std::set<int> proposal) {
        return ProposalMessage(ProposalMessage::Type::Nack, p.round, p.proposal_number+1, proposal);
    }

    std::shared_ptr<char[]> serialize(size_t &length) {
        length = sizeof(Message::Type) + sizeof(ProposalMessage::Type) + sizeof(round) + sizeof(proposal_number) + sizeof(proposal);

        size_t offset = 0; auto payload = std::shared_ptr<char[]>(new char[length]);
        serialize_field(payload.get(), offset, message_type);
        serialize_field(payload.get(), offset, proposal_type);
        serialize_field(payload.get(), offset, round);
        serialize_field(payload.get(), offset, proposal_number);
        serialize_field(payload.get(), offset, proposal.size());
        for (const auto& value : proposal) {
            serialize_field(payload.get(), offset, value);
        }

        return payload;
    }

    size_t get_round() const { return this->round; }
    size_t get_proposal_number() const { return this->proposal_number; }
    std::set<int> get_proposal() const { return this->proposal; }

    std::string to_string() const {
        std::string result = "ProposalMessage(round=" + std::to_string(this->round) + ", proposal_number=" + std::to_string(this->proposal_number) + ", proposal={ ";
        for (const auto& value : this->proposal) {
            result += std::to_string(value) + " ";
        }
        result += "})";
        return result;
    }
};

class BroadcastMessage : public Message {
private:
    static std::atomic_uint32_t next_id;
    size_t seq_number;
    size_t source_id;
    size_t length;
    std::shared_ptr<char[]> payload;

public:
    BroadcastMessage(size_t seq_number, size_t source_id, size_t length, std::shared_ptr<char[]> payload) : 
        Message(Message::Type::Broadcast), seq_number(seq_number), source_id(source_id), length(length), payload(std::move(payload)) {}

    BroadcastMessage(Message &m, size_t source_id) : Message(Message::Type::Broadcast), seq_number(next_id++), source_id(source_id) {
        this->payload = m.serialize(this->length);
    }

    BroadcastMessage(std::shared_ptr<char[]> payload) : Message(Message::Type::Broadcast) { 
        size_t offset = sizeof(message_type);
        this->seq_number = deserialize_field<size_t>(payload.get(), offset);
        this->source_id = deserialize_field<size_t>(payload.get(), offset);
        this->length = deserialize_field<size_t>(payload.get(), offset);
        this->payload = std::shared_ptr<char[]>(new char[this->length]);
        if (length > 0) { std::memcpy(this->payload.get(), payload.get() + offset, length); }
    }

    std::shared_ptr<char[]> serialize(size_t &length) {
        length = sizeof(this->message_type) + sizeof(this->seq_number) + sizeof(this->source_id) + sizeof(this->length) + this->length;

        size_t offset = 0; std::shared_ptr<char[]> payload(new char[length]);
        serialize_field(payload.get(), offset, this->message_type);
        serialize_field(payload.get(), offset, this->seq_number);
        serialize_field(payload.get(), offset, this->source_id);
        serialize_field(payload.get(), offset, this->length);
        if (this->length > 0) { std::memcpy(payload.get() + offset, this->payload.get(), this->length); } 

        return payload;
    }

    size_t get_seq_number() const { return this->seq_number; }
    size_t get_source_id() const { return this->source_id; }
    size_t get_length() const { return this->length; }
    std::shared_ptr<char[]> get_payload() const { return this->payload; }

    std::string to_string() const {
        std::string result = "BroadcastMessage(";
        result += "seq_number=" + std::to_string(this->seq_number);
        result += ", source_id=" + std::to_string(this->source_id);
        result += ", length=" + std::to_string(this->length);
        result += ")";
        return result;
    }
};


class TransportMessage: public Message {
public:
    enum class Type { Data, Ack };

private:
    static std::atomic_uint32_t next_id;
    Type transport_type;
    Host sender;
    Host receiver;
    size_t seq_number;
    std::shared_ptr<char[]> payload;
    size_t length;

public:
    TransportMessage() : Message(Message::Type::Transport) {}

    TransportMessage(TransportMessage::Type transport_type, Host sender, Host receiver, size_t seq_number, std::shared_ptr<char[]> payload, size_t length) :
        Message(Message::Type::Transport), transport_type(transport_type), sender(sender), receiver(receiver), seq_number(seq_number), payload(std::move(payload)), length(length) {}
    
    TransportMessage(Host sender, Host receiver, std::shared_ptr<char[]> payload, size_t length) :
        Message(Message::Type::Transport), transport_type(TransportMessage::Type::Data), sender(sender), receiver(receiver), seq_number(next_id++), payload(std::move(payload)), length(length) {}

     // Note: Deserializes from raw buffer
     TransportMessage (const char *buffer): Message(Message::Type::Transport) { 
        size_t offset = sizeof(Message::Type);
        this->transport_type = deserialize_field<TransportMessage::Type>(buffer, offset);
        this->sender = deserialize_field<Host>(buffer, offset);
        this->receiver = deserialize_field<Host>(buffer, offset);
        this->seq_number = deserialize_field<size_t>(buffer, offset);
        this->length = deserialize_field<size_t>(buffer, offset);
        this->payload = std::shared_ptr<char[]>(new char[this->length]);
        if (this->length > 0) { std::memcpy(this->payload.get(), buffer + offset, this->length); }
     }

    std::shared_ptr<char[]> serialize(size_t &length) {
        length = sizeof(this->message_type) + sizeof(this->transport_type) + sizeof(this->sender) + sizeof(this->receiver) + sizeof(this->seq_number) + sizeof(this->length) + this->length; 
        size_t offset = 0; auto payload = std::shared_ptr<char[]>(new char[length]);

        serialize_field<Message::Type>(payload.get(), offset, this->message_type);
        serialize_field<TransportMessage::Type>(payload.get(), offset, this->transport_type);
        serialize_field<Host>(payload.get(), offset, this->sender);
        serialize_field<Host>(payload.get(), offset, this->receiver);
        serialize_field<size_t>(payload.get(), offset, this->seq_number);
        serialize_field<size_t>(payload.get(), offset, this->length);
        if (this->length > 0) { std::memcpy(payload.get() + offset, this->payload.get(), this->length); }

        return payload;
    }

    // Create ACK
    static TransportMessage create_ack(const TransportMessage tm) {
        return TransportMessage(TransportMessage::Type::Ack, tm.receiver, tm.sender, tm.seq_number, nullptr, 0);
    }

    // Getters
    size_t get_seq_number() const { return this->seq_number; }
    Host get_sender() const { return this->sender; }
    Host get_receiver() const { return this->receiver; }
    size_t get_length() const { return this->length; }
    bool is_ack() const { return (this->transport_type == TransportMessage::Type::Ack); }
    std::shared_ptr<char[]> get_payload() const {
        auto copy = std::shared_ptr<char[]>(new char[length]);
        std::memcpy(copy.get(), payload.get(), length);
        return copy;
    }

    std::string to_string() const {
        std::string result = "TransportMessage(";
        result += "seq_number=" + std::to_string(this->seq_number);
        result += ", sender=" + std::to_string(this->sender.get_id());
        result += ", receiver=" + std::to_string(this->receiver.get_id()); 
        result += ", is_ack=" + std::string(is_ack() ? "true" : "false");
        result += ", length=" + std::to_string(this->length); 
        result += ")";
        return result;
    }
};

// Sequence numbers
const size_t SEQ_NUM_INIT = 0;
std::atomic_uint32_t TransportMessage::next_id{SEQ_NUM_INIT};
std::atomic_uint32_t BroadcastMessage::next_id{SEQ_NUM_INIT};