#pragma once

#include <set>

#include "best_effort_broadcast.hpp"

/** @brief Lattice Agreement (LA) using Best-Effort Broadcast (BEB) */
class LatticeAgreement {
private:
    bool active;
    size_t ack_count;
    size_t nack_count;
    size_t active_proposal_number;
    std::set<int> active_proposal;
    std::set<int> accepted_proposal;
    Hosts hosts;
    BestEffortBroadcast beb;
    std::function<void(std::set<int>)> decide;

    void propose() {
        this->ack_count = 0;
        this->nack_count = 0;
        this->active_proposal_number++;
        this->active_proposal = this->active_proposal;

        ProposalMessage pm(0, this->active_proposal_number, this->active_proposal);
        std::cout << "laPropose: " << pm << std::endl;
        this->beb.broadcast(pm);
    }

    void bebDeliver(TransportMessage tm) {
        ProposalMessage pm(tm.get_payload());
        std::cout << "laDeliver: " << pm << " from " << tm.get_sender() << std::endl;

        auto type = pm.get_type();
        auto proposal = pm.get_proposal();

        if (type == ProposalMessage::Type::Propose) {
            bool is_empty = this->accepted_proposal.empty();
            bool is_subset = std::includes(this->accepted_proposal.begin(), this->accepted_proposal.end(), proposal.begin(), proposal.end());
            if (is_empty || is_subset) {
                this->accepted_proposal = proposal;
                ProposalMessage ack = ProposalMessage::create_ack(pm);
                this->beb.send(ack, tm.get_sender());
            } else if (!is_subset) {
                this->accepted_proposal.insert(proposal.begin(), proposal.end());
                ProposalMessage nack = ProposalMessage::create_nack(pm, this->accepted_proposal);
                this->beb.send(nack, tm.get_sender());
            }
        } else if (type == ProposalMessage::Type::Ack) {
            if (this->active_proposal_number == pm.get_proposal_number()) {
                this->ack_count++;
            }
        } else if (type == ProposalMessage::Type::Nack) {
            if (this->active_proposal_number == pm.get_proposal_number()) {
                this->nack_count++;
                this->active_proposal.insert(proposal.begin(), proposal.end());
            }
        }

        if (this->nack_count > 0 && this->ack_count + this->nack_count >= this->hosts.get_host_count() && this->active) {
            this->propose();
        }

        if (this->ack_count >= this->hosts.get_host_count() && this->active) {
            this->decide(this->active_proposal);
            this->active = false;
        }
    }

public:
    LatticeAgreement(Host local_host, Hosts hosts, std::function<void(std::set<int>)> decide) :
        active(false), ack_count(0), nack_count(0), active_proposal_number(0), active_proposal(std::set<int>()), hosts(hosts),
        beb(local_host, hosts, [this](TransportMessage tm) { this->bebDeliver(std::move(tm)); }),
        decide(decide) {}

    void propose(std::set<int> proposal) {
        this->active = true;
        this->active_proposal = proposal;
        this->propose();
    }

    void shutdown() {
        this->beb.shutdown();
    }
};