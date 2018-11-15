/*! @file rtp_packet.hpp
 *
 */

#ifndef RTSP_RTP_PACKET_HPP
#define RTSP_RTP_PACKET_HPP

#include <cstdint>
#include <vector>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

namespace rtp {
namespace packet {

template<typename Payload>
struct header {
    uint_fast8_t version{2u};
    bool padding_set{false};
    bool extension_bit{false};
    uint_fast8_t csrc{};
    bool marker{false};
    uint_fast8_t payload_type_field{0};
    uint16_t sequence_number{0};
    uint32_t timestamp{};
    uint32_t ssrc{};
    std::vector<uint32_t> csrcs;
    using payload_type = Payload;
};

/*! @brief Incomplete JPEG RTP Profile just to be usable as professors "solution"
 *
 */
struct custom_jpeg_payload_header : header<custom_jpeg_payload_header> {
    uint8_t type_specific{0};
    uint_fast32_t fragment_offset{};
    uint8_t jpeg_type;
    uint8_t quantization_table{};
    uint8_t image_width{};
    uint8_t image_height{};
};

struct custom_jpeg_packet {
    custom_jpeg_payload_header header;
    std::vector<uint8_t> data;
};

namespace ns = ::boost::spirit::standard;


/*! @brief Follows the Boost parser concept
 *
 * @tparam Iterator Forward Iterator
 */
template<typename Iterator>
struct custom_jpeg_packet_grammar {
    struct parser_id;

    custom_jpeg_packet_grammar() {
        using ::boost::spirit::qi::uint_parser;
        using ::boost::spirit::qi::lexeme;
        using ::boost::spirit::qi::lit;
        using boost::spirit::qi::omit;
        using ::boost::spirit::qi::repeat;
        using boost::spirit::qi::big_word;//atleast16
        using boost::spirit::qi::big_dword;//atleast32

        std::bitset<8> header_first_octet{0};


        //start %= boost::spirit::qi::byte_[header_first_octet = read_out_first_octet(boost::spirit::_1)];
    }

    template<typename Context, typename Skipper, typename Attribute>
    bool parse(Iterator &begin, Iterator end, const Context &, const Skipper &, Attribute &packet) {
        return {};
    }

    template<typename Context, typename Iterator_>
    struct attribute {
        typedef custom_jpeg_packet type;
    };


private:

};


}
}

#endif //RTSP_RTP_PACKET_HPP
