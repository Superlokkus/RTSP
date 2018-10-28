#include "../include/rtsp_server_internals.hpp"

#include <cctype>
#include <tuple>

#include <rtsp_methods.hpp>
#include <rtsp_headers.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>


rtsp::server::rtsp_server_state::rtsp_server_state() :
        methods_{
                {method{"OPTIONS"},  std::bind(&rtsp_server_state::options, this, std::placeholders::_2)},
                {method{"SETUP"},    &methods::setup},
                {method{"TEARDOWN"}, &methods::teardown},
        } {
}

auto rtsp::server::rtsp_server_state::handle_incoming_request(const request &request)
-> response {
    const auto headers = rtsp::headers::normalize_headers(request.headers);


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
        response = this->methods_.at(request.method_or_extension)
                (inserted_session, std::make_pair(request, headers)).first;
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
                response = this->methods_.at(request.method_or_extension)(session_it->second,
                                                                          std::make_pair(request, headers)).first;
                if (request.method_or_extension == "TEARDOWN") {
                    this->sessions_.erase(session_it);
                }
            }
        }
    } else if (!this->methods_.count(request.method_or_extension)) {
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

auto rtsp::server::rtsp_server_state::options(const internal_request &request) const
-> std::pair<response, body> {
    response response{rtsp_major_version, rtsp_minor_version, 200, "OK", {{"CSeq", request.second.at("cseq")}}};

    response.headers.emplace_back("Public", "SETUP, TEARDOWN, PLAY, PAUSE");
    return std::make_pair(std::move(response), "");
}


