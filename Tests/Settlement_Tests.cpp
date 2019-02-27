#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../Settlement.h"
#include <string>

BOOST_AUTO_TEST_SUITE(SETTLEMENT_SUITE)

BOOST_AUTO_TEST_CASE(Defult_Constructor_Test) {
    Promises::Settlement settlement;
	
	try {
		//this should cause error cause
		//the internal promise is nullptr
		settlement.reject(std::logic_error("test"));
	} catch(const std::exception &e) {
		std::cout << e.what() << std::endl;
		BOOST_CHECK(true);
	}
}

BOOST_AUTO_TEST_SUITE_END()