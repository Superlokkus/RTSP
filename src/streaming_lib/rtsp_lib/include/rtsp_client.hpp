/*! @file rtsp_client.hpp
 *
 */

#ifndef RTSP_RTSP_CLIENT_HPP
#define RTSP_RTSP_CLIENT_HPP

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>
#include <string>

namespace rtsp {
class rtsp_client final {
public:
    using jpeg_frame = std::vector<uint8_t>;

    rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                std::function<void(std::exception &)> error_handler = [](auto) {},
                std::function<void(const std::string &)>
                log_handler = [](auto) {}
    );

    ~rtsp_client();


    using packet_count_t = uint32_t;

/*!
 *
 * @param received_packet_handler Gets received/expected rtp packet count for session
 * @param fec_packet_handler Gets corrected/not correctable rtp packet count for session
 */
    void set_rtp_packet_stat_callbacks(std::function<void(packet_count_t, packet_count_t)> received_packet_handler,
                                       std::function<void(packet_count_t, packet_count_t)> fec_packet_handler);

    void setup();

    void play();

    void pause();

    void teardown();

    void option();

    void describe();

private:
    std::function<void(std::exception &)> error_handler_;
    std::function<void(const std::string &)> log_handler_;
    std::function<void(jpeg_frame)> frame_handler_;
};

}

#endif //RTSP_RTSP_CLIENT_HPP
