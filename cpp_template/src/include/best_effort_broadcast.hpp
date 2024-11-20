#pragma once

#include <functional>

#include "hosts.hpp"
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
    PerfectLink pl;

public:
    BestEffortBroadcast(const Host local_host, const Hosts hosts, std::function<void(TransportMessage)> deliver): pl(local_host, hosts, deliver) {}

    void broadcast(const DataMessage message) {
        this->pl.broadcast(message);
    }

    void shutdown() {
        this->pl.shutdown();
    }
};
