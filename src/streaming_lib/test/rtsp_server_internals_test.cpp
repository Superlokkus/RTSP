#define BOOST_TEST_MODULE streaming_lib_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_server_internals.hpp>

BOOST_AUTO_TEST_SUITE(rtsp_server_internals)

struct rtsp_server_internals_fixture {
    rtsp_server_internals_fixture() :
            server_state{} {
    }

    rtsp::server::rtsp_server_state server_state;
};
BOOST_FIXTURE_TEST_SUITE(rtsp_server_internals_fixture_suite, rtsp_server_internals_fixture)

BOOST_AUTO_TEST_CASE(ok_response_test) {

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
