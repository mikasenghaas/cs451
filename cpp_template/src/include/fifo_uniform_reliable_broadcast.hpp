
#pragma once

#include "message_set.hpp"
#include "receive_buffer.hpp"
#include "uniform_reliable_broadcast.hpp"
#include "hosts.hpp"

/**
 * @brief FIFO-Order Uniform Reliable Broadcast (FRB)
 * 
 * @details Supports broadcasting a message to all processes in the system
 * and delivery of messages to all processes in the system satisfying FIFO-order
 * and uniform agreement.
 * - (FRB1: Validity) If a correct process p broadcasts a message m, then p
 *   eventually delivers m.
 * - (FRB2: No Duplication) No message is delivered more than once.
 * - (FRB3: No Creation) If a process delivers a message m with sender s, then 
 *   m must have been broadcast by s.
 * - (FRB4: Uniform Agreement) If m is delivered by some process (whether correct
 *   or faulty), then m is eventually delivered by every correct process.
 * - (FRB5: FIFO Order) If some process broadcasts message m before it
 *   broadcasts message n, then no correct process delivers n unless it has
 *   already delivered m.
 */
class FIFOUniformReliableBroadcast {
private:
    UniformReliableBroadcast urb;
    ReceiveBuffer receive_buffer;
    std::function<void(BroadcastMessage)> handler;

    void deliver(BroadcastMessage bm) {
        auto bms = this->receive_buffer.deliver(bm);
        for (auto &bm : bms) {
            std::cout << "frbDeliver: " << bm << std::endl;
            this->handler(bm);
        }
    }

public:
    FIFOUniformReliableBroadcast(Host host, Hosts hosts, std::function<void(BroadcastMessage)> handler):
        urb(host, hosts, [this](BroadcastMessage bm) { this->deliver(std::move(bm)); }),
        receive_buffer(hosts), handler(handler) {}

    void broadcast(Message &m) {
        this->urb.broadcast(m);
    }

    void shutdown() {
        this->urb.shutdown();
    }
};
