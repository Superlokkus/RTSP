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
#include <cstdint>

#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

namespace rtsp {
namespace fileapi = boost::filesystem;

using char_t = char;
using string = std::basic_string<char_t>;
using method = string;
using request_uri = string;
using header = std::pair<string, string>;
using raw_headers = std::vector<header>;

using cseq = uint32_t;

using normalized_headers = std::unordered_map<rtsp::header::first_type, rtsp::header::second_type>;
using body = string;

const std::unordered_set<method> rtsp_methods{
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
    method method_or_extension;
    request_uri uri;
    uint_fast16_t rtsp_version_major;
    uint_fast16_t rtsp_version_minor;
    raw_headers headers;
};

using internal_request = std::pair<request, normalized_headers>;

struct response {
    uint_fast16_t rtsp_version_major;
    uint_fast16_t rtsp_version_minor;
    uint_fast16_t status_code;
    string reason_phrase;
    raw_headers headers;
};
using message = boost::variant<request, response>;
}

#endif //RTSP_RTSP_DEFINITIONS_HPP
