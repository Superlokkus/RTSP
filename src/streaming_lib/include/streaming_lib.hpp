/*! @file streaming_lib.hpp
 * Convenience header for all user needed headers and also declares symbols for dynamic linking usage
 */

#ifndef STREAMING_LIB_HPP
#define STREAMING_LIB_HPP

#include <boost/dll.hpp>

#include <rtsp_server_pimpl.hpp>

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace streaming_lib {

API rtsp::rtsp_server_pimpl *start_rtsp_server(const char *video_file_directory, unsigned port) noexcept;
API void stop_rtsp_server(rtsp::rtsp_server_pimpl *server) noexcept;
}

#endif //STREAMING_LIB_HPP
