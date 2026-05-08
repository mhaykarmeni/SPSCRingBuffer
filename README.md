# SPSC Ring Buffer

A lock-free Single-Producer Single-Consumer (SPSC) ring buffer implemented in modern C++20.

---

## Goal

Demonstrate production-quality lock-free programming skills relevant to HFT systems:
correct atomic memory ordering, cache-friendly layout, and measurable low-latency throughput.

---

## What to Implement

### Core class: `SPSCQueue<T, N>`

A fixed-capacity, lock-free queue for exactly one producer thread and one consumer thread.

```
template<typename T, std::size_t N>
class SPSCQueue { ... };
```

**Template parameters:**
- `T` — element type (should be trivially copyable for best performance)
- `N` — capacity; must be a power of 2 (enforce with `static_assert`)

---

## Required Functionality

### 1. `bool try_push(const T& value)`
- Attempts to enqueue one element.
- Returns `true` on success, `false` if the queue is full.
- Must be called only from the **producer** thread.

### 2. `bool try_push(T&& value)`
- Move-overload of `try_push`.

### 3. `std::optional<T> try_pop()`
- Attempts to dequeue one element.
- Returns the element wrapped in `std::optional`, or `std::nullopt` if empty.
- Must be called only from the **consumer** thread.

### 4. `bool empty() const`
- Returns whether the queue is empty.
- Safe to call from either thread (approximate snapshot).

### 5. `bool full() const`
- Returns whether the queue is full.
- Safe to call from either thread (approximate snapshot).

### 6. `std::size_t size() const`
- Returns an approximate element count (may be stale by the time you read it).

---

## Design features

### Memory ordering
- Used `std::memory_order_acquire` and `std::memory_order_release` on head/tail loads and stores.
- `std::memory_order_seq_cst` is avoided.
- Head index is written only by the consumer; tail index is written only by the producer.

### Cache line isolation
- Placed `head` and `tail` on **separate cache lines** to eliminate false sharing.
- Used `alignas(64)` taking into account that cache line size is 64.

### Index wrapping
- Used bitmask wrapping (`index & (N - 1)`) instead of modulo — requires N to be power of 2.

### No dynamic allocation
- The internal buffer is plain array: `T buffer_[N]`.
- No `std::vector`, no heap allocation inside the queue.

### Non-copyable, non-movable
- Delete copy and move constructors/assignment operators.

---

## File Structure

```
SPSCRingBuffer/
├── src/
│   └── spsc_queue.hpp       # header-only implementation
├── tests/
│   └── spsc_queue_test.cpp  # Google Test suite
├── bench/
│   └── spsc_queue_bench.cpp # Google Benchmark suite
├── CMakeLists.txt
└── README.md
```

---

## Tests to Write (`tests/spsc_queue_test.cpp`)

| Test | What it verifies |
|------|-----------------|
| `PushPop_SingleElement` | push one, pop one, correct value |
| `PushPop_FullCapacity` | fill to capacity, drain, all values correct and in order |
| `TryPush_WhenFull_ReturnsFalse` | push N elements, next push returns false |
| `TryPop_WhenEmpty_ReturnsNullopt` | pop on empty queue returns `std::nullopt` |
| `FIFO_Order` | elements come out in the same order they went in |
| `WrapAround` | push/pop more than N elements total, verify wrap-around correctness |
| `Concurrent_StressTest` | producer thread pushes M items, consumer pops M items, verify no loss and correct order |
| `Concurrent_NoDataRace` | run under TSan; must pass with zero reported races |

---

## Benchmarks to Write (`bench/spsc_queue_bench.cpp`)

| Benchmark | What it measures |
|-----------|-----------------|
| `BM_Throughput` | producer and consumer on separate threads, messages/sec sustained |
| `BM_Latency_RTT` | round-trip time: producer pushes, consumer pops and pushes back, measure ns/op |
| `BM_PushOnly` | single-thread push throughput (upper bound, no contention) |
| `BM_PopOnly` | single-thread pop throughput after pre-filling |

Report results in the README after running on your machine.

---

## Build Setup

Use CMake with:
- C++20 standard
- Google Test (via FetchContent or system package)
- Google Benchmark (via FetchContent or system package)
- A `sanitize` build type that enables `-fsanitize=thread,undefined`

---

## Building & Testing

### Standard build
```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

### TSan build (data race detection)

> **WSL2 users:** run `sudo sysctl vm.mmap_rnd_bits=28` once before the TSan build.

```bash
cmake -S . -B build-tsan \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build-tsan
cd build-tsan && ctest --output-on-failure
```

Expected: 9/9 tests pass, zero data race reports.

---

## Acceptance Criteria

- [ ] All tests pass under TSan with zero data race reports
- [ ] Throughput benchmark shows > 100M ops/sec on modern hardware (realistic for SPSC)
- [ ] `try_push` / `try_pop` contain zero heap allocations (verify with `perf` or a custom allocator hook)
- [ ] Code compiles clean under `-Wall -Wextra -Wpedantic` with no warnings
