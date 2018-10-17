#define BOOST_TEST_MODULE streaming_lib_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_server_internals.hpp>
#include <rtsp_headers.hpp>

#include <algorithm>

BOOST_AUTO_TEST_SUITE(rtsp_server_internals)

BOOST_AUTO_TEST_CASE(harmonize_headers_test) {
    const rtsp::raw_headers raw_headers{
            {{"CSeq"},             {"5"}},
            {{"SomeComplexField"}, {"42"}},
            {{"SomeComplexField"}, {"76"}},
    };
    const rtsp::normalized_headers harmonized_headers{
            {{"cseq"},             {"5"}},
            {{"somecomplexfield"}, {"42,76"}},
    };
    const auto shoud_be_harmonized_headers = rtsp::headers::normalize_headers(raw_headers);
    BOOST_TEST(shoud_be_harmonized_headers == harmonized_headers);
}

struct rtsp_server_internals_fixture {
    rtsp_server_internals_fixture() {
        /*auto first_session = some_sessions_server_state.handle_incoming_request(
                {"SETUP", "rtsp://foo.com/bar.file",1,0, {{"CSeq", "0"}}});*/
    }

    rtsp::server::rtsp_server_state fresh_server_state;

    rtsp::server::rtsp_server_state some_sessions_server_state;
    /*!
     * Session, next CSeq, last Method
     */
    std::vector<std::tuple<char, unsigned, char>> sessions_to_some_sessions_server_state;


    rtsp::request valid_options_request{"OPTIONS", "*", 1, 0, {{"CSeq", "0"}}};
    rtsp::request invalid_options_request{"OPTIONS", "*", 1, 0, {}};



};
BOOST_FIXTURE_TEST_SUITE(rtsp_server_internals_fixture_suite, rtsp_server_internals_fixture)

BOOST_AUTO_TEST_CASE(valid_options_request_test) {
    const rtsp::response correct_response{1, 0, 200, "OK"};

    const auto generated_response = fresh_server_state.handle_incoming_request(valid_options_request);
    BOOST_CHECK_EQUAL(generated_response.status_code, correct_response.status_code);
    auto cseq_it = std::find_if(generated_response.headers.cbegin(), generated_response.headers.cend(),
                                [](const auto &value) {
                                    return value.first == "CSeq";
                                });
    BOOST_REQUIRE(cseq_it != generated_response.headers.cend());
    BOOST_CHECK_EQUAL(cseq_it->second, "0");
}

BOOST_AUTO_TEST_CASE(invalid_options_request_test) {
    const rtsp::response correct_response{1, 0, 400, "Bad Request", {}};

    const auto generated_response = fresh_server_state.handle_incoming_request(invalid_options_request);
    BOOST_CHECK_EQUAL(generated_response.status_code, correct_response.status_code);
}



BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
