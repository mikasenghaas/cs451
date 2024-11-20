#pragma once

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
    Host local_host;
    Hosts hosts;
    PerfectLink pl;
    std::function<void(DataMessage)> send_handler;
    std::function<void(DataMessage, Host)> deliver_handler;

public:
    BestEffortBroadcast(const Host local_host, const Hosts hosts, std::function<void(DataMessage)> send_handler, std::function<void(DataMessage, Host)> deliver_handler): local_host(local_host), hosts(hosts), pl(local_host, hosts), send_handler(send_handler), deliver_handler(deliver_handler) {
        this->pl.start_receiving(deliver_handler);
        this->pl.start_sending();
    }

    void broadcast(const DataMessage &message, bool immediate = false) {
        this->send_handler(message);
        for (auto host : this->hosts.get_hosts()) {
            size_t receiver_id = host.get_id();
            Host receiver_host(receiver_id, this->hosts.get_address(receiver_id));
            this->pl.send(message, receiver_host, immediate);
        }
    }

    void shutdown() {
        this->pl.shutdown();
    }
};
