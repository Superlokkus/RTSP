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
#include <deque>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <rtp_packet.hpp>

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

/*! @brief RTP receiver algorithms and metrics derived from RFC appendix reference implementation
 *
 */
class rtp_receiver_squence_utility {
public:
    rtp_receiver_squence_utility(uint16_t seq);

    void init_seq(uint16_t seq);

    /*!
     *
     * @param seq
     * @return A value of one is returned to indicate a valid sequence number.
     * Otherwise, the value zero is returned to indicate that the validation failed.
     */
    int update_seq(uint16_t seq);

    uint64_t extended_max() const {
        return this->cycles + this->max_seq;
    }

    uint64_t expected() const {
        return this->extended_max() - this->base_seq + 1;
    }

    uint64_t lost() const {
        return expected() - this->received;
    }

    uint32_t received_packets() const {
        return this->received;
    }

private:
    uint16_t max_seq;        /*!< highest seq. number seen */
    uint32_t cycles;         /*!< shifted count of seq. number cycles */
    uint32_t base_seq;       /*!< base seq number */
    uint32_t bad_seq;        /*!< last 'bad' seq number + 1 */
    uint32_t probation;      /*!< sequ. packets till source is valid */
    uint32_t received;       /*!< packets received */
    uint32_t expected_prior; /*!< packet expected at last interval */
    uint32_t received_prior; /*!< packet received at last interval */
    uint32_t transit;        /*!< relative trans time for prev pkt */
    uint32_t jitter;         /*!< estimated jitter */

    const int MAX_DROPOUT = 3000;
    const int MAX_MISORDER = 100;
    const int MIN_SEQUENTIAL = 2;
    const uint32_t RTP_SEQ_MOD = (1 << 16);
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
    boost::asio::steady_timer display_next_frame_timer_;

    std::deque<rtp::packet::custom_jpeg_packet> jpeg_packet_buffer_;

    const uint8_t frame_period{40u};//!<ms Should be read from jpeg headers I guess

    void start_async_receive(shared_udp_socket &socket);

    void handle_incoming_udp_traffic(const boost::system::error_code &error,
                                     std::size_t received_bytes,
                                     shared_udp_socket &incoming_socket);

    void handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                     shared_udp_socket &socket_received_from,
                                     boost::asio::ip::udp::endpoint received_from_endpoint);

    void display_next_frame_timer_handler(const boost::system::error_code &error);
};

}

#endif //RTSP_RTP_ENDSYSTEM_HPP
