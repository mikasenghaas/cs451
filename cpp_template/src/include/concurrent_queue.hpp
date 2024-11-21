#pragma once

#include <queue>
#include <mutex>

template <typename T>
class ConcurrentQueue {
private:
    std::queue<T> queue;
    std::mutex lock;

public:
    ConcurrentQueue() {}

    void push(T item) {
        this->lock.lock();
        this->queue.push(item);
        this->lock.unlock();
    }

    T pop() {
        this->lock.lock();
        T item = this->queue.front();
        this->queue.pop();
        this->lock.unlock();
        return item;
    }

    bool empty() {
        this->lock.lock();
        bool result = this->queue.empty();
        this->lock.unlock();
        return result;
    }
};
