#pragma once

#include <set>
#include <map>
#include <functional>

#include "host.hpp"
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
    std::function<void(DataMessage)> send_handler;
    std::function<void(DataMessage, Host)> deliver_handler;
    std::map<size_t, std::set<size_t>> pending_messages; // Set of message IDs of pending messages to sender host_id


public:
    UniformReliableBroadcast(Host local_host, Hosts hosts, std::function<void(TransportMessage)> handler): hosts(hosts), beb(local_host, hosts, handler) {
        for (auto host: hosts.get_hosts()) {
            pending_messages[host.get_id()];
        }
    }

    void broadcast(const DataMessage &message, bool immediate = false) {
        // Problem: I broadcast a message at which point I only know the std::string
        // I call bebBroadcast which iterates over hosts to call plSend
        // plSend adds to a send buffer
        // After 8 messages are in buffer, a TransportMessage with an seq_number is created
        // However, I need to know the seq number before so I can remember that the message with this seq number is pending
        for (auto host: this->hosts.get_hosts()) {
            this->pending_messages[host.get_id()].insert(message.get_id());
        }
        this->beb.broadcast(message, immediate);
    }

    void shutdown() {
        this->beb.shutdown();
    }
};
