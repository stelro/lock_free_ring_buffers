#pragma once 

#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>

template <typename T, std::size_t Size>
class spsc_queue {
public:
	spsc_queue()
		: head_(0), tail_(0), cap_(Size), size_(0) { }

	bool try_push(T value) {
		if (size_ >= cap_) return false;
		const auto next = next_(tail_);
		buffer[tail_] = std::move(value);
		tail_ = next;
		size_++;
		return true;
	}	

	std::optional<T> front() const {
		if (empty()) {
			return std::nullopt;
		}
		return buffer[head_];
	}

	bool empty() const { return size_ == 0; }
	std::size_t capacity() const { return Size; }
	std::size_t size() const { return size_; }

	bool try_pop() {
		if (empty()) return false;
		const auto next = next_(head_);
		size_--;
		head_ = next;
		return true;
	}

private:
	std::size_t next_(std::size_t i) const { return (i + 1) % Size; }

	T buffer[Size];

	std::size_t head_; // read
	std::size_t tail_; // write
	std::size_t cap_ ;
	// You can really avoid the extra size_ variable by applying the N-1 policy 
	// e.g. head == tail is empty head == next tail is full
	std::size_t size_;
};

