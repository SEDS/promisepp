#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../Lambda.h"
#include <cstring>
#include <memory>

BOOST_AUTO_TEST_SUITE(LAMBDA_SUITE)

BOOST_AUTO_TEST_CASE(RejectedLambda_Test) {

	//lambda return void
	auto lam1 = Promises::rejected_lambda([](const std::exception &ex){
		BOOST_CHECK(strcmp(ex.what(), "test") == 0);
	});
	
	//lambda return promise
	auto lam2 = Promises::rejected_lambda([](const std::exception &ex) -> Promises::IPROM_TYPE {
		BOOST_CHECK(strcmp(ex.what(), "test") == 0);
		
		Promises::IPROM_TYPE prom = nullptr;
		return prom;
	});
	
	Promises::STATE_TYPE state = std::make_shared<Promises::RejectedState>("test");
	
	auto p1 = lam1->call(state);
	auto p2 = lam2->call(state);
	
	BOOST_CHECK(p1 == nullptr);
	BOOST_CHECK(p2 == nullptr);
	
	Promises::STATE_TYPE s = nullptr;
	try {
		lam1->call(s);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "RejectedLambda.call(): state is null") == 0);
	}
	
	try {
		lam2->call(s);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "RejectedLambda.call(): state is null") == 0);
	}
}

BOOST_AUTO_TEST_CASE(ResolvedLambda_Test) {

	//lambda return void
	auto lam1 = Promises::resolved_lambda([](int num){
		BOOST_CHECK(num == 10);
	});
	
	//lambda return promise
	auto lam2 = Promises::resolved_lambda([](char c) -> Promises::IPROM_TYPE {
		BOOST_CHECK(c == '$');
		Promises::IPROM_TYPE prom = nullptr;
		return prom;
	});
	
	Promises::STATE_TYPE state1 = std::make_shared<Promises::ResolvedState<int>>(10);
	Promises::STATE_TYPE state2 = std::make_shared<Promises::ResolvedState<char>>('$');
	
	auto p1 = lam1->call(state1);
	auto p2 = lam2->call(state2);
	
	BOOST_CHECK(p1 == nullptr);
	BOOST_CHECK(p2 == nullptr);
	
	Promises::STATE_TYPE s = nullptr;
	try {
		lam1->call(s);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "ResolvedLambda.call(): state is null") == 0);
	}
	
	try {
		lam2->call(s);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "ResolvedLambda.call(): state is null") == 0);
	}
}

BOOST_AUTO_TEST_SUITE_END()