#include "Promise.h"
#include <iostream>
#include <string>

int main(void) {
	bool got_new_car = true;

	auto my_prom = promise([&got_new_car](Promises::Settlement settle) {
		if (got_new_car)
			settle.resolve<std::string>("V12 Engine!");
		else
			settle.reject(std::logic_error("I did not get a car :("));
	});

	auto another = my_prom->then([](std::string value) {
		std::cout << value << std::endl;
		std::cout << "But does it have cup holders? ";
		return Promises::Resolve('Y');
	})->then([](char value) {
		std::cout << value << std::endl;
	});

	try {
		auto value = Promises::await<void>(another);
		std::cout << value << std::endl;
	} catch(std::exception &ex) {
		std::cout << "main failure:" << ex.what() << std::endl;
	}

	return 0;
}
