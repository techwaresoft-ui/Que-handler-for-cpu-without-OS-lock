#pragma once

#include <atomic>
#include <vector>
#include <optional>

template <typename T, size_t Capacity>
class LockFreeRingBuffer {
private:
    struct Node {
        T data;
    };

    std::vector<Node> buffer;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

public:
    LockFreeRingBuffer() : buffer(Capacity) {}

    // Push an item into the queue
    bool push(const T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t current_head = head.load(std::memory_order_acquire);

        // Check if buffer is full
        if ((current_tail - current_head) >= Capacity) {
            return false; // Queue full
        }

        buffer[current_tail % Capacity].data = item;
        tail.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    // Pop an item from the queue
    bool pop(T& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_acquire);

        // Check if buffer is empty
        if (current_head == current_tail) {
            return false; // Queue empty
        }

        item = buffer[current_head % Capacity].data;
        head.store(current_head + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }
};
