/*! @file rtsp_methods.hpp
 * Defines the built in rtsp methods
 */

#ifndef RTSP_RTSP_METHODS_HPP
#define RTSP_RTSP_METHODS_HPP

#include <rtsp_definitions.hpp>
#include <rtsp_session.hpp>

namespace rtsp {
namespace methods {

std::pair<response, body> setup(rtsp::rtsp_session &, const rtsp::request &);

std::pair<response, body> play(rtsp::rtsp_session &, const rtsp::request &);

std::pair<response, body> teardown(rtsp::rtsp_session &, const rtsp::request &);

std::pair<response, body> describe(rtsp::rtsp_session &, const rtsp::request &);

std::pair<response, body> pause(rtsp::rtsp_session &, const rtsp::request &);

}
}

#endif //RTSP_RTSP_METHODS_HPP
