/*! @file rtsp_headers.hpp
 * @brief Declarations for complex rtsp headers, which need normalized headers as input
 * @notice This file may not contain functionality rtsp headers, like CSeq or Session
 */

#ifndef RTSP_RTSP_HEADERS_HPP
#define RTSP_RTSP_HEADERS_HPP

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>


#include <rtsp_definitions.hpp>

namespace rtsp {
namespace headers {

/*! @brief RFC 2616  4.2 compliant harmonization of headers
 *
 * @param raw_headers Headers as parsed in
 * @return Harmonized headers
 */
normalized_headers normalize_headers(const raw_headers &raw_headers);

struct transport{
    struct transport_spec {
        using ttl = uint_fast16_t;
        using port_number = uint_fast32_t;
        using ssrc = uint32_t;
        using port_range = std::pair<port_number, port_number>;
        using port = boost::variant<port_number, port_range>;
        using parameter = boost::variant<string, ttl>;
        string transport_protocol;
        string profile;
        boost::optional<string> lower_transport;
        std::vector<parameter> parameters;
    };

    std::vector<transport_spec> specifications;
};

}
}

#endif //RTSP_RTSP_HEADERS_HPP
