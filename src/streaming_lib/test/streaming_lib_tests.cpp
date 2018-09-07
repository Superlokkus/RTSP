#define BOOST_TEST_MODULE streaming_lib_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_request.hpp>
#include <string>
#include <iterator>
#include <vector>


BOOST_AUTO_TEST_SUITE(rtsp)

    BOOST_AUTO_TEST_SUITE(rtsp_response)
        struct rtsp_phrases_fixture {
            std::string ok_response{"RTSP/1.0\t200 \t  OK"};
            std::string ok_response_with_crlf{"RTSP/1.0\t200 \t  OK\r\n"};
            std::string ok_rtsp2_3_repsonse{"RTSP/2.3 200\tOK\r\n"};
            std::string ok_rtsp25_19_repsonse{"RTSP/25.19 200\tOK\r\n"};
            std::string ok_http_1_1_repsonse{"HTTP/1.1 200  OK\r\n"};
            std::string ok_rtsp1_1_repsonse{"RTSP/1.1 200  OK\r\n"};
            std::string pause_request{
                    "PAUSE rtsp://audio.example.com/twister/audio.en/lofi RTSP/1.0\r\nSession: 4231\r\nCseq: 3\r\nRange: npt=37"};
            std::vector<std::string> invalid_stuff{
                    {"oijsdisdjlisdfjlrur93209p831§\"§=)§"},
                    {"\n\r fajfajkj \n\n\r\r\n"},
                    {"\r\n afsfas3244afs"},
                    {" \r\n"},
                    {"jjlfsjflsjkl"},
                    {"RTSP/1.0\t2000 \t  OK\r\n"},
                    {"RTSP/1.0\t20 \t  OK\r\n"},
                    {"RTSP/.0\t200 \t  OK\r\n"},
                    {"RTSP/ \t200 \t  OK\r\n"}
            };
            std::string::const_iterator begin{};
            std::string::const_iterator end{};
            rtsp::response response{};
            bool success{false};

            void parse_phrase(const std::string &phrase) {
                rtsp::rtsp_response_grammar<std::string::const_iterator> response_grammar{};
                begin = phrase.cbegin();
                end = phrase.cend();
                success = boost::spirit::qi::phrase_parse(begin, end, response_grammar,
                                                          boost::spirit::ascii::space, response);
            }
        };
        BOOST_FIXTURE_TEST_SUITE(rtsp_response_startline, rtsp_phrases_fixture)
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
            }

            BOOST_AUTO_TEST_CASE(ok_rtsp2_3_repsonse_test) {
                parse_phrase(ok_rtsp2_3_repsonse);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 2);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 3);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
            }

            BOOST_AUTO_TEST_CASE(ok_rtsp25_19_repsonse_test) {
                parse_phrase(ok_rtsp25_19_repsonse);
                BOOST_CHECK(success);
                BOOST_CHECK(begin == end);
                BOOST_CHECK_EQUAL(response.rtsp_version_major, 25);
                BOOST_CHECK_EQUAL(response.rtsp_version_minor, 19);
                BOOST_CHECK_EQUAL(response.status_code, 200);
                BOOST_CHECK_EQUAL(response.reason_phrase, "OK");
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
            rtsp::response response{1,1,200,"OK"};
            std::string output;
            rtsp::generate_response(std::back_inserter(output), response);
            BOOST_CHECK_EQUAL(output, "RTSP/1.1 200 OK\r\n");
        }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

