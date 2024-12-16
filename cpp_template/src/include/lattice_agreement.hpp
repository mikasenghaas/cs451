#pragma once

#include <set>
#include <map>

#include "best_effort_broadcast.hpp"
#include "receive_buffer.hpp"
#include "message.hpp"

/** @brief Lattice Agreement (LA) using Best-Effort Broadcast (BEB) */
class LatticeAgreement {
private:
    std::map<size_t, bool> active;
    std::map<size_t, size_t> ack_count;
    std::map<size_t, size_t> nack_count;
    std::map<size_t, size_t> active_proposal_number;
    std::map<size_t, Proposal> active_proposal;
    std::map<size_t, Proposal> accepted_proposal;
    Hosts hosts;
    BestEffortBroadcast beb;
    std::function<void(Proposal)> decide;
    LatticeReceiveBuffer receive_buffer;
    size_t threshold;

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

        if (this->active[round] && this->nack_count[round] > 0 && this->ack_count[round] + this->nack_count[round] >= this->threshold) {
            this->propose(round, this->active_proposal[round]);
        }

        if (this->active[round] && this->ack_count[round] >= this->threshold) {
            this->active[round] = false;
            std::vector<Proposal> proposals = this->receive_buffer.deliver(pm);
            for (const auto& proposal: proposals) {
                this->decide(proposal);
            }
        }
    }

public:
    LatticeAgreement(Host local_host, Hosts hosts, std::function<void(Proposal)> decide) :
        active(std::map<Round, bool>()),
        ack_count(std::map<Round, size_t>()),
        nack_count(std::map<Round, size_t>()),
        active_proposal_number(std::map<Round, size_t>()),
        active_proposal(std::map<size_t, Proposal>()),
        accepted_proposal(std::map<size_t, Proposal>()),
        hosts(hosts),
        beb(local_host, hosts, [this](TransportMessage tm) { this->bebDeliver(std::move(tm)); }),
        decide(decide),
        receive_buffer(hosts),
        threshold(static_cast<size_t>(hosts.get_host_count() / 2 + 1)) {}

    void propose(Round round, Proposal proposal) {
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