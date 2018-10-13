#include <rtsp_headers.hpp>

#include <boost/algorithm/string/case_conv.hpp>

rtsp::normalized_headers rtsp::headers::normalize_headers(const rtsp::raw_headers &raw_headers) {
    normalized_headers new_headers{};
    for (const auto &header : raw_headers) {
        auto harmonized_key = boost::algorithm::to_lower_copy(header.first);
        auto header_slot = new_headers.find(harmonized_key);
        if (header_slot == new_headers.end()) {
            new_headers.emplace(harmonized_key, header.second);
        } else {
            header_slot->second += string{","} + header.second;
        }
    }
    return new_headers;
}
