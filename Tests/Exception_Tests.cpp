#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../Promise_Error.h"
#include <cstring>

BOOST_AUTO_TEST_SUITE(EXCEPTION_SUITE)

BOOST_AUTO_TEST_CASE(String_Constructor_Test) {
    Promise_Error err(std::string("test"));

    BOOST_CHECK(strcmp(err.what(), "test") == 0);
}

BOOST_AUTO_TEST_CASE(CString_Constructor_Test) {
    Promise_Error err("test");

    BOOST_CHECK(strcmp(err.what(), "test") == 0);
}

BOOST_AUTO_TEST_CASE(Exception_Constructor_Test) {
    Promise_Error err(std::logic_error("test"));

    BOOST_CHECK(strcmp(err.what(), "test") == 0);
}

BOOST_AUTO_TEST_CASE(Copy_Constructor_Test) {
    Promise_Error err(std::logic_error("test"));
    Promise_Error copy(err);

    BOOST_CHECK(strcmp(err.what(), copy.what()) == 0);
}

BOOST_AUTO_TEST_CASE(Equality_Op_Test) {
    Promise_Error err(std::string("test"));
    Promise_Error ex(std::string("test"));;

    BOOST_CHECK(err == ex);
}

BOOST_AUTO_TEST_CASE(Inequality_Op_Test) {
    Promise_Error err(std::string("test"));
    Promise_Error ex(std::string("tset"));;

    BOOST_CHECK(err != ex);
}


BOOST_AUTO_TEST_SUITE_END()