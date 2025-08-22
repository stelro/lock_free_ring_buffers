  #pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace stel {

template <typename T>
class thread_safe_queue {
public:

	thread_safe_queue() 
		: head_(new node()), tail_(head_.get()), stop_(false) 
	{
	}

	thread_safe_queue(const thread_safe_queue&) = delete;
	thread_safe_queue& operator =(const thread_safe_queue&) = delete;
	thread_safe_queue(thread_safe_queue&&) = default;
	thread_safe_queue& operator =(thread_safe_queue&&) = default;

	~thread_safe_queue() = default;

	void push(T value) {

		const std::shared_ptr data(std::make_shared<T>(std::move(value)));
		std::unique_ptr<node> dummy = std::make_unique<node>();
		node* new_tail = dummy.get();

		{
			std::lock_guard lock(tail_mutex_);
			tail_->data = std::move(data);
			tail_->next = std::move(dummy);
			tail_ = new_tail;
		}

		size_.fetch_add(1, std::memory_order_relaxed);
		cv_.notify_one();
	}

	std::shared_ptr<T> pop() {
		std::unique_ptr<node> old_head = try_pop_head_();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	bool pop(T& value) {
		std::unique_ptr<node> old_head = try_pop_head_(value);
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	// void wait_and_pop(T& result) {
	// 	std::unique_ptr<node> const old_head = wait_and_pop_head_(result);
	// }

	bool wait_and_pop(T& result) {

		std::unique_lock lock(head_mutex_);
		cv_.wait(lock, [&] { return head_.get() != get_tail_() || stop_; });
		if ((head_.get() == get_tail_()) && stop_) {
			return false;
		}

		result = std::move(*head_->data);
		head_ = std::move(head_->next);
		return true;
	}

	std::shared_ptr<T> wait_and_pop() {
		std::unique_ptr<node> const old_head = wait_and_pop_head_();
		return old_head->data;
	}
	
	std::size_t size() const {
		return size_.load(std::memory_order_relaxed);
	}

	std::shared_ptr<T> back() const {
		std::lock_guard lock(head_mutex_);
		return (head_.get() != get_tail_()) ? get_tail_()->data : std::shared_ptr<T>{};
	}

	std::shared_ptr<T> front() const {
		std::lock_guard lock(head_mutex_);
		return (head_.get() != get_tail_()) ? head_->data : std::shared_ptr<T>{};
	}

	bool empty() const {
		std::lock_guard lock(head_mutex_);
		return head_.get() == get_tail_();
	}

	// Sinal threads to stop waiting
	void shutdown() {
		{
			std::lock_guard lock(head_mutex_);
			stop_ = true;
		}
		cv_.notify_all();
	}

	bool done() const {
		return stop_.load(std::memory_order_relaxed);
	}

private:
	struct node {
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};

	std::unique_ptr<node> head_;
	node* tail_;

	mutable std::mutex head_mutex_;
	mutable std::mutex tail_mutex_;
	std::condition_variable cv_;

	std::atomic<int> size_;

	// Since wait_and_pop is blocking - we want to have a shutdown mechanism
	// in case this queue is used within a context like a thread pool
	std::atomic<bool> stop_;

	node* get_tail_() {
		std::lock_guard lock(tail_mutex_);
		return tail_;
	}

	const node* get_tail_() const {
		std::lock_guard lock(tail_mutex_);
		return tail_;
	}

	std::unique_ptr<node> try_pop_head_() {
		std::lock_guard<std::mutex> lock(head_mutex_);
		if (head_.get() == get_tail_()) {
			return nullptr;
		}
		return pop_head_();
	}

	std::unique_ptr<node> try_pop_head_(T& value) {
		std::lock_guard<std::mutex> lock(head_mutex_);
		if (head_.get() == get_tail_()) {
			return nullptr;
		}
		value = std::move(*head_->data());
		return pop_head_();
	}

	std::unique_ptr<node> pop_head_() {
		std::unique_ptr<node> old_head = std::move(head_);
		head_ = std::move(old_head->next);
		return old_head;
	}

	std::unique_lock<std::mutex> wait_for_data_() {
		std::unique_lock<std::mutex> lock(head_mutex_);
		cv_.wait(lock, [&]{ return head_.get() != get_tail_() || stop_; });
		return lock;
	}

	std::unique_ptr<node> wait_and_pop_head_() {
		std::unique_lock<std::mutex> thread_safe_queuelock(wait_for_data_());
		return pop_head_();
	}

	std::unique_ptr<node> wait_and_pop_head_(T& value) {
		std::unique_lock<std::mutex> lock(wait_for_data_());
		value = *head_->data;
		return pop_head_();
	}
};

} // namespace stel
