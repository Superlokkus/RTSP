#define BOOST_TEST_MODULE streaming_lib_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_message.hpp>
#include <string>
#include <iterator>
#include <vector>


BOOST_AUTO_TEST_SUITE(rtsp)

    template<typename RTSP_MESSAGE_TYPE>
    struct rtsp_phrases_fixture {
        std::string ok_response{"RTSP/1.0\t200 \t  OK\r\n"};
        std::string ok_response_with_crlf{"RTSP/1.0\t200 \t  OK\r\n\r\n"};
        std::string ok_response_with_header{ok_response + "Session: 42\r\n 31\r\nCseq: 3\r\n" + "\r\n"};
        std::string ok_rtsp2_3_repsonse{"RTSP/2.3 200\tOK\r\n\r\n"};
        std::string ok_rtsp25_19_repsonse{"RTSP/25.19 200\tOK\r\n\r\n"};
        std::string ok_http_1_1_repsonse{"HTTP/1.1 200  OK\r\n\r\n"};
        std::string ok_rtsp1_1_repsonse{"RTSP/1.1 200  OK\r\n\r\n"};
        std::string pause_request{std::string{
                "PAUSE rtsp://audio.example.com/twister/audio.en/lofi RTSP/1.0\r\n"} +
                                  "Session:  4231\r\nCseq:\t 3\r\nRange: n\r\n pt=37\r\n\r\n"};
        std::string setup_request{
                "SETUP rtsp://example.com/foo/bar/baz.rm RTSP/1.0\r\n\r\n"
        };
        std::vector<std::string> invalid_stuff{
                {"oijsdisdjlisdfjlrur93209p831§\"§=)§"},
                {"\n\r fajfajkj \n\n\r\r\n"},
                {"\r\n afsfas3244afs"},
                {" \r\n"},
                {"jjlfsjflsjkl"},
                {"RTSP/1.0\t2000 \t  OK\r\n\r\n"},
                {"RTSP/1.0\t20 \t  OK\r\n\r\n"},
                {"RTSP/.0\t200 \t  OK\r\n\r\n"},
                {"RTSP/ \t200 \t  OK\r\n\r\n"}
        };
        std::string::const_iterator begin{};
        std::string::const_iterator end{};
        bool success{false};
    };

    BOOST_AUTO_TEST_SUITE(rtsp_response)

        struct rtsp_reponse_fixture : rtsp_phrases_fixture<rtsp_reponse_fixture> {
            rtsp::response response{};

            void parse_phrase(const std::string &phrase) {
                rtsp::rtsp_response_grammar<std::string::const_iterator> response_grammar{};
                begin = phrase.cbegin();
                end = phrase.cend();
                success = boost::spirit::qi::phrase_parse(begin, end, response_grammar,
                                                          boost::spirit::ascii::space, response);
            }
        };
        BOOST_FIXTURE_TEST_SUITE(rtsp_response_startline, rtsp_reponse_fixture)

            BOOST_AUTO_TEST_CASE(ok_response_test) {
                parse_phrase(ok_response);
                BOOST_CHECK(!success);
            }

            BOOST_AUTO_TEST_CASE(ok_response_with_crlf_test) {
                parse_phrase(ok_response_with_crlf);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 1);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 0);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
                BOOST_CHECK_EQUAL(response.headers.size(), 0);
            }

            BOOST_AUTO_TEST_CASE(ok_response_with_header_test) {
                parse_phrase(ok_response_with_header);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 1);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 0);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
                BOOST_CHECK_EQUAL(response.headers.size(), 2);
            }

            BOOST_AUTO_TEST_CASE(ok_rtsp2_3_repsonse_test) {
                parse_phrase(ok_rtsp2_3_repsonse);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 2);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 3);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
                BOOST_CHECK_EQUAL(response.headers.size(), 0);
            }

            BOOST_AUTO_TEST_CASE(ok_rtsp25_19_repsonse_test) {
                parse_phrase(ok_rtsp25_19_repsonse);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 25);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 19);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
                BOOST_CHECK_EQUAL(response.headers.size(), 0);
            }

            BOOST_AUTO_TEST_CASE(ok_http_1_1_repsonse_test) {
                parse_phrase(ok_http_1_1_repsonse);
                BOOST_CHECK(!success);
                BOOST_CHECK(begin != end);
            }

            BOOST_AUTO_TEST_CASE(ok_rtsp1_1_repsonse_test) {
                parse_phrase(ok_rtsp1_1_repsonse);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 1);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 1);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
                BOOST_CHECK_EQUAL(response.headers.size(), 0);
            }

            BOOST_AUTO_TEST_CASE(pause_request_test) {
                parse_phrase(pause_request);
                BOOST_CHECK(!success);
                BOOST_CHECK(begin != end);
            }

            BOOST_AUTO_TEST_CASE(invalid_stuff_test) {
                for (const auto &phrase : invalid_stuff) {
                    BOOST_TEST_MESSAGE(phrase);
                    parse_phrase(phrase);
                    BOOST_CHECK(!success);
                }
            }

        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_CASE(gen) {
            rtsp::response response{1, 1, 200, "OK"};
            std::string output;
            rtsp::generate_response(std::back_inserter(output), response);
            BOOST_CHECK_EQUAL(output, "RTSP/1.1 200 OK\r\n\r\n");
        }

        BOOST_AUTO_TEST_CASE(gen_with_headers) {
            rtsp::response response{1, 1, 200, "OK", {
                    rtsp::header{"Session", " 4231"},
                    rtsp::header{"Cseq", "3"},
            }};
            std::string output;
            rtsp::generate_response(std::back_inserter(output), response);
            const auto expected = std::string{"RTSP/1.1 200 OK\r\n"}
                                  + "Session:  4231\r\n"
                                  + "Cseq: 3\r\n"
                                  + "\r\n";
            BOOST_CHECK_EQUAL(output, expected);
        }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(rtsp_request)

        struct rtsp_request_phrases_fixture : rtsp_phrases_fixture<rtsp_request_phrases_fixture> {

            rtsp::request request{};

            void parse_phrase(const std::string &phrase) {
                rtsp::rtsp_request_grammar<std::string::const_iterator> request_grammar{};
                begin = phrase.cbegin();
                end = phrase.cend();
                success = boost::spirit::qi::phrase_parse(begin, end, request_grammar,
                                                          boost::spirit::ascii::space, request);
            }
        };

        BOOST_FIXTURE_TEST_SUITE(rtsp_request_startline, rtsp_request_phrases_fixture)


            BOOST_AUTO_TEST_CASE(setup_request_test) {
                parse_phrase(setup_request);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(request.rtsp_version_major, 1);
                BOOST_CHECK_EQUAL(request.rtsp_version_minor, 0);
                BOOST_CHECK_EQUAL(request.method_or_extension, "SETUP");
                BOOST_CHECK_EQUAL(request.uri, "rtsp://example.com/foo/bar/baz.rm");
                BOOST_CHECK_EQUAL(request.headers.size(), 0);
            }

            BOOST_AUTO_TEST_CASE(pause__request_test) {
                parse_phrase(pause_request);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(request.rtsp_version_major, 1);
                BOOST_CHECK_EQUAL(request.rtsp_version_minor, 0);
                BOOST_CHECK_EQUAL(request.method_or_extension, "PAUSE");
                BOOST_CHECK_EQUAL(request.uri, "rtsp://audio.example.com/twister/audio.en/lofi");
                BOOST_CHECK_EQUAL(request.headers.size(), 3);
            }

            BOOST_AUTO_TEST_CASE(invalid_stuff_test) {
                for (const auto &phrase : invalid_stuff) {
                    BOOST_TEST_MESSAGE(phrase);
                    parse_phrase(phrase);
                    BOOST_CHECK(!success);
                }
            }

        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_CASE(gen) {
            rtsp::request request{"PLAY", "rtspu://127.0.0.1:8888/meeti-ng.en?", 1, 0};
            std::string output;
            rtsp::generate_request(std::back_inserter(output), request);
            BOOST_CHECK_EQUAL(output, "PLAY rtspu://127.0.0.1:8888/meeti-ng.en? RTSP/1.0\r\n\r\n");
        }

        BOOST_AUTO_TEST_CASE(gen_with_headers) {
            rtsp::request request{"PLAY", "rtspu://127.0.0.1:8888/meeti-ng.en?", 1, 0, {
                    rtsp::header{"Session", " 4231"},
                    rtsp::header{"Cseq", "3"},
                    rtsp::header{"Range", "npt=37"}
            }};
            std::string output;
            rtsp::generate_request(std::back_inserter(output), request);
            const auto expected = std::string{"PLAY rtspu://127.0.0.1:8888/meeti-ng.en? RTSP/1.0\r\n"}
                                  + "Session:  4231\r\n"
                                  + "Cseq: 3\r\n"
                                  + "Range: npt=37\r\n"
                                  + "\r\n";
            BOOST_CHECK_EQUAL(output, expected);

        }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

