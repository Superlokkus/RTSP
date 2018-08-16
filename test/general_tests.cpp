#define BOOST_TEST_MODULE My Test

#include <boost/test/included/unit_test.hpp>

#include <boost_network_adapter.hpp>


BOOST_AUTO_TEST_CASE(FIRST_TEST) {
    lib a;
    BOOST_CHECK_EQUAL(a.foo(), 5);
    BOOST_CHECK(true);
}



