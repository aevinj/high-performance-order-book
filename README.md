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
- ✅ Unit test suite (GoogleTest) — 9 functional tests
- ✅ Basic performance benchmarking (~54K ops/sec @ 100K random ops, Release build)

---

## 🧪 Benchmark Results (Baseline)
**Environment:** macOS, Apple M2, Release build

| Operation | Throughput | Notes |
|-----------|------------|-------|
| Random 100K ops (add/cancel/modify/match) | ~53,769 ops/sec | Naive implementation using `std::map` + `std::list` + heap allocation |

---

## 🔬 Development Roadmap
This project is being developed iteratively. Each stage is a learning milestone and introduces key systems-programming techniques:

1. **Baseline Correctness (Current Stage)**
   - `std::map` for bids/asks
   - `std::list` for price level FIFO queues
   - Heap allocation for orders
   - Stress testing and profiling setup

2. **Optimizations (Planned)**
   - Replace dynamic allocation with a **memory pool** for deterministic performance
   - Replace `std::list` with **cache-friendly deque or ring buffer**
   - Replace `std::map` with a **flat hash map** or **price-indexed array**
   - Benchmark and document improvements after each optimization

3. **Advanced Extensions**
   - Backtesting integration
   - Persistent logging of trades
   - Comparison of C++ vs Rust implementations

---

## 📊 Profiling Insights (Baseline)
Profiling (via gperftools `pprof`) shows current hot spots:
- **65% of CPU time in libc++** (`std::map`, `std::list`, heap allocations)
- `cancel_order` and `modify_order` dominate due to pointer chasing in linked lists
- Confirms that memory pool + cache-friendly data structures are the right next steps

---

## 🛠️ How to Build & Run
### Build
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
```

### Run demo app
```bash
./build-release/demo_app
```
### Run tests
```bash
ctest --test-dir build-release --output-on-failure
```
### Run with profiler
```bash
CPUPROFILE=profile.out ./build-release/OrderBookTests --gtest_filter=LimitOrderBookStressTest.RandomizedOperationsWithTiming
pprof --pdf ./build-release/OrderBookTests profile.out > profile.pdf
```
### References
- Performance-Aware Programming (Fermilab, 2019)
- High-Frequency Trading Systems Design (various sources)
- Google Performance Tools (gperftools)

### Author's Note
This project is part of my exploration into systems-level software engineering for HFT.
Each milestone demonstrates mastery of both correctness-first engineering and performance-critical optimisation.