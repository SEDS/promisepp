#include "State.h"
#include <iostream>
#include <cstring>

int main(void) {

	std::logic_error s1("nyalia");
	std::cout << strcmp(s1.what(), std::logic_error("nyalia").what()) << std::endl;
	// bool got_new_car = true;

	// auto my_prom = promise([&](Promises::Settlement settle) {
	// 	if (got_new_car)
	// 		settle.resolve<std::string>("V12 Engine!");
	// 	else
	// 		settle.reject(std::logic_error("I did not get a car :("));
	// });

	// my_prom->then([&](std::string value) {
	// 	std::cout << value << std::endl;
	// 	std::cout << "But does it have cup holders? ";
	// 	return Promises::Resolve('Y');
	// })->then([&](char value ){
	// 	std::cout << value << std::endl;
	// });

	// try {
	// 	auto value = Promises::await<std::string>(another_prom);
	// 	std::cout << *value << std::endl;
	// } catch(std::exception &ex) {
	// 	std::cout << "main failure:" << ex.what() << std::endl;
	// }

	return 0;
}
