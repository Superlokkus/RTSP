/*! @file rtsp_definitions.hpp
 * Defines common used definitions in the RTSP context
 */

#ifndef RTSP_RTSP_DEFINITIONS_HPP
#define RTSP_RTSP_DEFINITIONS_HPP

#include <string>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <boost/variant.hpp>

namespace rtsp {
using char_t = char;
using string_t = std::basic_string<char_t>;
using method_t = string_t;
using request_uri = string_t;
using header = std::pair<string_t, string_t>;
using raw_headers = std::vector<header>;

using headers_t = std::unordered_map<rtsp::header::first_type, rtsp::header::second_type>;
using body = string_t;

const std::unordered_set<method_t> rtsp_methods{
        "DESCRIBE",
        "ANNOUNCE",
        "GET_PARAMETER",
        "OPTIONS",
        "PAUSE",
        "PLAY",
        "RECORD",
        "REDIRECT",
        "SETUP",
        "SET_PARAMETER",
        "TEARDOWN",
};

struct request {
    method_t method_or_extension;
    request_uri uri;
    uint_fast16_t rtsp_version_major;
    uint_fast16_t rtsp_version_minor;
    raw_headers headers;
};
struct response {
    uint_fast16_t rtsp_version_major;
    uint_fast16_t rtsp_version_minor;
    uint_fast16_t status_code;
    string_t reason_phrase;
    raw_headers headers;
};
using message = boost::variant<request, response>;
}

#endif //RTSP_RTSP_DEFINITIONS_HPP
