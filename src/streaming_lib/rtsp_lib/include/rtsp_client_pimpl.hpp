/*! @file rtsp_client_pimpl.hpp
 *
 */

#ifndef RTSP_RTSP_CLIENT_PIMPL_HPP
#define RTSP_RTSP_CLIENT_PIMPL_HPP

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>
#include <string>

namespace rtsp {

class rtsp_client;

struct rtsp_client_pimpl final {
    using jpeg_frame = std::vector<uint8_t>;

    explicit rtsp_client_pimpl(std::string url, std::function<void(jpeg_frame)> frame_handler,
                               std::function<void(std::exception &)> error_handler = [](auto) {},
                               std::function<void(const std::string &)> log_handler = [](auto) {});

    ~rtsp_client_pimpl();

    /*!
     * @param rtp_statistics_handler Expect parameters:
     * uint64_t received;
     * uint64_t expected;
     * uint64_t corrected;
     * uint64_t uncorrectable;
     */
    void set_rtp_statistics_handler(std::function<void(uint64_t, uint64_t, uint64_t, uint64_t)> rtp_statistics_handler);

    void setup();

    void play();

    void pause();

    void teardown();

    void option();

    void describe();


private:
    std::unique_ptr<rtsp_client> rtsp_client_;
};

}

#endif //RTSP_RTSP_CLIENT_PIMPL_HPP
