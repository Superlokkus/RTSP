#define BOOST_TEST_MODULE My Test

#include <boost/test/included/unit_test.hpp>

#include <boost_network_adapter.hpp>
#include <rtsp_request.hpp>

BOOST_AUTO_TEST_SUITE(test_suite)

    BOOST_AUTO_TEST_CASE(FIRST_TEST) {
        lib a;
        BOOST_CHECK_EQUAL(a.foo(), 5);
        BOOST_CHECK(true);
    }

BOOST_AUTO_TEST_SUITE_END()


