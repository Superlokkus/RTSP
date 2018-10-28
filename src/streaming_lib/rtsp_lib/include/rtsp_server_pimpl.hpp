/*! @file rtsp_server_pimpl.hpp
 *
 */

#ifndef RTSP_RTSP_SERVER_PIMPL_HPP
#define RTSP_RTSP_SERVER_PIMPL_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace rtsp {
class rtsp_server;

struct rtsp_server_pimpl final {
    explicit rtsp_server_pimpl(std::string video_file_directory, uint16_t port = 554,
                               std::function<void(std::exception &)> error_handler = [](auto) {});

    ~rtsp_server_pimpl();

    void graceful_shutdown();

private:
    std::unique_ptr<rtsp_server> rtsp_server_;
};

struct rtsp_client final {
    explicit rtsp_client(std::string host, uint16_t host_port, std::string video_file);
};
}

#endif //STREAMING_LIB_HPP

