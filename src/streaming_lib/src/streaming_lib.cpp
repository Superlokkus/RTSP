/*! @file streaming_lib.cpp
 *
 */

#include <streaming_lib.hpp>


rtsp::rtsp_server_pimpl *streaming_lib::start_rtsp_server(const char *video_file_directory, unsigned port) noexcept {
    rtsp::rtsp_server_pimpl *return_value{nullptr};

    try {
        return_value = new rtsp::rtsp_server_pimpl(video_file_directory, port, [](auto) {});
    } catch (...) {
        delete (return_value);
        return_value = nullptr;
    }

    return return_value;
}

void streaming_lib::stop_rtsp_server(rtsp::rtsp_server_pimpl *server) noexcept {
    delete (server);
}
