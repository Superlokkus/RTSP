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

void rtsp::rtsp_client_pimpl::set_rtp_statistics_handler(std::function<void(uint64_t, uint64_t, uint64_t, uint64_t)>
                                                         rtp_statistics_handler) {

    this->rtsp_client_->set_rtp_statistics_handler([rtp_statistics_handler]
                                                           (rtp::unicast_jpeg_rtp_receiver::statistics s1) {
        rtp_statistics_handler(s1.received, s1.expected, s1.corrected, s1.uncorrectable);
    });
}

void rtsp::rtsp_client_pimpl::set_mkn_options(bool general_switch, double bernoulli_p, uint16_t fec_k, uint16_t fec_p) {
    this->rtsp_client_->set_mkn_options(general_switch, bernoulli_p, fec_k, fec_p);
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
