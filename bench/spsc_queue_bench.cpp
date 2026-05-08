#include <benchmark/benchmark.h>
#include <thread>
#include "spsc_queue.h"

static void BM_PushOnly(benchmark::State& state) {
    SPSCQueue<int, 1024> q;
    int i = 0;
    for (auto _ : state) {
        if (!q.try_push(i++))
            q.try_pop();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PushOnly);

static void BM_PopOnly(benchmark::State& state) {
    SPSCQueue<int, 1024> q;
    for (int i = 0; i < 1024; ++i) q.try_push(i);
    for (auto _ : state) {
        if (!q.try_pop())
            q.try_push(0);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PopOnly);

static void BM_Throughput(benchmark::State& state) {
    SPSCQueue<int, 4096> q;
    constexpr int BATCH = 100'000;

    for (auto _ : state) {
        std::thread producer([&]() {
            for (int i = 0; i < BATCH; ++i)
                while (!q.try_push(i));
        });
        std::thread consumer([&]() {
            int received = 0;
            while (received < BATCH) {
                if (q.try_pop()) ++received;
            }
        });
        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * BATCH);
}
BENCHMARK(BM_Throughput)->Unit(benchmark::kMillisecond);

static void BM_Latency_RTT(benchmark::State& state) {
    SPSCQueue<int, 4096> q_req;
    SPSCQueue<int, 4096> q_resp;

    std::thread responder([&]() {
        while (true) {
            if (auto val = q_req.try_pop()) {
                if (*val == -1) break;
                while (!q_resp.try_push(*val));
            }
        }
    });

    for (auto _ : state) {
        while (!q_req.try_push(1));
        while (!q_resp.try_pop());
    }

    while (!q_req.try_push(-1));
    responder.join();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Latency_RTT)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
