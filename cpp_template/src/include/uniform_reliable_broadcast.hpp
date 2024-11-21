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
    PerfectLink pl;

    bool can_deliver(BroadcastMessage bm) {
        std::cout << "can_deliver" << std::endl;
        size_t min_correct_hosts = static_cast<size_t>(this->hosts.get_host_count() / 2) + 1;
        std::cout << "min_correct_hosts" << min_correct_hosts << std::endl;
        size_t source_id = bm.get_source_id();
        size_t message_id = bm.get_seq_number();
        size_t count = 0;
        for (auto host: this->hosts.get_hosts()) {
            size_t sender_id = host.get_id();
            if (this->acked_messages.contains(source_id, sender_id, message_id)) {
                count++;
            }
        }
        return count >= min_correct_hosts;
    }

    void deliver(BroadcastMessage bm, Host sender) {
        std::cout << "urbReceive: " << bm << std::endl;

        // // Add broadcast message to ACK set (I know that the sender has seen this broadcast message from source)
        size_t sender_id = sender.get_id();
        size_t source_id = bm.get_source_id();
        size_t message_id = bm.get_seq_number();
        this->acked_messages.insert(source_id, sender_id, message_id);


        // If not pending, then add to pending set and relay
        bool not_pending = !this->pending_messages.contains(source_id, bm.get_seq_number());
        if (not_pending) {
            std::cout << "Relaying: " << bm << std::endl;
            this->pending_messages.insert(source_id, bm.get_seq_number());
            this->pl.broadcast(bm);
            return;
        } 


        // // Else, check majority if message can be delivered
        bool can_deliver = this->can_deliver(std::move(bm));
        std::cout << "can_deliver: " << can_deliver << std::endl;
        this->handler(std::move(bm));
        // bool not_delivered_yet = !this->delivered_messages.contains(source_id, bm.get_seq_number());
        // if (can_deliver && not_delivered_yet) {
        //     std::cout << "urbDeliver: " << bm << std::endl;
        //     this->delivered_messages.insert(source_id, bm.get_seq_number());
        //     this->handler(std::move(bm));
        // }
    }

public:
    UniformReliableBroadcast(Host local_host, Hosts hosts, std::function<void(BroadcastMessage)> handler): 
        host(local_host), hosts(hosts), pending_messages(hosts), delivered_messages(hosts), acked_messages(hosts), handler(handler),
        pl(local_host, hosts, [this](TransportMessage tm) { 
            this->deliver(BroadcastMessage(tm.get_payload(), tm.get_length()), tm.get_sender());
        }) {
        std::cout << "Setting up URB at " << local_host.get_address().to_string() << std::endl;
    }

    void broadcast(Message &m) {
        size_t source_id = (this->host.get_id());
        auto bm = BroadcastMessage(m, source_id);
        std::cout << "urbBroadcast: " << bm << std::endl;
        for (auto host: this->hosts.get_hosts()) {
            this->pending_messages.insert(host.get_id(), bm.get_seq_number());
        }
        this->pl.broadcast(bm);
    }

    void shutdown() {
        this->pl.shutdown();
    }
};
