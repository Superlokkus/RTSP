/*! @file rtp_packet.hpp
 *
 */

#ifndef RTSP_RTP_PACKET_HPP
#define RTSP_RTP_PACKET_HPP

#include <cstdint>
#include <vector>
#include <algorithm>
#include <array>

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
    std::vector<uint8_t> header_extension; //! including first 32 bits
    using payload_type = Payload;
};

/*! @brief Incomplete JPEG RTP Profile just to be usable as professors "solution"
 *
 */
struct custom_jpeg_payload_header : header<custom_jpeg_payload_header> {
    uint8_t type_specific{0};
    uint_fast32_t fragment_offset{};
    uint8_t jpeg_type{};
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

    custom_jpeg_packet_grammar() = default;

    template<typename Context, typename Skipper, typename Attribute>
    bool parse(Iterator &begin, Iterator end, const Context &, const Skipper &, Attribute &packet) const {
        if (begin == end)
            return false;
        if (begin + 3 * 4 > end)
            return false;

        namespace qi = boost::spirit::qi;

        uint8_t octet_buffer{};
        Iterator old_begin = begin;

        qi::parse(begin, end, qi::byte_, octet_buffer);

        packet.header.version = (octet_buffer & 0b11000000) >> 6;
        if (packet.header.version != 2)
            return false;
        packet.header.padding_set = (octet_buffer & 0b00100000) >> 5;
        packet.header.extension_bit = (octet_buffer & 0b00010000) >> 4;
        packet.header.csrc = octet_buffer & 0x0F;

        octet_buffer = 0;
        qi::parse(begin, end, qi::byte_, octet_buffer);

        packet.header.marker = (octet_buffer & 0b10000000) >> 7;
        packet.header.payload_type_field = (octet_buffer & 0b01111111);

        qi::parse(begin, end, qi::big_word, packet.header.sequence_number);
        qi::parse(begin, end, qi::big_dword, packet.header.timestamp);
        qi::parse(begin, end, qi::big_dword, packet.header.ssrc);

        if (begin + 4 * packet.header.csrc > end)
            return false;
        for (unsigned i = 0; i < packet.header.csrc; ++i) {
            uint32_t csrc{};
            qi::parse(begin, end, qi::big_dword, csrc);
            packet.header.csrcs.push_back(std::move(csrc));
        }
        if (packet.header.extension_bit) {
            if (begin + 4 > end)
                return false;
            packet.header.header_extension.clear();
            std::array<uint8_t, 4> header_extension_header{};
            uint16_t header_extension_length{};
            std::copy_n(begin, 4, header_extension_header.begin());
            std::advance(begin, 4);
            qi::parse(begin, end, qi::big_word, header_extension_length);
            packet.header.header_extension.reserve(4 + 4 * header_extension_length);
            std::copy(header_extension_header.begin(), header_extension_header.end(),
                      std::back_inserter(packet.header.header_extension));
            if (begin + 4 * header_extension_length > end)
                return false;
            std::copy_n(begin, 4 * header_extension_length, std::back_inserter(packet.header.header_extension));
            std::advance(begin, 4 * header_extension_length);
        }

        //JPEG profile
        if (packet.header.payload_type_field != 26)
            return false;
        if (begin + 4 * 2 > end)
            return false;
        qi::parse(begin, end, qi::byte_, packet.header.type_specific);

        std::array<uint8_t, 4> fragment_offset{};
        qi::parse(begin, end, qi::byte_, fragment_offset[1]);
        qi::parse(begin, end, qi::byte_, fragment_offset[2]);
        qi::parse(begin, end, qi::byte_, fragment_offset[3]);
        qi::parse(fragment_offset.begin(), fragment_offset.end(), qi::big_dword, packet.header.fragment_offset);

        qi::parse(begin, end, qi::byte_, packet.header.jpeg_type);
        qi::parse(begin, end, qi::byte_, packet.header.quantization_table);
        qi::parse(begin, end, qi::byte_, packet.header.image_width);
        qi::parse(begin, end, qi::byte_, packet.header.image_height);

        const auto payload_size = std::distance(begin, end);
        packet.data.reserve(payload_size);
        std::copy(begin, end, std::back_inserter(packet.data));
        std::advance(begin, payload_size);

        return true;
    }

    template<typename Context, typename Iterator_>
    struct attribute {
        typedef custom_jpeg_packet type;
    };


private:

};

template<typename OutputIterator>
struct custom_jpeg_packet_generator {
    struct generator_id;
    typedef boost::mpl::int_<boost::spirit::karma::generator_properties::no_properties> properties;
    template<typename Context>
    struct attribute {
        typedef custom_jpeg_packet type;
    };

    template<typename Sink, typename Context, typename Delimiter, typename Attribute>
    bool generate(Sink &sink, Context &, Delimiter const &delim, Attribute const &packet) const {
        namespace karma = boost::spirit::karma;

        if (packet.header.version != 2u)
            return false;
        uint8_t octet_buffer{};
        octet_buffer |= packet.header.version << 6;
        octet_buffer |= packet.header.padding_set << 5;
        octet_buffer |= packet.header.extension_bit << 4;
        octet_buffer |= packet.header.csrc;
        karma::generate(sink, karma::byte_, octet_buffer);

        octet_buffer = 0u;
        octet_buffer |= packet.header.marker << 7;
        octet_buffer |= packet.header.payload_type_field;
        karma::generate(sink, karma::byte_, octet_buffer);

        karma::generate(sink, karma::big_word, packet.header.sequence_number);
        karma::generate(sink, karma::big_dword, packet.header.timestamp);
        karma::generate(sink, karma::big_dword, packet.header.ssrc);

        if (packet.header.csrc != packet.header.csrcs.size())
            return false;
        for (const auto &csrc : packet.header.csrcs)
            karma::generate(sink, karma::big_dword, csrc);

        karma::generate(sink, karma::byte_, packet.header.type_specific);
        std::array<uint8_t, 4> fragment_offset{};
        karma::generate(fragment_offset.begin(), karma::big_dword, packet.header.fragment_offset);
        karma::generate(sink, karma::byte_, fragment_offset[1]);
        karma::generate(sink, karma::byte_, fragment_offset[2]);
        karma::generate(sink, karma::byte_, fragment_offset[3]);

        karma::generate(sink, karma::byte_, packet.header.jpeg_type);
        karma::generate(sink, karma::byte_, packet.header.quantization_table);
        karma::generate(sink, karma::byte_, packet.header.image_width);
        karma::generate(sink, karma::byte_, packet.header.image_height);

        for (auto octet : packet.data)
            karma::generate(sink, karma::byte_, octet);

        return true;
    }
};


}
}

#endif //RTSP_RTP_PACKET_HPP
