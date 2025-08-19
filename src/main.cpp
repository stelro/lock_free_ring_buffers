#include <iostream>
#include <stdexcept>

#include "lock_free_mpmc_bounded.hpp"

int main() {

	mpmc_bounded_queue<int> q(4);

	for (int i = 0; i < 6; i++) {
		if (q.try_enqueue(i)) {
			std::cout << "enqueue: " << i << std::endl;
		} else {
			std::cout << "Failed to enc for index: " << i << std::endl;
		}
	}

	// for (int i = 0; i < 6; i++) {
	// 	int val = 0;
	// 	if (q.try_dequeue(val)) {
	// 		std::cout << "dequeued: " << val << std::endl;
	// 	} else {
	// 		std::cout << "Failed to deq for index: " << i << std::endl;
	// 	}
	// }

    return 0;
}
