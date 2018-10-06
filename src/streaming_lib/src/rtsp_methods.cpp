#include <rtsp_methods.hpp>


std::pair<rtsp::headers_t, rtsp::body> rtsp::methods::setup(rtsp::rtsp_session &, const rtsp::request &) {
    return std::pair<rtsp::headers_t, rtsp::body>();
}
