#define BOOST_TEST_MODULE streaming_lib_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_server_internals.hpp>

BOOST_AUTO_TEST_SUITE(rtsp_server_internals)

BOOST_AUTO_TEST_CASE(harmonize_headers_test) {
    const rtsp::raw_headers raw_headers{
            {{"CSeq"},             {"5"}},
            {{"SomeComplexField"}, {"42"}},
            {{"SomeComplexField"}, {"76"}},
    };
    const rtsp::headers harmonized_headers{
            {{"cseq"},             {"5"}},
            {{"somecomplexfield"}, {"42,76"}},
    };
    const auto shoud_be_harmonized_headers = rtsp::server::rtsp_server_state::harmonize_headers(raw_headers);
    BOOST_TEST(shoud_be_harmonized_headers == harmonized_headers);
}

struct rtsp_server_internals_fixture {

    rtsp::server::rtsp_server_state server_state;
    rtsp::request valid_options_request{"OPTIONS", "*", 1, 0, {{"cseq", "0"}}};
    rtsp::request invalid_options_request{"OPTIONS", "*", 1, 0, {}};

    rtsp::request invalid_method{"dskfjksjdfIIUER", "*", 1, 0, {}};

};
BOOST_FIXTURE_TEST_SUITE(rtsp_server_internals_fixture_suite, rtsp_server_internals_fixture)

BOOST_AUTO_TEST_CASE(valid_options_request_test) {
    const rtsp::response correct_response{1, 0, 200, "OK", {{"cseq", "0"}}};

    const auto generated_response = server_state.handle_incoming_request(valid_options_request);
    BOOST_CHECK_EQUAL(generated_response.status_code, correct_response.status_code);
    BOOST_TEST(generated_response.headers == correct_response.headers);
}

BOOST_AUTO_TEST_CASE(invalid_options_request_test) {
    const rtsp::response correct_response{1, 0, 400, "Bad Request", {}};

    const auto generated_response = server_state.handle_incoming_request(invalid_options_request);
    BOOST_CHECK_EQUAL(generated_response.status_code, correct_response.status_code);
}

BOOST_AUTO_TEST_CASE(invalid_method_test) {
    const rtsp::response correct_response{1, 0, 501, "Not Implemented", {{"cseq", "0"}}};

    const auto generated_response = server_state.handle_incoming_request(invalid_method);
    BOOST_CHECK_EQUAL(generated_response.status_code, correct_response.status_code);
    BOOST_TEST(generated_response.headers == correct_response.headers);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
