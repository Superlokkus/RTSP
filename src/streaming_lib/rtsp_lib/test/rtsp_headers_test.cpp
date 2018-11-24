#define BOOST_TEST_MODULE rtsp_headers_tests

#include <boost/test/included/unit_test.hpp>

#include "../include/rtsp_headers.hpp"
#include <iterator>

BOOST_AUTO_TEST_SUITE(rtsp_headers)

BOOST_AUTO_TEST_CASE(port_range_test) {
    std::string phrase{"42-1337"};
    rtsp::headers::common_rules
            <std::string::const_iterator> rules{};
    auto begin = phrase.cbegin();
    auto end = phrase.cend();
    rtsp::headers::transport::transport_spec::port_range range;
    bool success = boost::spirit::qi::phrase_parse(begin, end, rules.port_range_,
                                                   boost::spirit::ascii::space, range);
    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
    BOOST_CHECK_EQUAL(std::get<0>(range), 42);
    BOOST_CHECK_EQUAL(std::get<1>(range), 1337);
}


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

BOOST_AUTO_TEST_CASE(spec_uni_client_play_test) {
    parse_phrase(spec_uni_client_play);
    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
    BOOST_CHECK_EQUAL(spec.transport_protocol, "RTP");
    BOOST_CHECK_EQUAL(spec.profile, "AVP");
    BOOST_CHECK(!spec.lower_transport);
    BOOST_CHECK_EQUAL(spec.parameters.size(), 3);
    BOOST_CHECK_EQUAL(spec.parameters.at(1).which(), 1);
    const auto &client_port = boost::get<rtsp::headers::transport::transport_spec::port>(spec.parameters.at(1));
    const auto client_port_range = boost::get<rtsp::headers::transport::transport_spec::port_range>(
            client_port.port_numbers);
    const auto should_range = rtsp::headers::transport::transport_spec::port_range{std::make_pair(3456, 3457)};
    BOOST_CHECK(client_port_range == should_range);
    BOOST_CHECK(client_port.type == rtsp::headers::transport::transport_spec::port::port_type::client);
    BOOST_CHECK_EQUAL(boost::get<rtsp::headers::transport::transport_spec::mode>(spec.parameters.at(2)), "PLAY");
    BOOST_CHECK_EQUAL(boost::get<rtsp::string>(spec.parameters.at(0)), "unicast");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(transport_gen_test) {
    rtsp::headers::transport::transport_spec spec1{"RTP", "AVP", ::string > boost::make_optional<rtsp("UDP")};
    rtsp::headers::transport::transport_spec spec2{"RTP", "AVP", boost::none};
    rtsp::headers::transport transport{{spec1, spec2}};
    std::string output;
    rtsp::headers::generate_transport_header_grammar<std::back_insert_iterator<std::string>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output), gen_grammar, transport);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(output, "RTP/AVP/UDP,RTP/AVP");
}

BOOST_AUTO_TEST_CASE(mode_gen_test) {
    auto mode = rtsp::headers::transport::transport_spec::mode{"PLAY"};
    std::string output;
    rtsp::headers::transport_generators<std::back_insert_iterator<std::string>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output), gen_grammar.mode_gen, mode);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(output, "mode=\"PLAY\"");
}

BOOST_AUTO_TEST_CASE(mode_varaint_gen_test) {
    rtsp::headers::transport::transport_spec::parameter mode{rtsp::headers::transport::transport_spec::mode{"PLAY"}};
    std::string output;
    rtsp::headers::transport_generators<std::back_insert_iterator<std::string>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output),
                                                        gen_grammar.parameter_gen, mode);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(output, ";mode=\"PLAY\"");
}

BOOST_AUTO_TEST_CASE(transport_spec_gen_test) {
    rtsp::headers::transport::transport_spec spec1{"RTP", "AVP", boost::make_optional<rtsp::string>("UDP"), {
            rtsp::headers::transport::transport_spec::port{
                    rtsp::headers::transport::transport_spec::port::port_type::general,
                    {42}},
            rtsp::headers::transport::transport_spec::port{
                    rtsp::headers::transport::transport_spec::port::port_type::client,
                    rtsp::headers::transport::transport_spec::port_range{5, 1337}},
            rtsp::headers::transport::transport_spec::mode{"PLAY"}
    }};
    rtsp::headers::transport::transport_spec spec2{"RTP", "AVP", boost::none};
    rtsp::headers::transport transport{{spec1, spec2}};
    std::string output;
    rtsp::headers::generate_transport_header_grammar<std::back_insert_iterator<std::string>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output), gen_grammar, transport);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(output, "RTP/AVP/UDP;port=42;client_port=5-1337;mode=\"PLAY\",RTP/AVP");
}

BOOST_AUTO_TEST_SUITE_END()
