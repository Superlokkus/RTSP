#define BOOST_TEST_MODULE rtp_endsystem_test

#include <boost/test/included/unit_test.hpp>

#include <fstream>
#include <iterator>
#include <algorithm>

#include <rtp_packet.hpp>


BOOST_AUTO_TEST_SUITE(rtp_endsystem)

struct rtp_packet_fixture {
    std::vector<uint8_t> first_3_32_bits_voigt_packet
            {0x80, 0x80, 0x00, 0x3E, 0x00, 0x00, 0x09, 0xB0, 0x00, 0x00, 0x00, 0x00};
    std::vector<uint8_t> complete_voigt_packet;

    std::vector<uint8_t>::const_iterator begin{};
    std::vector<uint8_t>::const_iterator end{};
    bool success{false};
    rtp::packet::custom_jpeg_packet packet{};

    rtp_packet_fixture() {
        std::ifstream file{"rtp_vogel_example_packet.bin", std::ios::binary};
        BOOST_REQUIRE(file.is_open());
        std::istream_iterator<uint8_t> begin{file}, end{};
        std::copy(begin, end, std::back_inserter(this->complete_voigt_packet));
    }

    void parse_phrase(const std::vector<uint8_t> &phrase) {
        rtp::packet::custom_jpeg_packet_grammar<std::vector<uint8_t>::const_iterator> grammar{};
        begin = phrase.cbegin();
        end = phrase.cend();
        success = boost::spirit::qi::parse(begin, end, grammar, packet);
    }

};

BOOST_FIXTURE_TEST_SUITE(rtp_packet_tests, rtp_packet_fixture)

BOOST_AUTO_TEST_CASE(RTP_Header_Voigt) {
    packet.header.version = 0;
    parse_phrase(complete_voigt_packet);

    BOOST_CHECK_EQUAL(packet.header.version, 2);
    BOOST_CHECK_EQUAL(packet.header.padding_set, false);
    BOOST_CHECK_EQUAL(packet.header.extension_bit, false);
    BOOST_CHECK_EQUAL(packet.header.csrc, 0);
    BOOST_CHECK_EQUAL(packet.header.marker, true);
    //BOOST_CHECK_EQUAL(packet.header.payload_type_field, 26);
    BOOST_CHECK_EQUAL(packet.header.sequence_number, 62);
    BOOST_CHECK_EQUAL(packet.header.timestamp, 2480);
    BOOST_CHECK_EQUAL(packet.header.ssrc, 0);
}

BOOST_AUTO_TEST_CASE(RTP_Voigt_JPEG_custom_style) {
    packet.header.version = 0;
    parse_phrase(complete_voigt_packet);

    BOOST_CHECK(success);
    BOOST_CHECK(begin == end);
}

BOOST_AUTO_TEST_SUITE_END() //rtp_packet_tests

BOOST_AUTO_TEST_SUITE_END()
