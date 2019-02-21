#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cstring>
#include "../State.h"

BOOST_AUTO_TEST_SUITE(STATE_SUITE)

BOOST_AUTO_TEST_CASE(Resolved_NonVoid_Test) {
    int *num = new int(10);
    Promises::ResolvedState<int> state(num);
	
    BOOST_CHECK((int*)state.getValue() == num);
    BOOST_CHECK(state.getReason().what() == Promises::noerr.what());
}

BOOST_AUTO_TEST_CASE(Resolved_Void_Test) {
    Promises::ResolvedState<void> state;
	
    BOOST_CHECK(state.getValue() == nullptr);
    BOOST_CHECK(state.getReason().what() == Promises::noerr.what());
}

BOOST_AUTO_TEST_CASE(Rejected_Test) {
    Promises::RejectedState state(std::logic_error("tests"));
	
    BOOST_CHECK(state.getValue() == nullptr);
    BOOST_CHECK(strcmp(state.getReason().what(), std::logic_error("tests").what()) == 0);
}

BOOST_AUTO_TEST_CASE(Copy_State_Test) {
    Promises::ResolvedState<int> state1(new int(10));
    Promises::ResolvedState<void> state2;
    Promises::RejectedState state3(std::logic_error("tests"));

    Promises::ResolvedState<int> copy1(state1);
    Promises::ResolvedState<void> copy2(state2);
    Promises::RejectedState copy3(state3);

    BOOST_CHECK(*(int*)copy1.getValue() == *(int*)state1.getValue());
    BOOST_CHECK(copy1.getReason().what() == state1.getReason().what());

    BOOST_CHECK(copy2.getValue() == state2.getValue());
    BOOST_CHECK(copy2.getReason().what() == state2.getReason().what());

    BOOST_CHECK(copy3.getValue() == state3.getValue());
    BOOST_CHECK(copy3.getReason().what() == state3.getReason().what());
}

BOOST_AUTO_TEST_CASE(Equality_Op_Test) {
    Promises::ResolvedState<int> state1(new int(10));
    Promises::ResolvedState<void> state2;
    Promises::RejectedState state3(std::logic_error("tests"));

    Promises::ResolvedState<int> copy1(state1);
    Promises::ResolvedState<void> copy2(state2);
    Promises::RejectedState copy3(state3);

    BOOST_CHECK(state1 == copy1);
    BOOST_CHECK(state2 == copy2);
    BOOST_CHECK(state3 == copy3);

    Promises::State* pstate1 = &state1;
    Promises::State* pstate2 = &state2;
    Promises::State* pstate3 = &state3;

    BOOST_CHECK(*pstate1 == copy1);
    BOOST_CHECK(*pstate2 == copy2);
    BOOST_CHECK(*pstate3 == copy3);
}

BOOST_AUTO_TEST_CASE(Inequality_Op_Test) {
    //do not test inequality of ResolvedState<void> because it's impossible to have inequal objects of that type
    Promises::ResolvedState<int> state1(new int(10));
    Promises::RejectedState state3(std::logic_error("tests"));

    Promises::ResolvedState<int> copy1(new int(20));
    Promises::RejectedState copy3(std::logic_error("stset"));

    BOOST_CHECK(state1 != copy1);
    BOOST_CHECK(state3 != copy3);

    Promises::State* pstate1 = &state1;
    Promises::State* pstate3 = &state3;

    BOOST_CHECK(*pstate1 != copy3);
    BOOST_CHECK(*pstate3 != copy1);
}

BOOST_AUTO_TEST_SUITE_END()