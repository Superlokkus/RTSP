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
    rtsp_client(std::string url,
                std::function<void(std::exception &)> error_handler = [](auto) {},
                std::function<void()>
                log_handler = []() {}
    );

    ~rtsp_client();

    using jpeg_frame = std::vector<uint8_t>;

    void set_new_jpeg_frame_callback(std::function<void(jpeg_frame)> frame_handler = [](auto) {});

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

private:
};

}

#endif //RTSP_RTSP_CLIENT_HPP
