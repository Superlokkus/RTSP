/*! @file rtp_endsystem.cpp
 *
 */

#ifndef RTSP_RTP_ENDSYSTEM_HPP
#define RTSP_RTP_ENDSYSTEM_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <istream>
#include <atomic>

#include <boost/asio.hpp>

namespace rtp {

class unicast_jpeg_rtp_sender final {
public:
    unicast_jpeg_rtp_sender() = delete;

    unicast_jpeg_rtp_sender(boost::asio::ip::udp::endpoint destination, uint16_t source_port,
                            uint32_t ssrc, std::unique_ptr<std::istream> jpeg_source,
                            boost::asio::io_context &io_context);

    ~unicast_jpeg_rtp_sender();

    void start();

    void stop();

private:
    boost::asio::io_context &io_context_;
    boost::asio::ip::udp::socket socket_;
    std::unique_ptr<std::istream> jpeg_source_;
    boost::asio::steady_timer send_packet_timer_;
    uint32_t ssrc_;
    boost::asio::io_context::strand strand_;
    boost::asio::ip::udp::endpoint destination_;

    std::atomic<bool> running_{false};

    static const uint8_t payload_type_number{26u};
    const uint8_t frame_period{40u};//!<ms Should be read from jpeg headers I guess
    using frame_counter_t = uint_fast16_t;
    static constexpr frame_counter_t frame_absolute_count{500u};//!<ms Should be read from jpeg headers I guess

    void send_next_packet_handler(const boost::system::error_code &error, frame_counter_t current_frame);
};

class unicast_jpeg_rtp_receiver final {
public:
    using jpeg_frame = std::vector<uint8_t>;

    unicast_jpeg_rtp_receiver() = delete;

    unicast_jpeg_rtp_receiver(uint16_t sink_port,
                              uint32_t ssrc, std::function<void(jpeg_frame)> frame_handler,
                              boost::asio::io_context &io_context);

    ~unicast_jpeg_rtp_receiver();

private:
    boost::asio::io_context &io_context_;

    using udp_buffer = std::array<char, 0xFFFF>;
    using shared_udp_socket = std::tuple<boost::asio::ip::udp::socket,
            boost::asio::io_context::strand,
            udp_buffer,
            boost::asio::ip::udp::endpoint>;
    shared_udp_socket socket_v4_;
    shared_udp_socket socket_v6_;

    std::function<void(jpeg_frame)> frame_handler_;

    uint32_t ssrc_;
    boost::asio::io_context::strand strand_;

    void start_async_receive(shared_udp_socket &socket);

    void handle_incoming_udp_traffic(const boost::system::error_code &error,
                                     std::size_t received_bytes,
                                     shared_udp_socket &incoming_socket);

    void handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                     shared_udp_socket &socket_received_from,
                                     boost::asio::ip::udp::endpoint received_from_endpoint);
};

}

#endif //RTSP_RTP_ENDSYSTEM_HPP
