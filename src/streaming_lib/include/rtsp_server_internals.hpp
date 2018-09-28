/*!
 * @file rtsp_server_internals.hpp
 */

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <cctype>

#include <rtsp_message.hpp>
#include <rtsp_server_.hpp>
#include <rtsp_session.hpp>

#include <boost/algorithm/string/case_conv.hpp>

namespace rtsp {
namespace server {

/*! @brief This class handles the rtsp dialog from a server perspective, on the level of parsed rtsp messages
 *  while maintaining the necessary state and side-effects in a thread safe manner
 *
 *  This class will likely be a bottle neck but is not optimized yet, for demonstration purposes
 *  Likely good solution would be hashing sessions to specific threads, and let reponses be given by via
 *  a callback or future
 *
 *  @par Thread Safety
 *  Distinct Objects: Safe
 *  Shared Objectts: Safe for @ref handle_incoming_request method
 */
class rtsp_server_state final {
public:
    explicit rtsp_server_state() = default;

    const uint_fast16_t rtsp_major_version = 1;
    const uint_fast16_t rtsp_minor_version = 0;


    response handle_incoming_request(const request &request) {
        const auto headers = harmonize_headers(request.headers);
        for (const auto &header: headers) {
            std::cout << header.first << ": " << header.second << std::endl;
        }

        return response{rtsp_major_version, rtsp_minor_version, 500, "Work in progress"};
    }

private:
    std::mutex sessions_mutex_;
    std::unordered_set<::rtsp::rtsp_session> sessions_;

    using harmonized_headers = std::unordered_map<header::first_type, header::second_type>;

    /*! @brief RFC 2616  4.2 compliant harmonization of headers
     *
     * @param raw_headers Headers as parsed in
     * @return Harmonized headers
     */
    static harmonized_headers harmonize_headers(const raw_headers &raw_headers) {
        harmonized_headers new_headers{};
        for (const auto &header : raw_headers) {
            auto header_slot = new_headers.find(boost::algorithm::to_lower_copy(header.first));
            if (header_slot == new_headers.end()) {
                new_headers.emplace(header);
            } else {
                header_slot->second += string{","} + header.second;
            }
        }
        return new_headers;
    }
};

}
}
