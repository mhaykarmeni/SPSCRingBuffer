#include <iostream>
#include <thread>

#include "spsc_queue.h"

int main() {
    SPSCQueueLF<int, 1024> queue;
    constexpr int NUM_MESSAGES = 512;
    bool ok = true;

    std::thread producer([&]() {
        for (int i = 0; i < NUM_MESSAGES; ++i)
            while (!queue.try_push(i));
    });

    std::thread consumer([&]() {
        for (int i = 0; i < NUM_MESSAGES; ++i) {
            int val;
            while (true) {
                if (auto v = queue.try_pop()) { val = *v; break; }
            }
            if (val != i) ok = false;
        }
    });

    producer.join();
    consumer.join();

    std::cout << (ok ? "OK: all " : "FAIL: ")
              << NUM_MESSAGES << " messages received in order\n";
}
