#pragma once

#include <set>

#include "best_effort_broadcast.hpp"

/** @brief Lattice Agreement (LA) using Best-Effort Broadcast (BEB)
 * 
 */
class LatticeAgreement {
private:
    BestEffortBroadcast beb;
    std::function<void(ProposalMessage)> decide;

    void bebDeliver(TransportMessage tm) {
        std::cout << "bebDeliver: " << tm << std::endl;
        ProposalMessage pm(tm.get_payload());
        this->decide(pm);
    }

public:
    LatticeAgreement(Host local_host, Hosts hosts, std::function<void(ProposalMessage)> decide) :
        beb(local_host, hosts, [this](TransportMessage tm) { this->bebDeliver(std::move(tm)); }),
        decide(decide) {}

    void propose(std::set<int> proposal) {
        ProposalMessage pm(0, 0, proposal);
        std::cout << "laPropose: " << pm << std::endl;
        this->beb.broadcast(pm);
    }

    void shutdown() {
        this->beb.shutdown();
    }
};