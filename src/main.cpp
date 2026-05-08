#include <iostream>
#include <thread>

#include "spsc_queue.h"

int main() {
    SPSCQueue<int, 1024> queue;
    constexpr int NUM_MESSAGES = 512;

    std::thread producer([&]() {
        for (int i = 0; i <= NUM_MESSAGES; ++i) {
            while (!queue.try_push(i));
        }
    });

    std::thread consumer([&]() {
        int received = 0;
        while (received < NUM_MESSAGES) {
            if (auto val = queue.try_pop()) {
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();
}
