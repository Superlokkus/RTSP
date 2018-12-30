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

}

#endif //RTSP_RTP_ENDSYSTEM_HPP
