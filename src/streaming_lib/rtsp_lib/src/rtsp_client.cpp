/*! @file rtsp_client.cpp
 *
 */

#include <rtsp_client.hpp>

rtsp::rtsp_client::rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                               std::function<void(std::exception &)> error_handler,
                               std::function<void(const std::string &)> log_handler)
        :
        frame_handler_(std::move(frame_handler)),
        error_handler_(std::move(error_handler)),
        log_handler_(std::move(log_handler)) {
    this->log_handler_(std::string{"rtsp_client created for URL: "} + url);

}

rtsp::rtsp_client::~rtsp_client() {
    this->log_handler_("rtsp_client destroyed");
}


void rtsp::rtsp_client::set_rtp_packet_stat_callbacks(
        std::function<void(packet_count_t, packet_count_t)> received_packet_handler,
        std::function<void(packet_count_t, packet_count_t)> fec_packet_handler) {

}

void rtsp::rtsp_client::setup() {

}

void rtsp::rtsp_client::play() {

}

void rtsp::rtsp_client::pause() {

}

void rtsp::rtsp_client::teardown() {

}

void rtsp::rtsp_client::option() {

}

void rtsp::rtsp_client::describe() {

}
