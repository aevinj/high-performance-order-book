# High-Performance Limit Order Book (LOB)

A C++ implementation of a Limit Order Book designed as a foundation for exploring
**ultra-low-latency trading system components**.  

This repository is part of a structured project plan to progressively refine a naive, correct implementation into an
**optimized, cache-friendly, high-performance engine**, similar in design goals to those used in High-Frequency Trading (HFT).

---

## 📌 Features (Current Implementation)
- ✅ Add, cancel, and modify limit orders
- ✅ Matching engine with:
  - Partial and full fills
  - Multi-level sweeps
  - Price–time priority
- ✅ Stress test framework with invariant checks
- ✅ Unit test suite (GoogleTest) — 10+ functional tests
- ✅ Profiling support (gperftools)
- ✅ Custom memory pool for order allocation (deterministic latency, no heap allocs in hot path)
- ✅ Fixed tick size (0.01) and bounded price range (90–110)
- ✅ Price-indexed vector of levels instead of std::map (removes red–black tree overhead)
- ✅ Active level tracking (std::set) for fast best-bid / best-ask access

---

## 🧪 Benchmark Results (Baseline)
**Environment:** macOS, Apple M2, Release build

| Stage | Operation | Throughput | Notes |
|-------|-----------|------------|-------|
|Baseline| Random 100K ops (add/cancel/modify/match) | ~53,769 ops/sec | std::map + std::list, heavy STL overhead |
|Memory Pool| Same Workload | ~50-55k ops/sec | Heap allocs removed, but container costs dominate |
|Price-Indexed Vector + Active Level Sets| Same Workload | ~563,683 ops/sec | Major redesign: contiguous levels, integer tick indexing, active bid/ask tracking |


This marks a 10x throughput improvement over the baseline by eliminating cache-unfriendly containers and scanning.

---

## 🔬 Development Roadmap
This project is being developed iteratively. Each stage is a learning milestone and introduces key systems-programming techniques:

1. **Baseline Correctness (Current Stage)**
   - `std::map` for bids/asks
   - `std::list` for price level FIFO queues
   - Heap allocation for orders
   - Stress testing and profiling setup

2. **Optimizations (Current Stage)**
   - Memory pool allocator
   - Price-indexed vector levels
   - Active level sets for O(1) best bid/ask lookup

3. **Advanced Extensions (Planned)**
   - Backtesting integration
   - Persistent logging of trades

---

## 📊 Profiling Insights (Baseline)
Profiling (via gperftools pprof) evolution:

- Baseline:
   - ~65% CPU time in libc++ (std::map, std::list, heap allocations)
   - Cancels dominated due to pointer chasing in linked lists

- After Memory Pool:
   - Allocator overhead gone, but STL containers still bottleneck

- After Vector Levels + Active Sets:
   - Runtime dominated by matching logic itself
   - Cache locality dramatically improved
   - Cancel/insert operations O(log N) on active sets (N = active price levels)

---

## 🛠️ How to Build & Run
### Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Run demo app
```bash
./build/OrderBookApp
```
### Run tests
```bash
ctest --test-dir build --output-on-failure
```
### Run with profiler
```bash
CPUPROFILE=profile.out ./build/OrderBookTests --gtest_filter=LimitOrderBookStressTest.RandomizedOperationsWithTiming
pprof --pdf ./build/OrderBookTests profile.out > profile.pdf
pprof --text ./build/OrderBookTests profile.out | head -40
```
### References
- Performance-Aware Programming (Fermilab, 2019)
- High-Frequency Trading Systems Design (various sources)
- Google Performance Tools (gperftools)

### Author's Note
This project is part of my exploration into systems-level software engineering for HFT.
Each milestone demonstrates mastery of correctness-first engineering and performance-critical optimisation.

The latest redesign (tick-based vector levels + active tracking) proves how much throughput can be unlocked by aligning data structures with hardware realities.

Future work will push towards O(1) cancels with ring buffers, flat hash maps for order lookups, and further cache-friendly design.