#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#include "bounded_mpmc_pool.hpp"

using namespace stel;

int main() {

	bounded_mpmc_pool pool(16, 256);

	pool.submit([]() {
		std::cout << "Hello world from task 1\n";
	});

	pool.submit([]() {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::cout << "Hello world from task 2 - task 2 goes to sleep for 2 seconds\n";
	});

	pool.submit([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::cout << "Hello world from task 3 - task 3 goes to sleep for 500 milliseconds\n";
	});

	pool.submit([]() {
		std::cout << "Hello world from task 4\n";
	});


	std::this_thread::sleep_for(std::chrono::seconds(4));

	return 0;
}
