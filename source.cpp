#include "Promise.h"
#include <iostream>
#include <string>

int main(void)
{
	bool got_new_car = true;

	auto my_prom = promise([&](Promises::Settlement settle) {
		if (got_new_car)
			settle.resolve<std::string>("V12 Engine!");
		else
			settle.reject(std::logic_error("I did not get a car :("));
	});

	my_prom->then<std::string>([&](std::string value) {
		std::cout << value << std::endl;
		std::cout << "But does it have cup holders? ";
		return Promises::Resolve('Y');
	});

	//starts new chain
	my_prom->then<std::string>([&](std::string value) {
		std::cout << "The value is still " << value << std::endl;
		return Promises::Resolve<std::string>("yes new chain");
	});

	return 0;
}