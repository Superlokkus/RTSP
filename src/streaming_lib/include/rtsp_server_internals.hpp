/*!
 * @file rtsp_server_internals.hpp
 */

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <cctype>
#include <functional>

#include <rtsp_message.hpp>
#include <rtsp_server_.hpp>
#include <rtsp_session.hpp>
#include <rtsp_methods.hpp>

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
 *  Shared Objectts: Safe for @ref handle_incoming_request method and static member functions
 */
class rtsp_server_state final {
public:
    explicit rtsp_server_state() :
            methods_{
                    {method_t{"OPTIONS"}, std::bind(&rtsp_server_state::options, this, std::placeholders::_2)},
            } {
    }

    const uint_fast16_t rtsp_major_version = 1;
    const uint_fast16_t rtsp_minor_version = 0;


    response handle_incoming_request(const request &request) {
        const auto headers = harmonize_headers(request.headers);

        //TODO remove this debug output with proper logging
        std::cout << "Got request " << request.method_or_extension << " " << request.uri << "\n";
        for (const auto &header: headers) {
            std::cout << header.first << ": " << header.second << "\n";
        }
        std::cout << std::endl;

        response response{rtsp_major_version, rtsp_minor_version, 500, "Method not called"};

        if (!headers.count("session") &&
            !(request.method_or_extension == "SETUP" || request.method_or_extension == "OPTIONS")) {
            response.status_code = 454;
            response.reason_phrase = "Session not found";
        }

        if (this->methods_.count(request.method_or_extension)) {
            std::lock_guard<std::mutex> lock{this->sessions_mutex_};
            /*auto& session = this->sessions_.find()
            this->methods.at(request.method_or_extension)()*/
        } else {

        }

        return response;
    }

    using internal_request_t = std::pair<request, headers_t>;
    using method_implementation = std::function<
            std::pair<headers_t, body>(rtsp::rtsp_session &, const internal_request_t &)
    >;

    std::pair<headers_t, body> options(const internal_request_t &request) const {
        headers_t headers{};

        return std::make_pair<headers_t, body>(std::move(headers), "");
    }

    /*! @brief RFC 2616  4.2 compliant harmonization of headers
     *
     * @param raw_headers Headers as parsed in
     * @return Harmonized headers
     */
    static headers_t harmonize_headers(const raw_headers &raw_headers) {
        headers_t new_headers{};
        for (const auto &header : raw_headers) {
            auto header_slot = new_headers.find(boost::algorithm::to_lower_copy(header.first));
            if (header_slot == new_headers.end()) {
                new_headers.emplace(header);
            } else {
                header_slot->second += string_t{","} + header.second;
            }
        }
        return new_headers;
    }

private:
    std::mutex sessions_mutex_;
    std::unordered_set<::rtsp::rtsp_session> sessions_;

    std::unordered_map<rtsp::method_t, method_implementation> methods_;

};

}
}
