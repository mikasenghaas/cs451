#pragma once

#include <set>
#include <map>
#include <condition_variable>

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
    // Limit sending pace
    Round last_decided = 0;
    const size_t SEND_QUEUE_SIZE = 200;
    std::condition_variable cv;
    std::mutex lock;
    std::mutex cv_mutex;
    bool stop_sending = false;

    void bebDeliver(TransportMessage tm) {
        ProposalMessage pm(tm.get_payload());
        std::cout << "laDeliver: " << pm << " from " << tm.get_sender() << std::endl;

        auto type = pm.get_type();
        auto round = pm.get_round();
        auto proposal = pm.get_proposal();

        this->lock.lock();
        if (type == ProposalMessage::Type::Propose) {
            if (LatticeAgreement::is_subset(this->accepted_proposal[round], proposal)) {
                this->accepted_proposal[round] = proposal;
                ProposalMessage ack = ProposalMessage::create_ack(pm);
                this->beb.send(ack, tm.get_sender());
            } else {
                LatticeAgreement::set_union(this->accepted_proposal[round], proposal);
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
                LatticeAgreement::set_union(this->active_proposal[round], proposal);
            }
        }
        this->lock.unlock();

        if (this->active[round] && this->nack_count[round] > 0 && this->ack_count[round] + this->nack_count[round] >= this->threshold) {
            this->propose(round, this->active_proposal[round]);
        }

        if (this->active[round] && this->ack_count[round] >= this->threshold) {
            this->active[round] = false;
            std::vector<Proposal> proposals = this->receive_buffer.deliver(pm);
            auto decided = 0;
            for (const auto& proposal: proposals) {
                this->decide(proposal);
                decided++;
            }
            {
                std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
                this->last_decided += decided;
            }
            this->cv.notify_all();
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
        threshold(static_cast<size_t>(hosts.get_host_count())) {}

    void propose(Round round, Proposal proposal) {
        {
            std::unique_lock<std::mutex> lock(this->cv_mutex);
            while (round - this->last_decided > this->SEND_QUEUE_SIZE) {
                this->cv.wait(lock);
                if (this->stop_sending) { return; }
            }
        }
        if (this->stop_sending) { return; }

        this->lock.lock();
        this->active[round] = true;
        this->ack_count[round] = 0;
        this->nack_count[round] = 0;
        this->active_proposal_number[round]++;
        this->active_proposal[round] = proposal;
        ProposalMessage pm(round, this->active_proposal_number[round], this->active_proposal[round]);
        this->lock.unlock();

        std::cout << "laPropose: " << pm << std::endl;
        this->beb.broadcast(pm);
    }

    void shutdown() {
        {
            std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
            this->stop_sending = true;
        }
        this->cv.notify_all();
        this->beb.shutdown();
    }

private:
    static void set_union(Proposal &dest, Proposal &source) {
        for (auto value : source) {
            dest.insert(value);
        }
    }

    static bool is_subset(Proposal &subset, Proposal &superset) {
        if (superset.size() < subset.size()) { return false; }

        for (auto element : subset) {
            if (superset.count(element) == 0) {
                return false;
            }
        }

        return true;
    }
};