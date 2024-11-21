#pragma once

#include <map>
#include <set>
#include <mutex>

#include "hosts.hpp"

/**
 * @brief A set of messages per process.
 *
 * Supports insertion and query.
 */
class MessageSet {
private:
    std::map<size_t, std::set<size_t>> messages;
    std::mutex lock;

public:
    MessageSet(Hosts hosts) {
        this->lock.lock();
        for (const auto& host : hosts.get_hosts()) {
            messages[host.get_id()] = std::set<size_t>();
        }
        this->lock.unlock();
    }

    void insert(size_t process_id, size_t message_id) {
        this->lock.lock();
        this->messages[process_id].insert(message_id);
        this->lock.unlock();
    }

    bool contains(size_t process_id, size_t message_id) {
        this->lock.lock();
        bool result = this->messages[process_id].count(message_id) > 0;
        this->lock.unlock();
        return result;
    }
};


/**
 * @brief A set of messages per process pair (source, sender).
 * 
 * Supports insertion and query.
 */
class MessagePairSet {
private:
    std::map<std::pair<size_t, size_t>, std::set<size_t>> messages;
    std::mutex lock;

public:
    MessagePairSet(Hosts hosts) {
        this->lock.lock();
        for (const auto& host : hosts.get_hosts()) {
            messages[{host.get_id(), host.get_id()}] = std::set<size_t>();
        }
        this->lock.unlock();
    }

    void insert(size_t source_id, size_t sender_id, size_t message_id) {
        this->lock.lock();
        this->messages[{source_id, sender_id}].insert(message_id);
        this->lock.unlock();
    }

    bool contains(size_t source_id, size_t sender_id, size_t message_id) {
        this->lock.lock();
        bool result = this->messages[{source_id, sender_id}].count(message_id) > 0;
        this->lock.unlock();
        return result;
    }
};