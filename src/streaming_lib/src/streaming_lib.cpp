/*! @file streaming_lib.cpp
 *
 */

#include <streaming_lib.hpp>

#include <rtsp_server_.hpp>

rtsp::rtsp_server::rtsp_server(std::string video_file_directory, uint16_t udp_port,
                               std::function<void(std::exception &)> error_handler)
        : rtsp_server_{std::make_unique<rtsp::rtsp_server_>(video_file_directory, udp_port, std::move(error_handler))} {

}

rtsp::rtsp_server::~rtsp_server() {

}
