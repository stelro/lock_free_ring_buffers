#include <benchmark/benchmark.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <latch>

#include "bounded_mpmc_pool.hpp"
#include "thread_pool.hpp"

using BoundedPool = stel::bounded_mpmc_pool;
using MutexPool = stel::thread_pool;

// ----------------------------
// Tiny busy-work for each task
// ----------------------------
static inline void do_work_ns(uint64_t ns) {
    if (ns == 0) return;
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    while (std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start).count() < static_cast<int64_t>(ns)) {
        // prevent aggressive optimization
        benchmark::DoNotOptimize(ns);
    }
}

// --------------------------------------
// Benchmark 1: Bounded MPMC pool submit
// Args:
//   0 -> workers
//   1 -> capacity (queue size)
//   2 -> tasks per iteration
//   3 -> per-task work (ns)
// --------------------------------------
static void BM_BoundedPool_Submit(benchmark::State& state) {
    const std::size_t workers   = static_cast<std::size_t>(state.range(0));
    const std::size_t capacity  = static_cast<std::size_t>(state.range(1));
    const std::size_t tasks     = static_cast<std::size_t>(state.range(2));
    const uint64_t    work_ns   = static_cast<uint64_t>(state.range(3));

    // One pool per benchmark config
    static std::unique_ptr<BoundedPool> pool;
    if (state.thread_index() == 0) {
        pool = std::make_unique<BoundedPool>(workers, capacity);
    }

    // Ensure all threads see 'pool'
    std::atomic_thread_fence(std::memory_order_seq_cst);

    state.counters["workers"]   = benchmark::Counter(double(workers),  benchmark::Counter::kAvgThreads);
    state.counters["capacity"]  = benchmark::Counter(double(capacity), benchmark::Counter::kAvgThreads);
    state.counters["tasks"]     = benchmark::Counter(double(tasks),    benchmark::Counter::kAvgThreads);
    state.counters["work_ns"]   = benchmark::Counter(double(work_ns),  benchmark::Counter::kAvgThreads);

    for (auto _ : state) {
        state.PauseTiming();
        // We’ll submit from the benchmark thread only (single producer)
        std::latch done(tasks);
        state.ResumeTiming();

        for (std::size_t i = 0; i < tasks; ++i) {
            bool ok = pool->submit([&done, work_ns]{
                do_work_ns(work_ns);
                done.count_down();
            });
            if (!ok) {
                pool->submit([&done, work_ns]{ do_work_ns(work_ns); done.count_down(); });
            }
        }
        done.wait();

        state.SetItemsProcessed(state.items_processed() + static_cast<int64_t>(tasks));
    }

    if (state.thread_index() == 0) {
        pool->shutdown();
        pool.reset();
    }
}
BENCHMARK(BM_BoundedPool_Submit)
    ->Args({16, 256, 1<<20, 0})          // 16 workers, cap 256, 1M no-op tasks
    ->Args({16, 256, 1<<20, 500})        // add 500ns of work per task
    ->UseRealTime()
    ->Iterations(3)
    ->Threads(1);

// -------------------------------------------------
// Benchmark 2: Mutex+CV pool submit (same workload)
// Args:
//   0 -> workers
//   1 -> tasks per iteration
//   2 -> per-task work (ns)
// -------------------------------------------------
static void BM_MutexPool_Submit(benchmark::State& state) {
    const std::size_t workers   = static_cast<std::size_t>(state.range(0));
    const std::size_t tasks     = static_cast<std::size_t>(state.range(1));
    const uint64_t    work_ns   = static_cast<uint64_t>(state.range(2));

    static std::unique_ptr<MutexPool> pool;
    if (state.thread_index() == 0) {
        pool = std::make_unique<MutexPool>(workers);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    state.counters["workers"] = benchmark::Counter(double(workers), benchmark::Counter::kAvgThreads);
    state.counters["tasks"]   = benchmark::Counter(double(tasks),   benchmark::Counter::kAvgThreads);
    state.counters["work_ns"] = benchmark::Counter(double(work_ns), benchmark::Counter::kAvgThreads);

    for (auto _ : state) {
        state.PauseTiming();
        std::latch done(tasks);
        state.ResumeTiming();

        for (std::size_t i = 0; i < tasks; ++i) {
            pool->submit([&done, work_ns]{
                do_work_ns(work_ns);
                done.count_down();
            });
        }
        done.wait();

        state.SetItemsProcessed(state.items_processed() + static_cast<int64_t>(tasks));
    }

    if (state.thread_index() == 0) {
        pool->shutdown();
        pool.reset();
    }
}

BENCHMARK(BM_MutexPool_Submit)
    ->Args({16, 1<<20, 0})
    ->Args({16, 1<<20, 500})
    ->UseRealTime()
    ->Iterations(3)
    ->Threads(1);

BENCHMARK_MAIN();
