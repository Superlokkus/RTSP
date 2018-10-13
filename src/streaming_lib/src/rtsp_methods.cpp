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


    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}

std::pair<rtsp::response, rtsp::body> rtsp::methods::teardown(rtsp::rtsp_session &session,
                                                              const rtsp::internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};


    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}
