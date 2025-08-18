#include <iostream>
#include <stdexcept>

#include "lock_free_mpmc_bounded.hpp"

int main() {

	mpmc_bounded_queue<int> q(8);

	q.try_enqueue(1);
	q.try_enqueue(2);
	q.try_enqueue(3);
	q.try_enqueue(4);

	for (int i = 0; i < 4; i++) {
		int val = 0;
		q.try_dequeue(val);
		std::cout << "dequeued: " << val << std::endl;
	}

    return 0;
}
