#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <semaphore>
#include <limits>

#include "lock_free_mpmc_bounded.hpp"

namespace stel {

// This is not typical thread pool, it's a specialized pool for high-throughput scenarios.
class bounded_mpmc_pool {
public:
	bounded_mpmc_pool(std::size_t workers, 
				std::size_t queue_capacity) 
		: q_(queue_capacity), stop_(false), sem_(0) 
	{ 
		workers_.reserve(workers);
		for (std::size_t i = 0; i < workers; ++i) {
			workers_.emplace_back([this] { worker_loop(); });
		}
	}

	~bounded_mpmc_pool() {
		shutdown();
	}

	// "caller runs" strategy. 
	// However, it can lead to unbounded stack growth if a 
	// submitted task also tries to submit to a full queue.
	template <typename F>
	bool submit(F&& f) {
		Task t(std::forward<F>(f));

		// Fast path: try to enqueue
		if (q_.try_enqueue(Task(std::move(t)))) {
			sem_.release(); // Signal "work available"
			return true;
		}

		// Queue full policy - both are bad, second is worse
		Task run_now(std::move(t));
		run_now();
		return true;

		// Other option, block/spin until space is available (can deadlock without care)
		// while (!q_.try_enqueue(t)) {
		// 	std::this_thread::yield();
		// }
		// sem_.release();
		// return true;
	}

	void shutdown() {
		bool expected = false;
		if (!stop_.compare_exchange_strong(expected, true, 
					std::memory_order_acq_rel)) {
			return; // Already stopped
		}

		// Wake all the workers so they can observe stop_ and exit.
		for (std::size_t i = 0; i < workers_.size(); ++i) {
			sem_.release();
		}

		for (auto& worker : workers_) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

private:
	using Task = std::function<void()>;

	void worker_loop() {
		for (;;) {
			sem_.acquire(); // sleep until someone releases work or shutdown
			if (stop_.load(std::memory_order_acquire)) break;

			Task task;
			// Drain one task, if token was released, there should be one task
			while (!q_.try_dequeue(task)) {
				// Rare races: another worker might have grabbed it.
				// Backoff a tiny bit then retry or bail if stopping.
				if (stop_.load(std::memory_order_relaxed)) return;
				std::this_thread::yield();
			}
			task();
		}
	}
	
	mpmc_bounded_queue<Task> q_;
	std::atomic<bool> stop_;
	std::counting_semaphore<std::numeric_limits<int>::max()> sem_;
	std::vector<std::thread> workers_;
};

}
