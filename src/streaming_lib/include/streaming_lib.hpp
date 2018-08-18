/*! @file streaming_lib.hpp
 *
 */

#ifndef STREAMING_LIB_HPP
#define STREAMING_LIB_HPP

#include <cstdint>
#include <string>

namespace mk {
    struct rtsp_server final {
        using port_number = uint16_t;
        using path = std::string;

        rtsp_server(path video_file_directory, port_number udp_port = 554);
    };
}

#endif //STREAMING_LIB_HPP
