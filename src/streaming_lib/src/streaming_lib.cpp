/*! @file streaming_lib.cpp
 *
 */

#include <streaming_lib.hpp>

#include <rtsp_server_.hpp>

rtsp::rtsp_server::rtsp_server(std::string video_file_directory, uint16_t port,
                               std::function<void(std::exception &)> error_handler)
        : rtsp_server_{std::make_unique<rtsp::rtsp_server_>(video_file_directory, port, std::move(error_handler))} {

}

rtsp::rtsp_server::~rtsp_server() {

}

void rtsp::rtsp_server::graceful_shutdown() {
    this->rtsp_server_->graceful_shutdown();
}

