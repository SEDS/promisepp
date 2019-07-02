#include "Promise.h"
#include <iostream>
#include <string>

int main(void) {
	auto prom = promise([](Promises::Settlement settle) {
		settle.resolve<std::string>("V12 Engine!");
	});

	prom->then([](std::string value) {
		std::cout << value << std::endl;
		return Promises::Resolve(51);
	})->then([](int value) {
		std::cout << value << std::endl;
	});

	prom->then([](std::string value) {
		std::cout << "The value is still " << value << std::endl;
	});

	return 0;
}
