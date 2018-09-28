/*! @file streaming_lib.hpp
 *
 */

#ifndef STREAMING_LIB_HPP
#define STREAMING_LIB_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace rtsp {
    class rtsp_server_;
    struct rtsp_server final {
        explicit rtsp_server(std::string video_file_directory, uint16_t port = 554,
                             std::function<void(std::exception &)> error_handler = [](auto) {});

        ~rtsp_server();

        void graceful_shutdown();

    private:
        std::unique_ptr<rtsp_server_> rtsp_server_;
    };

    struct rtsp_client final {
        explicit rtsp_client(std::string host, uint16_t host_port, std::string video_file);
    };
}

#endif //STREAMING_LIB_HPP