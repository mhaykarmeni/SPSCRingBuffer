#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "spsc_queue.h"

TEST(SPSCQueueTest, PushPop_SingleElement) {
    SPSCQueue<int, 8> q;
    ASSERT_TRUE(q.try_push(42));
    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
}

TEST(SPSCQueueTest, TryPop_WhenEmpty_ReturnsNullopt) {
    SPSCQueue<int, 8> q;
    EXPECT_FALSE(q.try_pop().has_value());
}

TEST(SPSCQueueTest, TryPush_WhenFull_ReturnsFalse) {
    SPSCQueue<int, 8> q;
    for (int i = 0; i < 8; ++i)
        ASSERT_TRUE(q.try_push(i));
    EXPECT_FALSE(q.try_push(99));
}

TEST(SPSCQueueTest, EmptyAndFull) {
    SPSCQueue<int, 8> q;
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());

    for (int i = 0; i < 8; ++i)
        q.try_push(i);

    EXPECT_FALSE(q.empty());
    EXPECT_TRUE(q.full());
}

TEST(SPSCQueueTest, FIFO_Order) {
    SPSCQueue<int, 8> q;
    for (int i = 0; i < 8; ++i)
        q.try_push(i);
    for (int i = 0; i < 8; ++i) {
        auto val = q.try_pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, i);
    }
}

TEST(SPSCQueueTest, PushPop_FullCapacity) {
    SPSCQueue<int, 16> q;
    for (int i = 0; i < 16; ++i)
        ASSERT_TRUE(q.try_push(i));
    for (int i = 0; i < 16; ++i) {
        auto val = q.try_pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, i);
    }
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueueTest, WrapAround) {
    SPSCQueue<int, 8> q;
    // push/pop more than N elements total to force index wrap-around
    for (int round = 0; round < 4; ++round) {
        for (int i = 0; i < 8; ++i)
            ASSERT_TRUE(q.try_push(round * 8 + i));
        for (int i = 0; i < 8; ++i) {
            auto val = q.try_pop();
            ASSERT_TRUE(val.has_value());
            EXPECT_EQ(*val, round * 8 + i);
        }
    }
}

TEST(SPSCQueueTest, Size) {
    SPSCQueue<int, 8> q;
    EXPECT_EQ(q.size(), 0u);
    q.try_push(1);
    q.try_push(2);
    EXPECT_EQ(q.size(), 2u);
    q.try_pop();
    EXPECT_EQ(q.size(), 1u);
}

TEST(SPSCQueueTest, Concurrent_StressTest) {
    constexpr int N = 1 << 16;
    SPSCQueue<int, 1024> q;
    std::vector<int> results;
    results.reserve(N);

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i)
            while (!q.try_push(i));
    });

    std::thread consumer([&]() {
        int received = 0;
        while (received < N) {
            if (auto val = q.try_pop()) {
                results.push_back(*val);
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(static_cast<int>(results.size()), N);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(results[i], i);
}
