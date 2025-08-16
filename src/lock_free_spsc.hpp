#pragma once 

#include <atomic>
#include <optional>
#include <type_traits>
#include <new>

#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    constexpr std::size_t hardware_constructive_interference_size = 64;
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

// Lock free Single Producer - Single Consumer ring buffer
// with N - 1 Policy. 
// In other words, the Capacity of the Queue is N - 1 
// That is to simplify some operations and avoid the overhead of holding 
// an extra atomic to keep track of the size
template <typename T, std::size_t Size>
class lock_free_spsc_queue {
public:

	// Ensure at least T is move constructible
	static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
	static_assert(std::is_move_assignable_v<T>, "T must be move constructible");

	lock_free_spsc_queue()
		: head_(0), tail_(0) { }

	~lock_free_spsc_queue() {
		if (!std::is_trivially_destructible_v<T>) {
			while (try_pop()) ;
		}
	}

	bool try_push(T value) noexcept (std::is_nothrow_move_assignable_v<T>) {

		// An acquire load is meant to synchronize with a release from another thread
		// Since no other thread writes to tail_, this is relaxed
		const auto tail = tail_.load(std::memory_order_relaxed);
		const auto next = next_(tail);

		if (next == head_.load(std::memory_order_acquire)) {
			return false; // queue is full
		}
			
		new (&buffer_[tail]) T(std::move(value));
		
		tail_.store(next, std::memory_order_release);
		return true;
	}	

	std::optional<T> try_pop() noexcept (std::is_nothrow_move_constructible_v<T>) {

		// An acquire load is meant to synchronize with a release from another thread
		// Since no other thread writes to head_, this is relaxed
		const auto head = head_.load(std::memory_order_relaxed);

		if (head == tail_.load(std::memory_order_acquire)) {
			return std::nullopt; // queue is empty
		}

		T value(std::move(reinterpret_cast<T&>(buffer_[head])));
		reinterpret_cast<T&>(buffer_[head]).~T();
		
		head_.store(next_(head), std::memory_order_release);
		return value;
	}

	bool empty() const noexcept {  
		return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
	}

	std::size_t capacity() const noexcept { return Size - 1; }
	// maybe_size - Getting a consistent head and tail for a size is not trivial
	// as the size can be stale at the moment is returned.
	std::size_t maybe_size() const { 
		const auto head = head_.load(std::memory_order_relaxed);
		const auto tail = tail_.load(std::memory_order_relaxed);
		return (tail - head + Size) % Size; 
	}

private:
	std::size_t next_(std::size_t i) const noexcept { return (i + 1) % Size; }
	
	// Avoid default constructing T objects.
	// This buffer holds raw, uninitialized memory.
	T buffer_[Size];

	alignas(hardware_destructive_interference_size) std::atomic<std::size_t> head_; // read
	char pad_[hardware_destructive_interference_size - sizeof(head_)]; // padding to avoid false-sharing
	alignas(hardware_destructive_interference_size) std::atomic<std::size_t> tail_; // write
	
};

