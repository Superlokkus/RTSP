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
        file.unsetf(std::ios::skipws);
        std::istream_iterator<uint8_t> begin{file}, end{};
        std::copy(begin, end, std::back_inserter(this->complete_voigt_packet));
        BOOST_REQUIRE_EQUAL(this->complete_voigt_packet.at(6), 0x09);
        BOOST_REQUIRE_EQUAL(this->complete_voigt_packet.at(7), 0xB0);
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
    BOOST_CHECK_EQUAL(packet.header.payload_type_field, 26);
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

BOOST_AUTO_TEST_CASE(RTP_default_header_generation) {
    rtp::packet::custom_jpeg_packet packet{};
    std::vector<uint8_t> output;
    rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output), gen_grammar, packet);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(output.at(0), 0x80u);
    for (int i = 1; i < 3 + 4 * 4; ++i) {
        BOOST_CHECK_EQUAL(output.at(i), 0x00u);
    }

}

BOOST_AUTO_TEST_CASE(RTP_likely_header_generation) {
    rtp::packet::custom_jpeg_packet packet{};
    packet.header.csrc = 3u;
    packet.header.marker = true;
    packet.header.payload_type_field = 26u;
    packet.header.sequence_number = 1337u;
    packet.header.timestamp = 1337u * 1337u;
    packet.header.ssrc = 0x1C4A7EB4;
    packet.header.csrcs.push_back(56u * 1337u);
    packet.header.csrcs.push_back(0u);
    packet.header.csrcs.push_back(42u * 56u);
    packet.header.type_specific = 0x91u;
    packet.header.fragment_offset = 0xF0FFFBu;
    packet.header.jpeg_type = 11u;
    packet.header.quantization_table = 13u;
    packet.header.image_width = 17u;
    packet.header.image_height = 19u;
    packet.data = std::vector<uint8_t>(32, 0x0Eu);

    std::vector<uint8_t> output;
    rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
    const bool success = boost::spirit::karma::generate(std::back_inserter(output), gen_grammar, packet);
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(output.at(0), 0x83u);
    BOOST_CHECK_EQUAL(output.at(1), 0x9Au);
    BOOST_CHECK_EQUAL(output.at(2), 0x05u);
    BOOST_CHECK_EQUAL(output.at(3), 0x39u);

    BOOST_CHECK_EQUAL(output.at(0 + 4), 0x00u);
    BOOST_CHECK_EQUAL(output.at(1 + 4), 0x1Bu);
    BOOST_CHECK_EQUAL(output.at(2 + 4), 0x46u);
    BOOST_CHECK_EQUAL(output.at(3 + 4), 0xB1u);

    BOOST_CHECK_EQUAL(output.at(0 + 4 * 2), 0x1Cu);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 2), 0x4Au);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 2), 0x7Eu);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 2), 0xB4u);

    //CSRCS
    BOOST_CHECK_EQUAL(output.at(0 + 4 * 3), 0x00u);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 3), 0x01u);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 3), 0x24u);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 3), 0x78u);

    BOOST_CHECK_EQUAL(output.at(0 + 4 * 4), 0x00u);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 4), 0x00u);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 4), 0x00u);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 4), 0x00u);

    BOOST_CHECK_EQUAL(output.at(0 + 4 * 5), 0x00u);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 5), 0x00u);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 5), 0x09u);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 5), 0x30u);

    //JPEG
    BOOST_CHECK_EQUAL(output.at(0 + 4 * 6), 0x91u);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 6), 0xF0u);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 6), 0xFFu);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 6), 0xFBu);

    BOOST_CHECK_EQUAL(output.at(0 + 4 * 7), 11u);
    BOOST_CHECK_EQUAL(output.at(1 + 4 * 7), 13u);
    BOOST_CHECK_EQUAL(output.at(2 + 4 * 7), 17u);
    BOOST_CHECK_EQUAL(output.at(3 + 4 * 7), 19u);

    for (int i = 0; i < 32; ++i) {
        BOOST_CHECK_EQUAL(output.at(i + 4 * 8), 0x0Eu);
    }

}

BOOST_AUTO_TEST_SUITE_END()
