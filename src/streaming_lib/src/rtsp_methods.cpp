#include <rtsp_methods.hpp>

#include <rtsp_headers.hpp>

#include <boost/uuid/uuid_io.hpp>

rtsp::response rtsp::methods::common_response_sekeleton(const rtsp::rtsp_session &session,
                                                        const rtsp::internal_request &request) {
    return rtsp::response{common_rtsp_major_version, common_rtsp_minor_version,
                          200, "OK", {
                                  {"CSeq", request.second.at("cseq")},
                                  {"Session", boost::uuids::to_string(session.identifier())},
                          }};
}

std::pair<rtsp::response, rtsp::body> rtsp::methods::setup(rtsp::rtsp_session &session,
                                                           const rtsp::internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};

    rtsp::headers::transport request_transport{};


    if (request.second.count("transport")) {
        rtsp::headers::transport_grammar<std::string::const_iterator> transport_grammar{};
        auto begin = request.second.at("transport").cbegin();
        auto end = request.second.at("transport").cend();

        boost::spirit::qi::phrase_parse(begin, end, transport_grammar, boost::spirit::ascii::space, request_transport);

        if (begin != end) {
            response.status_code = 400;
            response.reason_phrase = rtsp::string{"Bad Request: Could not read transport header after"} +
                                     rtsp::string{begin, end};
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }
    }

    rtsp::headers::transport response_transport{};

    rtsp::string transport_string;
    rtsp::headers::generate_transport_header_grammar<std::back_insert_iterator<std::string>> transport_gen_grammar{};
    boost::spirit::karma::generate(std::back_inserter(transport_string), transport_gen_grammar, response_transport);
    response.headers.emplace_back("Transport", std::move(transport_string));

    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}

std::pair<rtsp::response, rtsp::body> rtsp::methods::teardown(rtsp::rtsp_session &session,
                                                              const rtsp::internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};


    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}
