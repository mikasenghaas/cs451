#pragma once

#include <set>
#include <map>
#include <iostream>
#include <functional>

#include "hosts.hpp"
#include "message_set.hpp"
#include "best_effort_broadcast.hpp"

/**
 * @brief Uniform Reliable Broadcast (URB) via Majority-Ack Algorithm
 * 
 * @details Supports broadcasting a message to all processes in the system
 * and delivery of messages to all processes in the system satisfying uniform
 * agreement.
 * - (URB1: Validity) If a correct process p broadcasts a message m, then p
 *   eventually delivers m.
 * - (URB2: No Duplication) No message is delivered more than once.
 * - (URB3: No Creation) If a process delivers a message m with sender s, then 
 *   m must have been broadcast by s.
 * - (URB4: Uniform Agreement) If m is delivered by some process (whether correct
 *   or faulty), then m is eventually delivered by every correct process.
 */
class UniformReliableBroadcast {
private:
    Host host;
    Hosts hosts;
    MessageSet pending_messages;
    MessageSet delivered_messages;
    MessagePairSet acked_messages;
    std::function<void(BroadcastMessage)> handler;
    BestEffortBroadcast beb;

    bool can_deliver(const BroadcastMessage &bm) {
        size_t min_correct_hosts = static_cast<size_t>(this->hosts.get_host_count() / 2) + 1;
        size_t source_id = bm.get_source_id();
        size_t count = 0;
        for (auto host: this->hosts.get_hosts()) {
            size_t sender_id = host.get_id();
            if (this->acked_messages.contains(source_id, sender_id, bm.get_seq_number())) {
                count++;
            }
        }
        return count >= min_correct_hosts;
    }

    void deliver(BroadcastMessage bm, Host sender) {
        // std::cout << "urbReceive: " << bm << std::endl;

        // Add broadcast message to ACK set (I know that the sender has seen this broadcast message from source)
        size_t sender_id = sender.get_id();
        size_t source_id = bm.get_source_id();
        size_t message_id = bm.get_seq_number();
        this->acked_messages.insert(source_id, sender_id, message_id);

        // If not pending, then add to pending set and relay
        bool pending = this->pending_messages.contains(source_id, message_id);
        if (!pending) {
            // std::cout << "urbRelay: " << bm << std::endl;
            this->pending_messages.insert(source_id, bm.get_seq_number());
            this->beb.broadcast(bm);
            return;
        } 

        // Else, check majority if message can be delivered
        bool can_deliver = this->can_deliver(bm);
        bool already_delivered = this->delivered_messages.contains(source_id, bm.get_seq_number());
        if (can_deliver && !already_delivered) {
            // std::cout << "urbDeliver: " << bm << std::endl;
            this->delivered_messages.insert(source_id, bm.get_seq_number());
            this->handler(std::move(bm));
        }
    }

public:
    UniformReliableBroadcast(Host local_host, Hosts hosts, std::function<void(BroadcastMessage)> handler): 
        host(local_host), hosts(hosts), pending_messages(hosts), delivered_messages(hosts), acked_messages(hosts), handler(handler),
        beb(local_host, hosts, [this](TransportMessage tm) { 
            this->deliver(BroadcastMessage(tm.get_payload()), tm.get_sender());
        }) {
        // std::cout << "Setting up URB at " << local_host.get_address().to_string() << std::endl;
    }

    void broadcast(Message &m) {
        size_t source_id = (this->host.get_id());
        auto bm = BroadcastMessage(m, source_id);
        this->pending_messages.insert(source_id, bm.get_seq_number());
        // std::cout << "urbBroadcast: " << bm << std::endl;
        this->beb.broadcast(bm);
    }

    void shutdown() {
        this->beb.shutdown();
    }
};
