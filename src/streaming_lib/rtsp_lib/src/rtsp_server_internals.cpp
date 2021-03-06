#include <rtsp_server_internals.hpp>

#include <cctype>
#include <tuple>

#include <rtsp_methods.hpp>
#include <rtsp_headers.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>


rtsp::server::rtsp_server_state::rtsp_server_state(boost::asio::io_context &io_context,
                                                   fileapi::path resource_root) :
        ressource_root_(std::move(resource_root)),
        methods_{
                {method{"OPTIONS"},  std::bind(&rtsp_server_state::options, this, std::placeholders::_2)},
                {method{"SETUP"},    std::bind(&methods::setup,
                                               std::placeholders::_1, std::placeholders::_2, ressource_root_,
                                               std::ref(io_context))},
                {method{"TEARDOWN"}, &methods::teardown},
                {method{"PLAY"},     &methods::play},
                {method{"PAUSE"},    &methods::pause},
        } {
}

auto rtsp::server::rtsp_server_state::handle_incoming_request(const request &request,
                                                              const boost::asio::ip::address &requester)
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
    } else if (request.method_or_extension == "DESCRIBE") {
        response.status_code = 501;
        response.reason_phrase = std::string{"\""} + request.method_or_extension + "\" not implemented";
        response.headers.emplace_back("CSeq", headers.at("cseq"));
    } else if (request.method_or_extension == "SETUP" && headers.count("session")) {
        response.status_code = 459;
        response.reason_phrase = "Aggregate Operation Not Allowed";
        response.headers.emplace_back("CSeq", headers.at("cseq"));
    } else if (request.method_or_extension == "SETUP") {
        rtsp_server_session new_session{};
        auto identifier = new_session.identifier();
        std::lock_guard<std::mutex> lock{this->sessions_mutex_};
        rtsp_server_session &inserted_session = this->sessions_.emplace
                (std::move(identifier), std::move(new_session)).first->second;
        inserted_session.last_seen_request_address = requester;
        this->sessions__by_host_.emplace(requester, inserted_session.identifier());
        response = this->methods_.at(request.method_or_extension)
                (inserted_session, std::make_pair(request, headers)).first;
    } else if (!headers.count("session")) {
        response.status_code = 400;
        response.reason_phrase = "Session header not found";
        response.headers.emplace_back("CSeq", headers.at("cseq"));
    } else if (this->methods_.count(request.method_or_extension)) {
        std::lock_guard<std::mutex> lock{this->sessions_mutex_};
        auto session_it = this->sessions_.find(headers.at("session"));
        if (session_it == this->sessions_.end()) {
            response.status_code = 454;
            response.reason_phrase = "Session not found";
            response.headers.emplace_back("CSeq", headers.at("cseq"));
        } else {
            session_it->second.last_seen_request_address = requester;
            response = this->methods_.at(request.method_or_extension)(session_it->second,
                                                                      std::make_pair(request, headers)).first;
            if (request.method_or_extension == "TEARDOWN") {
                this->delete_session(session_it);
            }
        }

    } else if (!this->methods_.count(request.method_or_extension)) {
        response.status_code = 501;
        response.reason_phrase = std::string{"\""} + request.method_or_extension + "\" not implemented";
        response.headers.emplace_back("CSeq", headers.at("cseq"));
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

void rtsp::server::rtsp_server_state::handle_timeout_of_host(const boost::asio::ip::address &host) {
    BOOST_LOG_TRIVIAL(info) << "Host: " << host << " timed out";
    std::lock_guard<std::mutex> lock{this->sessions_mutex_};
    for (auto host_session_it = this->sessions__by_host_.find(host);
         host_session_it != this->sessions__by_host_.end();
         host_session_it = this->sessions__by_host_.find(host)) {
        auto session_it = this->sessions_.find(host_session_it->second);
        if (session_it != this->sessions_.end())
            this->delete_session(session_it);
        this->sessions__by_host_.erase(host_session_it);
    }
}

void
rtsp::server::rtsp_server_state::delete_session(std::unordered_map<rtsp::string, rtsp::rtsp_server_session>
                                                ::iterator &session_it) {
    this->sessions_.erase(session_it);
}
