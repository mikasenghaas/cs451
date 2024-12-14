#pragma once

#include <iostream>
#include <functional>

#include "hosts.hpp"
#include "message.hpp"
#include "perfect_link.hpp"

/**
 * @brief Best-Effort Broadcast (BEB) using Perfect Link (PL)
 * 
 * @details Supports broadcasting a message to all processes in the system
 * and delivery of messages to all processes in the system. Satisfies the 
 * best-effort properties:
 * - (BEB1: Validity) If process p sends message m, then every correct process 
 *   eventually delivers m.
 * - (BEB2: No Duplication) No message is delivered more than once.
 * - (BEB3: No Creation) If a process delivers a message m, then m must have been 
 *   sent by some process.
 */
class BestEffortBroadcast {
private:
    Hosts hosts;
    PerfectLink pl;

public:
    BestEffortBroadcast(Host local_host, Hosts hosts, std::function<void(TransportMessage)> bebDeliver) :
        hosts(hosts), pl(local_host, hosts, bebDeliver) {}

    void broadcast(Message &m) {
        for (auto host : this->hosts.get_hosts()) {
            this->pl.send(m, host);
        }
    }

    void send(Message &m, Host host) {
        this->pl.send(m, host);
    }

    void shutdown() {
        this->pl.shutdown();
    }
};
