#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <queue>
#include <mutex>
#include "LockFreeRingBuffer.h"

// 1. Standard Locked Queue for baseline comparison
template <typename T>
class MutexQueue {
    std::queue<T> q;
    std::mutex mtx;
public:
    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(item);
        return true;
    }
    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        if (q.empty()) return false;
        item = q.front();
        q.pop();
        return true;
    }
};

// Benchmark runner function
template <typename QueueType>
void run_benchmark(QueueType& q, const std::string& name, int num_threads, int items_per_thread) {
    auto start = std::chrono::high_resolution_clock::now();

    // Spawn producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
        producers.emplace_back([&q, items_per_thread, i]() {
            for (int j = 0; j < items_per_thread; ++j) {
                while (!q.push(j)) {
                    // Spin-wait if queue is full (for bounded buffer)
                    std::this_thread::yield();
                }
            }
        });
    }

    // Spawn consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < num_threads; ++i) {
        consumers.emplace_back([&q, items_per_thread]() {
            int val;
            int popped = 0;
            while (popped < items_per_thread) {
                if (q.pop(val)) {
                    popped++;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Join threads
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "[" << name << "] Completed in: " << elapsed.count() << " ms\n";
}

int main() {
    const int NUM_THREADS = 4;
    const int ITEMS_PER_THREAD = 250000; // Total 1 million operations per test

    std::cout << "Starting Concurrency Benchmarks (" << NUM_THREADS << " threads, " 
              << (NUM_THREADS * ITEMS_PER_THREAD) << " items)...\n\n";

    // Test 1: Standard Mutex Queue
    MutexQueue<int> mutex_q;
    run_benchmark(mutex_q, "Standard Mutex Queue", NUM_THREADS, ITEMS_PER_THREAD);

    // Test 2: Lock-Free Ring Buffer
    LockFreeRingBuffer<int, 1048576> lock_free_q;
    run_benchmark(lock_free_q, "Lock-Free Ring Buffer", NUM_THREADS, ITEMS_PER_THREAD);

    return 0;
}
