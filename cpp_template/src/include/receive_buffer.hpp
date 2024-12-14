#pragma once

#include <map>
#include <queue>
#include <vector>
#include <mutex>

#include "hosts.hpp"
#include "message.hpp"
#include "message_set.hpp"

class BroadcastPriorityQueue {
private:
    struct MessageComparator {
        bool operator()(const BroadcastMessage& a, const BroadcastMessage& b) {
            return a.get_seq_number() > b.get_seq_number();
        }
    };
    std::priority_queue<BroadcastMessage, std::vector<BroadcastMessage>, MessageComparator> message_heap;

public:
    BroadcastPriorityQueue() : 
        message_heap(std::priority_queue<BroadcastMessage, std::vector<BroadcastMessage>, MessageComparator>()) {}

    void add_message(BroadcastMessage message) {
        this->message_heap.push(std::move(message));
    }

    BroadcastMessage remove_message() {
        BroadcastMessage result = std::move(this->message_heap.top());
        this->message_heap.pop();
        return result;
    }

    BroadcastMessage const& front() {
        BroadcastMessage const& result = this->message_heap.top();
        return result;
    }

    void pop() {
        this->message_heap.pop();
    }

    bool empty() {
        return this->message_heap.empty();
    }
};

class ReceiveBuffer {
private:
    std::map<size_t, BroadcastPriorityQueue> messages;
    std::map<size_t, size_t> next_seq_nums;
    std::mutex lock;

    bool has_next_message(size_t source_id) {
        return !this->messages[source_id].empty() && 
            this->messages[source_id].front().get_seq_number() == this->next_seq_nums[source_id];
    }

public:
    ReceiveBuffer(Hosts hosts) {
        for (const auto& host : hosts.get_hosts()) {
            this->messages[host.get_id()] = BroadcastPriorityQueue();
            this->next_seq_nums[host.get_id()] = SEQ_NUM_INIT;
        }
    }

    std::vector<BroadcastMessage> deliver(BroadcastMessage bm) {
        this->lock.lock();
        size_t source_id = bm.get_source_id();
        std::vector<BroadcastMessage> result;
        
        // Add the current message
        this->messages[source_id].add_message(bm);
        
        // Collect all deliverable messages
        while (this->has_next_message(source_id)) {
            BroadcastMessage bm = this->messages[source_id].remove_message();
            result.push_back(bm);
            this->next_seq_nums[source_id]++;
        }
        
        this->lock.unlock();
        return result;
    }
};

class LatticeReceiveBuffer {
private:
    std::map<size_t, std::set<int>> proposals;
    size_t next_round;
    std::mutex lock;

    bool has_next_round(size_t round) {
        return !this->proposals[round].empty();
    }

public:
    LatticeReceiveBuffer(Hosts hosts) {
        this->proposals = std::map<size_t, std::set<int>>();
        this->next_round = 0;
    }

    std::vector<std::set<int>> deliver(ProposalMessage pm) {
        this->lock.lock();

        // Add proposal to buffer
        this->proposals[pm.get_round()] = pm.get_proposal();
        
        // Collect all deliverable messages
        std::vector<std::set<int>> result;
        while (this->has_next_round(this->next_round)) {
            std::set<int> proposal = this->proposals[this->next_round];
            result.push_back(proposal);
            this->next_round++;
        }
        
        this->lock.unlock();
        return result;
    }
};