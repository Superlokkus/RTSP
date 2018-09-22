/*! @file rtsp_server.hpp
 *
 */

#ifndef RTSP_GUI_RTSP_SERVER_HPP
#define RTSP_GUI_RTSP_SERVER_HPP

#include <cstdint>
#include <utility>
#include <thread>
#include <vector>
#include <functional>
#include <memory>
#include <tuple>

#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>

#include <rtsp_request.hpp>

namespace rtsp {
    namespace fileapi = boost::filesystem;

    class rtsp_server_ final {
    public:
        /*!
         * @param ressource_root Directory which will be the root of the servers rtsp url absolute path
         * @param udp_port_number number to listen for new connections
         * @param error_handler Callback if a connection handler throws an error (for logging purposes), must be thread safe
         * @warning error_handler Can be called by multiple threads concurrently
         * @throws std::runtime_error When @ref ressource_root can not be opend
         * @throws std::exception For errors due to ressource allocation
         */
        rtsp_server_(fileapi::path ressource_root,
                     uint16_t udp_port_number = 554,
                     std::function<void(std::exception &)> error_handler = [](auto) {});


        ~rtsp_server_();

    private:
        fileapi::path ressource_root_;
        boost::asio::io_context io_context_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
        std::vector<std::thread> io_run_threads_;

        using udp_buffer = std::array<char, 0xFFFF>;
        using shared_udp_socket = std::tuple<boost::asio::ip::udp::socket,
                boost::asio::io_context::strand,
                udp_buffer,
                boost::asio::ip::udp::endpoint>;
        shared_udp_socket udp_v4_socket_;
        shared_udp_socket udp_v6_socket_;
        const std::function<void(std::exception &)> error_handler_;

        static void
        io_run_loop(boost::asio::io_context &context, const std::function<void(std::exception &)> &error_handler);

        static void handle_incoming_udp_traffic(const boost::system::error_code &error,
                                                std::size_t received_bytes,
                                                shared_udp_socket &incoming_socket);

        static void handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                                shared_udp_socket &socket_received_from);
    };
}


#endif //RTSP_GUI_RTSP_SERVER_HPP
