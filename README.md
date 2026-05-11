# SPSC Ring Buffer

Two SPSC (Single-Producer Single-Consumer) ring buffer implementations in modern C++20 — one lock-free, one mutex-based — with benchmarks comparing their performance.

---

## Goal

Demonstrate production-quality concurrent queue design relevant to HFT systems: correct atomic memory ordering, cache-friendly layout, measurable low-latency throughput, and a side-by-side comparison between lock-free and mutex-based approaches.

---

## Implementations

### `SPSCQueueLF<T, N>` — Lock-Free

A fixed-capacity, lock-free queue using atomic head/tail indices with acquire/release ordering. Zero contention between producer and consumer in the common case.

```cpp
template<typename T, std::size_t N>
class SPSCQueueLF { ... };
```

### `SPSCQueueMtx<T, N>` — Mutex-Based

A fixed-capacity queue using `std::mutex` for synchronization. Simpler invariants, correct under TSan, but pays a mutex acquire/release (~20 ns) on every operation.

```cpp
template<typename T, std::size_t N>
class SPSCQueueMtx { ... };
```

**Template parameters (both classes):**
- `T` — element type (should be trivially copyable for best performance)
- `N` — capacity; must be a power of 2 (enforced with `static_assert`)

---

## API (both classes)

### 1. `bool try_push(const T& value)` / `bool try_push(T&& value)`
- Enqueues one element. Returns `true` on success, `false` if full.
- Must be called only from the **producer** thread.

### 2. `std::optional<T> try_pop()`
- Dequeues one element. Returns the value or `std::nullopt` if empty.
- Must be called only from the **consumer** thread.

### 3. `bool empty() const`
- Returns whether the queue is empty (approximate snapshot).

### 4. `bool full() const`
- Returns whether the queue is full (approximate snapshot).

### 5. `std::size_t size() const`
- Returns an approximate element count.

---

## Design Features

### `SPSCQueueLF`

- **Memory ordering** — `acquire`/`release` on head/tail loads and stores; `seq_cst` avoided entirely.
- **Ownership** — head written only by consumer, tail written only by producer; no shared writes.
- **Cache line isolation** — `head`, `tail`, and `buffer` each on their own `alignas(64)` cache line to eliminate false sharing.
- **Index wrapping** — bitmask `index & (N - 1)` instead of modulo; requires N to be a power of 2.
- **No heap allocation** — internal buffer is a plain `std::array<T, N>`.

### `SPSCQueueMtx`

- **Synchronization** — single `std::mutex` serializes all access; mutex is `mutable` to allow locking in `const` methods.
- **Plain indices** — `std::size_t` head/tail (no atomics needed; mutex provides ordering).
- **Cache line isolation** — `buffer` and `mutex` on separate cache lines; no per-index padding needed since only one thread runs the critical section at a time.
- **No heap allocation** — internal buffer is a plain `std::array<T, N>`.

---

## File Structure

```
SPSCRingBuffer/
├── src/
│   └── spsc_queue.h         # header-only implementation (both classes)
├── tests/
│   └── spsc_queue_test.cpp  # Google Test suite (18 tests, both classes)
├── bench/
│   └── spsc_queue_bench.cpp # Google Benchmark suite (both classes)
├── CMakeLists.txt
└── README.md
```

---

## Tests (`tests/spsc_queue_test.cpp`)

The same 9 test cases run for both `SPSCQueueLFTest` and `SPSCQueueMtxTest` (18 total):

| Test | What it verifies |
|------|-----------------|
| `PushPop_SingleElement` | push one, pop one, correct value |
| `PushPop_FullCapacity` | fill to capacity, drain, all values correct and in order |
| `TryPush_WhenFull_ReturnsFalse` | push N elements, next push returns false |
| `TryPop_WhenEmpty_ReturnsNullopt` | pop on empty queue returns `std::nullopt` |
| `FIFO_Order` | elements come out in the same order they went in |
| `WrapAround` | push/pop more than N elements total, verify wrap-around correctness |
| `Size` | size() reflects pushes and pops correctly |
| `EmptyAndFull` | empty() and full() return correct states |
| `Concurrent_StressTest` | producer thread pushes 65536 items, consumer pops all, verify no loss and correct order |

---

## Benchmarks (`bench/spsc_queue_bench.cpp`)

| Benchmark | What it measures |
|-----------|-----------------|
| `BM_PushOnly` / `BM_PushOnly_Mtx` | single-thread push throughput (upper bound, no contention) |
| `BM_PopOnly` / `BM_PopOnly_Mtx` | single-thread pop throughput after pre-filling |
| `BM_Throughput` / `BM_Throughput_Mtx` | producer and consumer on separate threads, messages/sec sustained |
| `BM_Latency_RTT` / `BM_Latency_RTT_Mtx` | round-trip time: producer pushes, consumer pops and pushes back, measure ns/op |

### Results (Intel Core Ultra 7 155U, 32GB RAM, 1.7GHz, Ubuntu 24.04 WSL2, GCC 14.2, Release build)

| Benchmark | `SPSCQueueLF` | `SPSCQueueMtx` | Ratio |
|-----------|--------------|----------------|-------|
| `BM_PushOnly` | 0.55 ns / 1.66 Gops/s | 18.7 ns / 49 Mops/s | **34x slower** |
| `BM_PopOnly` | 0.45 ns / 2.05 Gops/s | 18.6 ns / 49 Mops/s | **41x slower** |
| `BM_Throughput` | 855 Mops/s | 608 Mops/s | 1.4x (WSL2 artifact) |
| `BM_Latency_RTT` | 210 ns | 1162 ns | **5.5x slower** |

### Results (AMD Ryzen 7 7730U, 8GB RAM, 2.0GHz, Ubuntu 24.04 WSL2, GCC 14.1, Release build)

| Benchmark | `SPSCQueueLF` | `SPSCQueueMtx` | Ratio |
|-----------|--------------|----------------|-------|
| `BM_PushOnly` | 1.56 ns / 658 Mops/s | 21.7 ns / 47 Mops/s | **14x slower** |
| `BM_PopOnly` | 1.35 ns / 765 Mops/s | 21.5 ns / 48 Mops/s | **16x slower** |
| `BM_Throughput` | 251 Mops/s | 313 Mops/s | ~1.2x (WSL2 artifact) |
| `BM_Latency_RTT` | 308 ns | 2715 ns | **9x slower** |

> Measured on WSL2 — bare metal Linux will show lower latency. The `BM_Throughput` inversion (mutex slightly ahead) is a WSL2 scheduling artifact; on bare metal lock-free wins across all benchmarks.

---

## Build Setup

Uses CMake with:
- C++20 standard
- Google Test (via FetchContent)
- Google Benchmark (via FetchContent)
- A `sanitize` build type that enables `-fsanitize=thread,undefined`

---

## Building & Testing

### Standard build
```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

### Release build (for accurate benchmarks)
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/spsc_bench
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

Expected: 18/18 tests pass, zero data race reports.

---

## Acceptance Criteria

- [x] All 18 tests pass under TSan with zero data race reports
- [x] Lock-free throughput > 100M ops/sec on modern hardware
- [x] `try_push` / `try_pop` contain zero heap allocations
- [x] Code compiles clean under `-Wall -Wextra -Wpedantic` with no warnings
