#define BOOST_TEST_MODULE rtp_endsystem_test

#include <boost/test/included/unit_test.hpp>

#include <rtp_packet.hpp>

BOOST_AUTO_TEST_SUITE(rtp_endsystem)

BOOST_AUTO_TEST_CASE(FIRST_TEST) {
    struct rtp::packet::custom_jpeg_packet_grammar<std::vector<uint8_t>::const_iterator> grammar{};
    std::vector<uint8_t> phrase{0x42};
    auto begin = phrase.cbegin();
    auto end = phrase.cend();
    rtp::packet::custom_jpeg_packet packet{};
    auto success = boost::spirit::qi::parse(begin, end, grammar, packet);

    BOOST_CHECK(success);
}

BOOST_AUTO_TEST_SUITE_END()