#define BOOST_TEST_MODULE My Test

#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite)

    BOOST_AUTO_TEST_CASE(FIRST_TEST) {
        BOOST_CHECK(true);
    }

BOOST_AUTO_TEST_SUITE_END()


