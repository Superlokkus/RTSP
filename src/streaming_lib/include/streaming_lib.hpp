/*! @file streaming_lib.hpp
 *
 */

#ifndef STREAMING_LIB_HPP
#define STREAMING_LIB_HPP

#include <cstdint>
#include <string>

namespace rtsp {
    using port_number = uint16_t;
    using path = std::string;
    using inet_adress = std::string;
    struct rtsp_server final {
        rtsp_server(path video_file_directory, port_number udp_port = 554);
    };

    struct rtsp_client final {
        rtsp_client(inet_adress host, port_number host_port, path video_file);
    };
}

#endif //STREAMING_LIB_HPP
