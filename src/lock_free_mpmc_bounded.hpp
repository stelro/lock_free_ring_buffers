#pragma once

#include <atomic>
#include <cmath>
#include <optional>
#include <type_traits>
#include <new>
#include <cassert>
#include <iostream>

template <typename T>
class mpmc_bounded_queue {
public:
	mpmc_bounded_queue(std::size_t cap) 
		: capacity_(cap) 
		, mask_(capacity_ - 1)
		, slots_(static_cast<Slot*>(::operator new[](capacity_ * sizeof(Slot))))  
		, head_(0)
		, tail_(0) 
	{
		assert((capacity_ & (capacity_ - 1)) == 0 && "Capacity must be power of 2");
		for (std::size_t i = 0; i < capacity_; ++i) {
			new(&slots_[i]) Slot(i);
		}
	}

	~mpmc_bounded_queue() { 
		// No other threads should be accessing the queue now.
		std::size_t h = head_.load(std::memory_order_relaxed);
		const std::size_t t = tail_.load(std::memory_order_relaxed);

		while (h != t) {
			Slot& s = slots_[h & mask_];
			// If full, it must be in the pos + 1 state for this slot
			if (s.seq.load(std::memory_order_acquire) == h + 1) {
				(void)s.remove();
				s.seq.store(h + capacity_, std::memory_order_release);
			}
			++h;
		}

		for (std::size_t i = 0; i < capacity_; ++i) {
			slots_[i].~Slot();
		}
		::operator delete[](slots_);
	}

	mpmc_bounded_queue(const mpmc_bounded_queue&) = delete;
	mpmc_bounded_queue& operator =(const mpmc_bounded_queue&) = delete;
	mpmc_bounded_queue(mpmc_bounded_queue&&) = delete;
	mpmc_bounded_queue& operator =(mpmc_bounded_queue&&) = delete;


	// Returns false if queue is full (non-blocking)
	bool try_enqueue(T value) {
		std::size_t pos = tail_.fetch_add(1, std::memory_order_acq_rel);
		Slot& s = slots_[pos & mask_];

		std::size_t seq = s.seq.load(std::memory_order_acquire);
		auto diff = static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos);
		// If diff < 0  -- slot is full
		// If diff > 0  -- another publisher hasn't finished publishing yet (rare)
		if (diff != 0) {
			// The queue is full
			return false;
		}

		s.put(std::move(value));
		s.seq.store(pos + 1, std::memory_order_release);
		return true;
	}

	bool try_dequeue(T& value) {
		std::size_t pos = head_.fetch_add(1, std::memory_order_acq_rel);
		Slot& s = slots_[pos & mask_];

		std::size_t seq = s.seq.load(std::memory_order_acquire);
		auto diff = static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos + 1);
		// if diff < 0  -- not published yet
		// if diff > 0  -- logically imposible (or indicates wrap we're not expecting)
		if (diff != 0) {
			// Slot not ready yet - empty
			return false;
		}

		value = std::move(s.remove());
		s.seq.store(pos + capacity_, std::memory_order_release);
		return true;
	}

	std::size_t capacity() const noexcept { return capacity_; }

	// get size on fast MPMC isn't free.
	// We can have 3 choices:
	// 1) Don't provide size at all (rely on empty()/full() instead)
	// 2) Approximate size (good enough for monitoring)
	// 3) Exact size (adding extra atomics - costs throughput)
	//
	// Size is tricky because in the per-slot-sequence MPMC:
	//	* tail is a -ticket counter- for claims by producers - not "published items"
	//	* head is a -ticket counter- for claims by consumers - not "completed pops"
	//
	// The approximation can be off up to roughly #active_producers + #active_consumers 
	// (each side can have at most one outstanding claim that isn't completed yet).
	std::size_t maybe_size() const {
		// Retry to reduce tearing
		for (;;) {
			auto h1 = head_.load(std::memory_order_relaxed);
			auto t = tail_.load(std::memory_order_relaxed);
			auto h2 = head_.load(std::memory_order_relaxed);
			if (h1 == h2) return t - h1;
			// else: someone advanced head while we sampled; try again
		}
	}
	
	// Racy by definitions:
	//	* Might return false, but another consumer claims it before we call dequeue
	//	* Might return true, but a producer publishes right after.
	bool empty_hint() const noexcept {
		const std::size_t h = head_.load(std::memory_order_relaxed);
		const std::size_t seq = slots_[h & mask_].seq.load(std::memory_order_acquire);
		return seq != (h + 1);
	}

private:
	struct Slot {
		std::atomic<std::size_t> seq;
		alignas(T) unsigned char storage[sizeof(T)];

		Slot(std::size_t s) : seq(s) { }
		void put(T&& value) { new(get_ptr()) T(std::move(value)); }
		void put(const T& value) { new(get_ptr()) T(value); }
		T remove() {
			T* ptr = get_ptr();
			T value = std::move(*ptr);
			ptr->~T();
			return value;
		}
		T* get_ptr() { return reinterpret_cast<T*>(storage); }
	};

	std::size_t capacity_;
	std::size_t mask_;

	Slot* slots_;

	static constexpr std::size_t alignment = 64;

	alignas(alignment) std::atomic<std::size_t> head_;
	char pad_1[alignment - sizeof(head_)];
	alignas(alignment) std::atomic<std::size_t> tail_;
	char pad_2[alignment - sizeof(tail_)];
};
