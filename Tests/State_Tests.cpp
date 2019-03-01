#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../State.h"
#include <cstring>
#include <iostream>

BOOST_AUTO_TEST_SUITE(STATE_SUITE)

BOOST_AUTO_TEST_CASE(Default_Constructor_Test) {
    Promises::ResolvedState<char> resolved;
	Promises::RejectedState rejected;

	BOOST_CHECK(resolved.get_value() == nullptr);
    BOOST_CHECK(strcmp(resolved.get_reason().what(), Promises::noerr.what()) == 0);
	
	BOOST_CHECK(rejected.get_value() == nullptr);
    BOOST_CHECK(strcmp(rejected.get_reason().what(), Promises::noerr.what()) == 0);
}

BOOST_AUTO_TEST_CASE(Pending_Constructor_Test) {
	Promises::PendingState pending;
	
	BOOST_CHECK(pending.get_value() == nullptr);
    BOOST_CHECK(strcmp(pending.get_reason().what(), Promises::noerr.what()) == 0);
}

BOOST_AUTO_TEST_CASE(Resolved_Constructor_Test) {
	char* c = new char('$');
    Promises::ResolvedState<char> resolved(c);
	
	BOOST_CHECK((char*)resolved.get_value() == c);
    BOOST_CHECK(strcmp(resolved.get_reason().what(), Promises::noerr.what()) == 0);
}

BOOST_AUTO_TEST_CASE(Rejected_Constructor_Test) {
	Promises::RejectedState rejected1(std::logic_error("test"));
	Promises::RejectedState rejected2("test");
	Promises::RejectedState rejected3(std::string("test"));
	
	BOOST_CHECK(rejected1.get_value() == nullptr);
    BOOST_CHECK(strcmp(rejected1.get_reason().what(), "test") == 0);
	
	BOOST_CHECK(rejected2.get_value() == nullptr);
    BOOST_CHECK(strcmp(rejected2.get_reason().what(), "test") == 0);
	
	BOOST_CHECK(rejected3.get_value() == nullptr);
    BOOST_CHECK(strcmp(rejected3.get_reason().what(), "test") == 0);
}

BOOST_AUTO_TEST_CASE(Copy_Constructor_Test) {
	char* c = new char('$');
    Promises::ResolvedState<char> resolved(c);
	Promises::ResolvedState<char> copy_resolved(resolved);
	
	Promises::RejectedState rejected(std::logic_error("test"));
	Promises::RejectedState copy_rejected(rejected);

	BOOST_CHECK((char*)copy_resolved.get_value() == c);
    BOOST_CHECK(strcmp(copy_resolved.get_reason().what(), resolved.get_reason().what()) == 0);
	
	BOOST_CHECK(copy_rejected.get_value() == rejected.get_value());
    BOOST_CHECK(strcmp(copy_rejected.get_reason().what(), rejected.get_reason().what()) == 0);
}

BOOST_AUTO_TEST_CASE(Destructor_Test) {
	int* num = new int(10);
    Promises::ResolvedState<int>* resolved = new Promises::ResolvedState<int>(num);
	delete resolved;
	
	//delete integer sets integer to 0 value
	BOOST_CHECK(*num == 0);
}

BOOST_AUTO_TEST_CASE(Equality_Test) {
	char* c = new char('$');
	int* num = new int(10);

	Promises::PendingState pending1;
	Promises::PendingState pending2;
	
    Promises::ResolvedState<char> resolved1(c);
	Promises::ResolvedState<int> resolved2(num);
	
	Promises::RejectedState rejected1(std::logic_error("test"));
	Promises::RejectedState rejected2(std::logic_error("another"));

	BOOST_CHECK(pending1 == pending2);
	BOOST_CHECK(resolved1 == resolved2);
	BOOST_CHECK(rejected1 == rejected2);
}

BOOST_AUTO_TEST_CASE(Inequality_Test) {
	char* c = new char('$');
	Promises::PendingState pending;
    Promises::ResolvedState<char> resolved(c);
	Promises::RejectedState rejected(std::logic_error("test"));
	
	BOOST_CHECK(pending != rejected);
	BOOST_CHECK(resolved != rejected);
	BOOST_CHECK(rejected != resolved);
}
BOOST_AUTO_TEST_SUITE_END()