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

#include <rtsp_message.hpp>
#include <rtsp_server_internals.hpp>

namespace rtsp {
    namespace fileapi = boost::filesystem;

class rtsp_server final {
    public:
        /*!
         * @param ressource_root Directory which will be the root of the servers rtsp url absolute path
         * @param port_number number to listen for new connections
         * @param error_handler Callback if a connection handler throws an
         * error (for logging purposes), must be thread safe
         * @warning error_handler Can be called by multiple threads concurrently
         * @throws std::runtime_error When @ref ressource_root can not be opend
         * @throws std::exception For errors due to ressource allocation
         */
        rtsp_server(fileapi::path ressource_root,
                    uint16_t port_number = 554,
                    std::function<void(std::exception &)> error_handler = [](auto) {});


        /*! @brief Shutdowns the server as fast as possible
         */
        ~rtsp_server();

        /*! @brief Initiates a graceful shutdown and blocks until done
         * Graceful means that responses to already received requests will be send, and tcp connections are closed
         */
        void graceful_shutdown();

    private:
        fileapi::path ressource_root_;
        rtsp::server::rtsp_server_state server_state_;
        boost::asio::io_context io_context_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
        std::vector<std::thread> io_run_threads_;

        using udp_buffer = std::array<char, 0xFFFF>;
        using shared_udp_socket = std::tuple<boost::asio::ip::udp::socket,
                boost::asio::io_context::strand,
                udp_buffer,
                boost::asio::ip::udp::endpoint>;

        /*! @brief Struct for self-containting tcp connections i.e. sessions
         *
         */
        struct tcp_connection;

        shared_udp_socket udp_v4_socket_;
        shared_udp_socket udp_v6_socket_;
        boost::asio::ip::tcp::acceptor tcp_v4_acceptor_;
        boost::asio::ip::tcp::acceptor tcp_v6_acceptor_;
        const std::function<void(std::exception &)> error_handler_;

        static void
        io_run_loop(boost::asio::io_context &context, const std::function<void(std::exception &)> &error_handler);

        /*! @brief Handler to extract endpoint from socket global for @ref handle_new_incoming_message , which will
         * be called async with all the data from the incoming datagram, and restart listening for the next udp datagram
         * @param error Error from boost asio
         * @param received_bytes Received bytes from boost asio
         * @param incoming_socket Socket we got the datagram from
         */
        void handle_incoming_udp_traffic(const boost::system::error_code &error,
                                         std::size_t received_bytes,
                                         shared_udp_socket &incoming_socket);

        /*! @brief Handler to start the new @ref tcp_connection and issue the listening for the next tcp connection
         * by calling @ref start_async_receive
         * @param error Error by asio
         * @param acceptor Accepctor to issue next @ref start_async_receive on
         * @param new_connection Connection struct with the socket for the new tcp connection
         */
        void handle_new_tcp_connection(const boost::system::error_code &error,
                                       boost::asio::ip::tcp::acceptor &acceptor,
                                       std::shared_ptr<tcp_connection> new_connection);

        /*! @brief Handles new udp requests
         *
         * Parses new datagram requests, sends bad request responses on failre to parse or forwards the parsed request to
         * the server state, to send it's returned response
         * @param message Incoming raw rtsp request
         * @param socket_received_from Socket to use to send the responses
         * @param received_from_endpoint Endpoint to send response to
         * @param server_state State to use for side effects of rtsp request
         */
        static void handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                                shared_udp_socket &socket_received_from,
                                                boost::asio::ip::udp::endpoint received_from_endpoint,
                                                server::rtsp_server_state &server_state);


        /*! @brief Utility to start listening for a new udp datagram on the socket and
         * let @ref handle_incoming_udp_traffic then handle it
         *
         * @param socket Socket to listen for new datagrams
         */
        void start_async_receive(shared_udp_socket &socket);

        /*! @brief Utility to start to listening for a new tcp connection on the given acceptor, to let
         * @ref handle_new_tcp_connection handle new connections
         *
         * @param acceptor Acceptor to use for new connections
         */
        void start_async_receive(boost::asio::ip::tcp::acceptor &acceptor);

        /*! @brief Utility to start a async send to the udp endpoint using the given socket while sychronizing via its
         * strand
         * @param socket UDP socket to send on, including it's strand
         * @param to_endpoint Endpoint to send to
         * @param message Message to send
         */
        static void start_async_send_to(shared_udp_socket &socket,
                                        boost::asio::ip::udp::endpoint to_endpoint,
                                        std::shared_ptr<std::string> message);

    };
}


#endif //RTSP_GUI_RTSP_SERVER_HPP
