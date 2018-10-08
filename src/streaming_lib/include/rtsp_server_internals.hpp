/*!
 * @file rtsp_server_internals.hpp
 */

#ifndef STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP
#define STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <cctype>
#include <functional>
#include <tuple>

#include <rtsp_message.hpp>
#include <rtsp_session.hpp>
#include <rtsp_methods.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>

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
                    {method{"OPTIONS"}, std::bind(&rtsp_server_state::options, this, std::placeholders::_2)},
            } {
    }

    const uint_fast16_t rtsp_major_version = 1;
    const uint_fast16_t rtsp_minor_version = 0;


    response handle_incoming_request(const request &request) {
        const auto headers = harmonize_headers(request.headers);


        BOOST_LOG_TRIVIAL(info) << request.method_or_extension << " " << request.uri;
        for (const auto &header: headers) {
            BOOST_LOG_TRIVIAL(info) << header.first << ": " << header.second;
        }
        BOOST_LOG_TRIVIAL(info) << "\n";

        response response{rtsp_major_version, rtsp_minor_version, 500, "Internal error: Response unchanged"};

        if (!headers.count("cseq") || headers.at("cseq").size() == 0) {
            response.status_code = 400;
            response.reason_phrase = "Bad Request: CSeq missing";
        } else if (request.method_or_extension == "OPTIONS") {
            body body{};
            std::tie(response, body) = options(std::make_pair(request, headers));
        } else if (request.method_or_extension == "SETUP") {
            rtsp_session new_session{};
            auto identifier = new_session.identifier();
            std::lock_guard<std::mutex> lock{this->sessions_mutex_};
            rtsp_session &inserted_session = this->sessions_.emplace
                    (std::move(identifier), std::move(new_session)).first->second;
            this->methods_.at(request.method_or_extension)(inserted_session, std::make_pair(request, headers));
        } else if (!headers.count("session")) {
            response.status_code = 400;
            response.reason_phrase = "Session header not found";
        } else if (this->methods_.count(request.method_or_extension)) {
            session_identifier session_id{};
            auto converted = boost::conversion::try_lexical_convert<boost::uuids::uuid &, const std::string &>
                    (headers.at("session"), session_id);
            if (!converted || session_id.is_nil()) {
                response.status_code = 454;
                response.reason_phrase = "Session-id malformend";
            } else {
                std::lock_guard<std::mutex> lock{this->sessions_mutex_};
                auto session_it = this->sessions_.find(session_id);
                if (session_it == this->sessions_.end()) {
                    response.status_code = 454;
                    response.reason_phrase = "Session not found";
                } else {
                    this->methods_.at(request.method_or_extension)(session_it->second,
                                                                   std::make_pair(request, headers));
                }
            }
        } else {
            response.status_code = 501;
            response.reason_phrase = std::string{"\""} + request.method_or_extension + "\" not implemented";
        }

        BOOST_LOG_TRIVIAL(info) << response.status_code << " \"" << response.reason_phrase << "\"";
        for (const auto &header: response.headers) {
            BOOST_LOG_TRIVIAL(info) << header.first << ": " << header.second;
        }
        BOOST_LOG_TRIVIAL(info) << "\n";

        return response;
    }

    using internal_request = std::pair<request, headers>;
    using method_implementation = std::function<
            std::pair<response, body>(rtsp::rtsp_session &, const internal_request &)
    >;

    std::pair<response, body> options(const internal_request &request) const {
        response response{rtsp_major_version, rtsp_minor_version, 200, "OK", {{"CSeq", request.second.at("cseq")}}};

        response.headers.emplace_back("Public", "SETUP, TEARDOWN, PLAY, PAUSE");
        return std::make_pair(std::move(response), "");
    }

    /*! @brief RFC 2616  4.2 compliant harmonization of headers
     *
     * @param raw_headers Headers as parsed in
     * @return Harmonized headers
     */
    static headers harmonize_headers(const raw_headers &raw_headers) {
        headers new_headers{};
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

private:
    std::mutex sessions_mutex_;
    std::unordered_map<rtsp::session_identifier, rtsp::rtsp_session> sessions_;
    std::unordered_map<rtsp::method, method_implementation> methods_;

};

}
}

#endif //STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP
