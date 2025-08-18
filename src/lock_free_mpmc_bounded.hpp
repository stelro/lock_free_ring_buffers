#pragma once

#include <atomic>
#include <optional>
#include <type_traits>
#include <new>
#include <cassert>

#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    constexpr std::size_t hardware_constructive_interference_size = 64;
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

// This is a classic (Vyukov-style) bounded array MPMC queue.
template <typename T>
class mpmc_bounded_queue {
public:

	explicit mpmc_bounded_queue(std::size_t capacity) 
		: cap_(round_up_pow2(capacity))
		, mask_(cap_ - 1)
		, slots_(static_cast<slot*>(::operator new[](cap_ * sizeof(slot))))
		, head_(0)
		, tail_(0) {

		for (std::size_t i = 0; i < cap_; ++i) {
			new (&slots_[i]) slot(i);
		}
	}

	~mpmc_bounded_queue() {
		// Destroy any constructed T is non-trivial; We assume the queue
		// is drained by the user before destruction. Then destroy metadata.
		// 
		// TODO:
		// No concurrent producers/consumers are running while the destructor executes (or we’ll have data races).
		// Any remaining constructed T objects in slots are destroyed, otherwise we leak resources.
		for (std::size_t i = 0; i < cap_; ++i) {
			slots_[i].~slot();
		}
		::operator delete[](slots_);
	}
	
	// Return false if the queue is full at the moment (non-blocking try)
	bool try_enqueue(T x) {
		std::size_t pos = tail_.fetch_add(1, std::memory_order_acq_rel);
		slot& s = slots_[pos & mask_];

		// Wait until this slot's seq equals the expected "ticket" (pos)
		std::size_t seq = s.seq.load(std::memory_order_acquire);
		std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos);
		if (diff != 0) {
			// Not ready; we didn't actually take the slot - singal failure.
			// (we could also spin instead of failing fast)
			return false;
		}

		s.storage_put(std::move(x));
		s.seq.store(pos + 1, std::memory_order_release);
		return true;
	}

	// Return false if the queue is empty at the moment (non-blocking try)
	bool try_dequeue(T& out) {
		std::size_t pos = head_.fetch_add(1, std::memory_order_acq_rel);
		slot& s = slots_[pos & mask_];

		// Wait until this slot is full: seq == pos + 1
		std::size_t seq = s.seq.load(std::memory_order_acquire);
		std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos + 1);
		if (diff != 0) {
			// Not ready; we didn't actually read anything - signal failure. 
			// (we could also spin instead of failing fast.)
			return false;
		}

		out = s.storage_take();
		// Mark slot free for the next wrap: seq = pos + cap_
		s.seq.store(pos + cap_, std::memory_order_release);
		return true;
	}

	std::size_t capacity() const noexcept { return cap_; }
private:
	struct slot {
		std::atomic<std::size_t> seq;
		typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
		explicit slot(std::size_t initial_seq) : seq(initial_seq) { }

		void storage_put(T&& v) { new (&storage) T(std::move(v)); }
		void storage_put(const T& v) { new (&storage) T(v); }

		T storage_take() {
			T* p = reinterpret_cast<T*>(&storage);
			T v = std::move(*p);
			p->~T();
			return v;
		}
	};

	static std::size_t round_up_pow2(std::size_t n) {
		if (n < 2) return 2;
		// Round up the next power of 2
		--n;
		for (std::size_t i = 1; i < sizeof(std::size_t) * 8; i <<= 1) {
			n |= n >> i;
		}
		return n + 1;
	}

	const std::size_t cap_;
	const std::size_t mask_;
	slot* const slots_;

	alignas(hardware_destructive_interference_size) std::atomic<std::size_t> head_; // read
	char pad_[hardware_destructive_interference_size - sizeof(head_)]; // padding to avoid false-sharing
	alignas(hardware_destructive_interference_size) std::atomic<std::size_t> tail_; // write
	char pad_2[hardware_destructive_interference_size - sizeof(tail_)]; // padding to avoid false-sharing
};
