/*! @file rtsp_methods.hpp
 * Defines the built in rtsp methods
 */

#ifndef RTSP_RTSP_METHODS_HPP
#define RTSP_RTSP_METHODS_HPP

#include <rtsp_definitions.hpp>
#include <rtsp_session.hpp>

namespace rtsp {
namespace methods {

std::pair<headers, body> setup(rtsp::rtsp_session &, const rtsp::request &);

std::pair<headers, body> play(rtsp::rtsp_session &, const rtsp::request &);

std::pair<headers, body> teardown(rtsp::rtsp_session &, const rtsp::request &);

std::pair<headers, body> describe(rtsp::rtsp_session &, const rtsp::request &);

std::pair<headers, body> pause(rtsp::rtsp_session &, const rtsp::request &);

}
}

#endif //RTSP_RTSP_METHODS_HPP
