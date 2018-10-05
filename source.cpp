#include "Promise.h"
#include <iostream>
#include <stdexcept>
#include <map>

int main(void)
{
	bool gotMyPayCheck = true;

	auto my_prom = promise([&](Promises::Settlement settle) {
		if (gotMyPayCheck)
			settle.resolve(7000);
		else
			settle.reject(std::logic_error("I did not get paid :("));
	});

	auto my_prom2 = promise([&](Promises::Settlement settle) {
		settle.resolve(10);
	});

	auto my_prom3 = promise([&](Promises::Settlement settle) {
		settle.resolve(242);
	});

	std::vector<std::shared_ptr<Promises::PendingPromise>> proms;
	proms.push_back(my_prom);
	proms.push_back(my_prom2);
	proms.push_back(my_prom3);
	proms.push_back(Promises::Resolve(400));
	proms.push_back(Promises::Resolve(123));
	proms.push_back(Promises::Resolve(2));

	// my_prom->then<int>([&](int value) {
	// //value == 4000
	// std::cout << "I was paid $" << value << std::endl;
	// std::cout << "Did I deposit my check in the bank? ";
	// return Promises::Resolve('Y');
	// })->then<char>([&](char value) {
	// //value == 'Y'
	// std::cout << value << std::endl;
	// std::cout << "Did I withdraw from the ATM? ";
	// return Promises::Resolve(true);
	// })->_catch([&](std::exception e) {
	// //e is some thrown exception, either from rejected promise
	// //or system threw exception.
	// std::cout << e.what() << std::endl;
	// std::cout << "But did program recover? ";
	// return Promises::Resolve(true);
	// })->then<bool>([&](bool value) {
	// //value == true
	// std::cout << value << std::endl;
	// return Promises::Resolve(3);
	// });

	// my_prom->then<int>([&](int value) {
	// //value == 4000
	// std::cout << "The value is still " << value << std::endl;
	// return Promises::Resolve(3.14);
	// })->then<double>([&](double value) {
	// std::cout << "Double Value is: " << value << std::endl;
	// return Promises::Resolve<std::string>("IUPUI");
	// })->finally<std::string>([&](std::string value) {
	// std::cout << "inside finally " + value << std::endl;
	// return Promises::Reject(std::runtime_error("some error"));
	// });

	Promises::all(proms)->then<std::vector<int>>([&](std::vector<int> values) {
		std::cout << "in then" << std::endl;
		for (int i = 0; i<values.size(); ++i)
			std::cout << values[i] << std::endl;

		return Promises::Resolve<char>(' ');
	});
	std::map<std::string, std::shared_ptr<Promises::PendingPromise>> mymap;
	mymap[std::string("prom1")] = my_prom;
	mymap[std::string("prom2")] = my_prom2;
	mymap[std::string("prom3")] = my_prom3;
	mymap[std::string("prom4")] = Promises::Resolve(1004);
	mymap[std::string("prom5")] = Promises::Resolve(123);
	mymap[std::string("prom6")] = Promises::Resolve(40);


	Promises::hash(mymap)->then<std::map<std::string, int>>([&](std::map<std::string, int> values) {
		std::cout << "in then" << std::endl;
		for (auto it = values.begin(); it != values.end(); ++it)
			std::cout << it->first << ", " << it->second << std::endl;

		return Promises::Resolve<char>(' ');
	});

	return 0;
}
