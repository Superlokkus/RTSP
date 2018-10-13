/*! @file rtsp_methods.hpp
 * Defines the built in rtsp methods
 */

#ifndef RTSP_RTSP_METHODS_HPP
#define RTSP_RTSP_METHODS_HPP

#include <rtsp_definitions.hpp>
#include <rtsp_session.hpp>

namespace rtsp {
namespace methods {

const uint_fast16_t common_rtsp_major_version = 1;
const uint_fast16_t common_rtsp_minor_version = 0;

response common_response_sekeleton(const rtsp::rtsp_session &, const rtsp::internal_request &);

std::pair<response, body> setup(rtsp::rtsp_session &, const rtsp::internal_request &);

std::pair<response, body> play(rtsp::rtsp_session &, const rtsp::internal_request &);

std::pair<response, body> teardown(rtsp::rtsp_session &, const rtsp::internal_request &);

std::pair<response, body> pause(rtsp::rtsp_session &, const rtsp::internal_request &);

}
}

#endif //RTSP_RTSP_METHODS_HPP
