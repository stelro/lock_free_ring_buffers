#include <benchmark/benchmark.h>
#include <cstddef>
#include <memory>
#include <thread>
#include <atomic>

#include "lock_free_spsc.hpp"   

// Simple 2-thread spin barrier for start sync
struct SpinBarrier {
    std::atomic<int> arrived{0};
    void arrive_and_wait(int total) {
        if (arrived.fetch_add(1, std::memory_order_acq_rel) + 1 == total) {
            // last thread resets to 0 to release the other
            arrived.store(0, std::memory_order_release);
            return;
        }
        // spin until the last thread resets the counter
        while (arrived.load(std::memory_order_acquire) != 0) { benchmark::DoNotOptimize(arrived); }
    }
};

static void BM_SPSC_Throughput(benchmark::State& state) {
    const std::size_t items     = static_cast<std::size_t>(state.range(0));
    const std::size_t capacity  = static_cast<std::size_t>(state.range(1));

    static std::unique_ptr<lock_free_spsc_queue<std::uint64_t>> queue;
    static SpinBarrier start_bar;

    if (state.thread_index() == 0) {
        queue = std::make_unique<lock_free_spsc_queue<std::uint64_t>>(capacity);
    }

    //state.counters["capacity"] = static_cast<double>(capacity);
    //state.counters["items/iter"] = static_cast<double>(items);

	// Per-thread constants (avoid being summed)
	state.counters["capacity"] =
		benchmark::Counter(static_cast<double>(capacity),
						   benchmark::Counter::kAvgThreads);

	state.counters["items/s"] =
		benchmark::Counter(static_cast<double>(items),
						   benchmark::Counter::kAvgThreads);

	// Total system throughput (producer->consumer transfers per second).
	// Report it once (thread 0). Do NOT use kAvgThreads here.
	if (state.thread_index() == 0) {
		state.counters["throughput_total"] =
			benchmark::Counter(static_cast<double>(items),
							   benchmark::Counter::kIsRate |
							   benchmark::Counter::kIsIterationInvariantRate);
	}

    for (auto _ : state) {
        state.PauseTiming();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        state.ResumeTiming();

        start_bar.arrive_and_wait(/*total=*/2);

        if (state.thread_index() == 0) {
            // Producer
            for (std::size_t i = 0; i < items; ++i) {
                std::uint64_t v = static_cast<std::uint64_t>(i);
                // Busy-wait until space is available
                while (!queue->try_push(v)) {
                    benchmark::DoNotOptimize(v);
                }
            }
        } else {
            // Consumer
            std::uint64_t out;
            for (std::size_t i = 0; i < items; ++i) {
                // Busy-wait until an item arrives
                while (!queue->try_pop(out)) {
                    benchmark::DoNotOptimize(out);
                }
                benchmark::DoNotOptimize(out);
            }
        }

        // Make sure both threads finish the iteration before moving on
        start_bar.arrive_and_wait(/*total=*/2);
    }

    if (state.thread_index() == 0) {
        state.SetItemsProcessed(state.items_processed() + static_cast<int64_t>(items * state.iterations()));
        state.counters["items_per_sec"] = benchmark::Counter(
            static_cast<double>(items) * state.iterations(),
            benchmark::Counter::kIsRate);
    }
}

BENCHMARK(BM_SPSC_Throughput)
    ->Args({1 << 20, 1024})      // 1M items, cap=1024
    ->Args({1 << 20, 4096})
    ->Args({1 << 20, 1 << 15})
    ->Iterations(5)
    ->UseRealTime()              
	//->Repetitions(5)->ReportAggregatesOnly(true)
    ->Threads(2);

BENCHMARK_MAIN();
