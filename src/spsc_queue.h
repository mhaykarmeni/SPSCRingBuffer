#pragma once
#include <mutex>
#include <atomic>
#include <array>
#include <optional>

template<typename T, std::size_t N>
class SPSCQueueLF {
    static_assert(N >= 2 && (N & (N - 1)) == 0, "N must be a power of 2");
public:
    SPSCQueueLF() = default;
    SPSCQueueLF(const SPSCQueueLF&) = delete;
    SPSCQueueLF& operator=(const SPSCQueueLF&) = delete;
    SPSCQueueLF(SPSCQueueLF&&) = delete;
    SPSCQueueLF& operator=(SPSCQueueLF&&) = delete;

    template<typename U>
    bool try_push(U&& val) {
      auto tail = m_tail.load(std::memory_order_relaxed);
      auto head = m_head.load(std::memory_order_acquire);
      if (tail - head == N) 
        return false;
      m_buffer[tail & (N - 1)] = std::forward<U>(val);
      m_tail.store(tail + 1, std::memory_order_release);
      return true;
    }    

    std::optional<T> try_pop() {
        auto head = m_head.load(std::memory_order_relaxed);
        auto tail = m_tail.load(std::memory_order_acquire);
        if (head == tail) 
            return std::nullopt;
        T val = std::move(m_buffer[head & (N - 1)]);
        m_head.store(head + 1, std::memory_order_release);
        return val;
    }

    bool empty() const noexcept {
        return m_tail.load(std::memory_order_relaxed) == m_head.load(std::memory_order_relaxed);
    }

    bool full() const noexcept {
        return m_tail.load(std::memory_order_relaxed) - m_head.load(std::memory_order_relaxed) == N;
    }

    std::size_t size() const noexcept {
        return m_tail.load(std::memory_order_relaxed) - m_head.load(std::memory_order_relaxed);
    }

private:
    alignas(64) std::array<T, N> m_buffer;
    alignas(64) std::atomic<std::size_t> m_head = 0;
    alignas(64) std::atomic<std::size_t> m_tail = 0;
};


template<typename T, std::size_t N>
class SPSCQueueMtx {
    static_assert(N >= 2 && (N & (N - 1)) == 0, "N must be a power of 2");
public:
    SPSCQueueMtx() = default;
    SPSCQueueMtx(const SPSCQueueMtx&) = delete;
    SPSCQueueMtx& operator=(const SPSCQueueMtx&) = delete;
    SPSCQueueMtx(SPSCQueueMtx&&) = delete;
    SPSCQueueMtx& operator=(SPSCQueueMtx&&) = delete;

    template<typename U>
    bool try_push(U&& val) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_tail - m_head == N) 
            return false;
        m_buffer[m_tail & (N - 1)] = std::forward<U>(val);
        ++m_tail;
        return true;
    }    

    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_head == m_tail) 
            return std::nullopt;
        T val = std::move(m_buffer[m_head & (N - 1)]);
        ++m_head;
        return val;
    }

    bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tail == m_head;
    }

    bool full() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tail - m_head == N;
    }

    std::size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tail - m_head;
    }

private:
    alignas(64) std::array<T, N> m_buffer;
    std::size_t m_head = 0;
    std::size_t m_tail = 0;
    alignas(64) mutable std::mutex m_mutex;
};