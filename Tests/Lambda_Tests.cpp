#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../Lambda.h"
#include <cstring>

BOOST_AUTO_TEST_SUITE(LAMBDA_SUITE)

BOOST_AUTO_TEST_CASE(RejectedLambda_Test) {

	//lambda return void
	Promises::ILambda* lam1 = rejected_lambda([](std::exception &ex){
		BOOST_CHECK(strcmp(ex.what(), "test") == 0);
	});
	
	//lambda return promise
	Promises::ILambda* lam2 = rejected_lambda([](std::exception &ex) -> Promises::IPromise* {
		BOOST_CHECK(strcmp(ex.what(), "test") == 0);
		
		Promises::IPromise* prom = nullptr;
		return prom;
	});
	
	Promises::RejectedState state("test");
	
	auto p1 = lam1->call(&state);
	auto p2 = lam2->call(&state);
	
	BOOST_CHECK(p1 == nullptr);
	BOOST_CHECK(p2 == nullptr);
	
	Promises::State* s = nullptr;
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
	
	delete lam1;
	delete lam2;
}

BOOST_AUTO_TEST_CASE(ResolvedLambda_Test) {

	//lambda return void
	Promises::ILambda* lam1 = resolved_lambda([](int num){
		BOOST_CHECK(num == 10);
	});
	
	//lambda return promise
	Promises::ILambda* lam2 = resolved_lambda([](char c) {
		BOOST_CHECK(c == '$');
		Promises::IPromise* prom = nullptr;
		return prom;
	});
	
	Promises::ResolvedState<int> state1(new int(10));
	Promises::ResolvedState<char> state2(new char('$'));
	
	auto p1 = lam1->call(&state1);
	auto p2 = lam2->call(&state2);
	
	BOOST_CHECK(p1 == nullptr);
	BOOST_CHECK(p2 == nullptr);
	
	Promises::State* s = nullptr;
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
	
	delete lam1;
	delete lam2;
}

BOOST_AUTO_TEST_SUITE_END()