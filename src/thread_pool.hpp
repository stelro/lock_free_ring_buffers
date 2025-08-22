#pragma once 

#include <vector>
#include <thread>
#include <functional>

#include "lock_free_mpmc_bounded.hpp"
#include "thread_safe_queue.hpp"

namespace stel {

class thread_pool {
public:
	using Task = std::function<void()>;

	thread_pool(std::size_t workers) {
		workers_.reserve(workers);
		for (std::size_t i = 0; i < workers; ++i) {
			workers_.emplace_back(std::thread([&]() {
				this->run();
			}));
		}
	}

	thread_pool(const thread_pool&) = delete;
	thread_pool& operator =(const thread_pool&) = delete;
	thread_pool(thread_pool&&) = delete;
	thread_pool& operator =(thread_pool&&) = delete;

	~thread_pool() {
		task_.shutdown();
		for (auto& th : workers_) {
			if (th.joinable()) {
				th.join();
			}
		}
	}

	template <typename F>
	void submit(F&& f) {
		task_.push(std::forward<F>(f));
	}

	void shutdown() {
		task_.shutdown();
	}

private:
	void run() {
		while (true) {
			Task t;
			if (!task_.wait_and_pop(t)) {
				break;
			}
			t();
		}
	}
	std::vector<std::thread> workers_;
	thread_safe_queue<Task> task_;
};
} // namespace stel
