/*! @file rtsp_client_pimpl.cpp
 *
 */

#include <rtsp_client_pimpl.hpp>

#include <rtsp_client.hpp>

rtsp::rtsp_client_pimpl::rtsp_client_pimpl(std::string url, std::function<void(jpeg_frame)> frame_handler,
                                           std::function<void(std::exception &)> error_handler,
                                           std::function<void(const std::string &)> log_handler) :
        rtsp_client_(std::make_unique<rtsp_client>(
                std::move(url),
                std::move(frame_handler),
                std::move(error_handler),
                std::move(log_handler)
        )) {}

rtsp::rtsp_client_pimpl::~rtsp_client_pimpl() = default;

void rtsp::rtsp_client_pimpl::set_rtp_packet_stat_callbacks(
        std::function<void(packet_count_t, packet_count_t)> received_packet_handler,
        std::function<void(packet_count_t, packet_count_t)> fec_packet_handler) {
    this->rtsp_client_->set_rtp_packet_stat_callbacks(std::move(received_packet_handler),
                                                      std::move(fec_packet_handler));
}

void rtsp::rtsp_client_pimpl::setup() {
    this->rtsp_client_->setup();
}

void rtsp::rtsp_client_pimpl::play() {
    this->rtsp_client_->play();
}

void rtsp::rtsp_client_pimpl::pause() {
    this->rtsp_client_->pause();
}

void rtsp::rtsp_client_pimpl::teardown() {
    this->rtsp_client_->teardown();
}

void rtsp::rtsp_client_pimpl::option() {
    this->rtsp_client_->option();
}

void rtsp::rtsp_client_pimpl::describe() {
    this->rtsp_client_->describe();
}
