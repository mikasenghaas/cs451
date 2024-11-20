#pragma once

#include <set>
#include <map>
#include <iostream>
#include <functional>

#include "hosts.hpp"
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
    Hosts hosts;
    BestEffortBroadcast beb;
    std::map<size_t, std::set<size_t>> pending_messages;

public:
    UniformReliableBroadcast(Host local_host, Hosts hosts, std::function<void(TransportMessage)> handler): hosts(hosts), beb(local_host, hosts, handler) {
        std::cout << "Setting up uniform reliable broadcast at " << local_host.get_address().to_string() << std::endl;
        for (auto host: this->hosts.get_hosts()) {
            this->pending_messages[host.get_id()];
        }
    }

    void broadcast(Message &m) {
        auto bm = BroadcastMessage(m);
        std::cout << "urbBroadcast" << bm << std::endl;
        for (auto host: this->hosts.get_hosts()) {
            this->pending_messages[host.get_id()].insert(bm.get_seq_number());
        }
        this->beb.broadcast(bm);
    }

    void shutdown() {
        this->beb.shutdown();
    }
};
