#pragma once

#include <set>
#include <map>

#include "best_effort_broadcast.hpp"

/** @brief Lattice Agreement (LA) using Best-Effort Broadcast (BEB) */
class LatticeAgreement {
private:
    std::map<size_t, bool> active;
    std::map<size_t, size_t> ack_count;
    std::map<size_t, size_t> nack_count;
    std::map<size_t, size_t> active_proposal_number;
    std::map<size_t, std::set<int>> active_proposal;
    std::map<size_t, std::set<int>> accepted_proposal;
    Hosts hosts;
    BestEffortBroadcast beb;
    std::function<void(std::set<int>)> decide;

    void bebDeliver(TransportMessage tm) {
        ProposalMessage pm(tm.get_payload());
        std::cout << "laDeliver: " << pm << " from " << tm.get_sender() << std::endl;

        auto type = pm.get_type();
        auto round = pm.get_round();
        auto proposal = pm.get_proposal();

        if (type == ProposalMessage::Type::Propose) {
            bool is_empty = this->accepted_proposal[round].empty();
            bool is_subset = std::includes(this->accepted_proposal[round].begin(), this->accepted_proposal[round].end(), proposal.begin(), proposal.end());
            if (is_empty || is_subset) {
                this->accepted_proposal[round] = proposal;
                ProposalMessage ack = ProposalMessage::create_ack(pm);
                this->beb.send(ack, tm.get_sender());
            } else if (!is_subset) {
                this->accepted_proposal[round].insert(proposal.begin(), proposal.end());
                ProposalMessage nack = ProposalMessage::create_nack(pm, this->accepted_proposal[round]);
                this->beb.send(nack, tm.get_sender());
            }
        } else if (type == ProposalMessage::Type::Ack) {
            if (this->active_proposal_number[round] == pm.get_proposal_number()) {
                this->ack_count[round]++;
            }
        } else if (type == ProposalMessage::Type::Nack) {
            if (this->active_proposal_number[round] == pm.get_proposal_number()) {
                this->nack_count[round]++;
                this->active_proposal[round].insert(proposal.begin(), proposal.end());
            }
        }

        if (this->nack_count[round] > 0 && this->ack_count[round] + this->nack_count[round] >= this->hosts.get_host_count() && this->active[round]) {
            this->propose(round, this->active_proposal[round]);
        }

        if (this->ack_count[round] >= this->hosts.get_host_count() && this->active[round]) {
            this->decide(this->active_proposal[round]);
            this->active[round] = false;
        }
    }

public:
    LatticeAgreement(Host local_host, Hosts hosts, std::function<void(std::set<int>)> decide) :
        active(std::map<size_t, bool>()),
        ack_count(std::map<size_t, size_t>()),
        nack_count(std::map<size_t, size_t>()),
        active_proposal_number(std::map<size_t, size_t>()),
        active_proposal(std::map<size_t, std::set<int>>()),
        accepted_proposal(std::map<size_t, std::set<int>>()), hosts(hosts),
        beb(local_host, hosts, [this](TransportMessage tm) { this->bebDeliver(std::move(tm)); }),
        decide(decide) {}

    void propose(size_t round, std::set<int> proposal) {
        this->active[round] = true;
        this->ack_count[round] = 0;
        this->nack_count[round] = 0;
        this->active_proposal_number[round]++;
        this->active_proposal[round] = proposal;

        ProposalMessage pm(round, this->active_proposal_number[round], this->active_proposal[round]);
        std::cout << "laPropose: " << pm << std::endl;
        this->beb.broadcast(pm);
    }

    void shutdown() {
        this->beb.shutdown();
    }
};