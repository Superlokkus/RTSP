#define BOOST_TEST_MODULE rtsp_headers_tests

#include <boost/test/included/unit_test.hpp>

#include <rtsp_headers.hpp>
#include <iterator>

BOOST_AUTO_TEST_SUITE(rtsp_headers)

struct transport_phrases_fixture {
    std::string simple_transport_spec{"RTP/AVP/TCP"};

    std::string transport_specs{"RTP/AVP;multicast;ttl=127;mode=\"PLAY\","
                                "RTP/AVP;unicast;client_port=3456-3457;mode=\"PLAY\""};

    std::vector<std::string> invalid_stuff{
            {"RTSP/1.0\t200 \t  OK\r\n"},
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
    rtsp::headers::transport specs{};

    void parse_phrase(const std::string &phrase) {
        rtsp::headers::transport_grammar<std::string::const_iterator> grammar{};
        begin = phrase.cbegin();
        end = phrase.cend();
        success = boost::spirit::qi::phrase_parse(begin, end, grammar,
                                                  boost::spirit::ascii::space, specs);
    }
};

BOOST_FIXTURE_TEST_SUITE(transport_header_tests, transport_phrases_fixture)

BOOST_AUTO_TEST_CASE(simple_specs_test) {
    parse_phrase(simple_transport_spec);
    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
    BOOST_REQUIRE_EQUAL(specs.specifications.size(), 1);
    const auto &first_spec = specs.specifications.at(0);
    BOOST_CHECK_EQUAL(first_spec.transport_protocol, "RTP");
    BOOST_CHECK_EQUAL(first_spec.profile, "AVP");
    BOOST_REQUIRE_EQUAL(bool(first_spec.lower_transport), true);
    BOOST_CHECK_EQUAL(first_spec.lower_transport.get(), "TCP");
}

BOOST_AUTO_TEST_CASE(transport_specs_test) {
    parse_phrase(transport_specs);
    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
    std::cout << "WOOT: \"" << std::string{begin, end} << "\"" << std::endl;
    BOOST_REQUIRE_EQUAL(specs.specifications.size(), 2);
    const auto &first_spec = specs.specifications.at(0);
    const auto &second_spec = specs.specifications.at(1);
    BOOST_CHECK_EQUAL(first_spec.transport_protocol, "RTP");
    BOOST_CHECK_EQUAL(second_spec.transport_protocol, "RTP");
}

BOOST_AUTO_TEST_SUITE_END()

struct transport_spec_phrases_fixture {
    std::string spec_multi_ttl_play{"RTP/AVP;multicast;ttl=127;mode=\"PLAY\""};
    std::string spec_uni_client_play{"RTP/AVP;unicast;client_port=3456-3457;mode=\"PLAY\""};

    std::string::const_iterator begin{};
    std::string::const_iterator end{};
    bool success{false};
    rtsp::headers::transport::transport_spec spec{};

    void parse_phrase(const std::string &phrase) {
        rtsp::headers::common_rules<std::string::const_iterator> rules{};
        begin = phrase.cbegin();
        end = phrase.cend();
        success = boost::spirit::qi::phrase_parse(begin, end, rules.transport_spec_,
                                                  boost::spirit::ascii::space, spec);
    }
};

BOOST_FIXTURE_TEST_SUITE(transport_header_spec_tests, transport_spec_phrases_fixture)

BOOST_AUTO_TEST_CASE(spec_multi_ttl_play_test) {
    parse_phrase(spec_multi_ttl_play);
    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
    BOOST_CHECK_EQUAL(spec.transport_protocol, "RTP");
    BOOST_CHECK_EQUAL(spec.profile, "AVP");
    BOOST_CHECK(!spec.lower_transport);
    BOOST_CHECK_EQUAL(spec.parameters.size(), 3);
    BOOST_CHECK_EQUAL(boost::get<rtsp::headers::transport::transport_spec::ttl>(spec.parameters.at(1)), 127);
    BOOST_CHECK_EQUAL(boost::get<rtsp::headers::transport::transport_spec::mode>(spec.parameters.at(2)), "PLAY");
    BOOST_CHECK_EQUAL(boost::get<rtsp::string>(spec.parameters.at(0)), "multicast");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
