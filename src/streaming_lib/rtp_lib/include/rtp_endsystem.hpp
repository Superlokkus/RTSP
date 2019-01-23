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
#include <random>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <rtp_packet.hpp>

namespace rtp {

struct fec_generator {
    fec_generator(uint16_t fec_k);

    /*! @Generates the FEC header, fec level headers and payload (but not the rtp header)
     *
     */
    std::shared_ptr<std::vector<uint8_t>> generate_next_fec_packet(const std::vector<uint8_t> &media_packet,
                                                                   uint16_t seq_num);

    static const uint8_t fec_payload_type_number{100u};
private:
    uint16_t fec_k_;
    uint16_t current_fec_k_;

    std::vector<uint8_t> fec_bit_string_;
    uint16_t fec_seq_base{0u};
};

class unicast_jpeg_rtp_sender final {
public:
    unicast_jpeg_rtp_sender() = delete;

    unicast_jpeg_rtp_sender(boost::asio::ip::udp::endpoint destination, uint16_t source_port,
                            uint32_t ssrc, std::unique_ptr<std::istream> jpeg_source,
                            boost::asio::io_context &io_context);

    unicast_jpeg_rtp_sender(boost::asio::ip::udp::endpoint destination, uint16_t source_port,
                            uint32_t ssrc, std::unique_ptr<std::istream> jpeg_source,
                            boost::asio::io_context &io_context, double channel_simulation_droprate,
                            uint16_t fec_k);

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

    std::default_random_engine re_;

    boost::optional<std::bernoulli_distribution> channel_failure_distribution_; ///< Prob of loss
    boost::optional<fec_generator> fec_generator_;


    uint16_t current_sequence_number;
    uint16_t current_fec_sequence_number;

    void send_next_packet_handler(const boost::system::error_code &error);
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

    uint16_t highest_seq_seen() const {
        return max_seq;
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

    int MAX_DROPOUT = 3000;
    int MAX_MISORDER = 100;
    int MIN_SEQUENTIAL = 2;
    uint32_t RTP_SEQ_MOD = (1 << 16);
};

class unicast_jpeg_rtp_receiver final {
public:
    using jpeg_frame = std::vector<uint8_t>;

    unicast_jpeg_rtp_receiver() = delete;

    unicast_jpeg_rtp_receiver(uint16_t sink_port,
                              uint32_t ssrc, std::function<void(jpeg_frame)> frame_handler,
                              boost::asio::io_context &io_context);

    ~unicast_jpeg_rtp_receiver();

    struct statistics {
        uint64_t received;
        uint64_t expected;
        uint64_t corrected;
        uint64_t uncorrectable;
    };

    void set_statistics_handler(std::function<void(statistics)> statistics_handler);

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
    std::function<void(statistics)> statistics_handler_;

    uint32_t ssrc_;
    boost::asio::io_context::strand strand_;
    boost::asio::steady_timer display_next_frame_timer_;

    boost::optional<rtp_receiver_squence_utility> jpeg_sequence_utility_;
    boost::optional<rtp_receiver_squence_utility> fec_sequence_utility_;

    std::deque<rtp::packet::custom_jpeg_packet> jpeg_packet_frame_buffer_;

    std::deque<std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>>> jpeg_packet_incoming_buffer_;
    std::deque<std::pair<rtp::packet::custom_fec_packet, std::vector<uint8_t>>> fec_packet_incoming_buffer_;


    uint64_t corrected_{0u};
    uint64_t uncorrectable_{0u};

    const uint8_t frame_period{40u};//!<ms Should be read from jpeg headers I guess
    const uint8_t buffer_size{50u};
    const uint8_t media_packet_delay{20u};

    void start_async_receive(shared_udp_socket &socket);

    void handle_incoming_udp_traffic(const boost::system::error_code &error,
                                     std::size_t received_bytes,
                                     shared_udp_socket &incoming_socket);

    void handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                     shared_udp_socket &socket_received_from,
                                     boost::asio::ip::udp::endpoint received_from_endpoint);

    bool handle_new_jpeg_packet(const std::vector<char> &message);

    bool handle_new_fec_packet(const std::vector<char> &message);

    bool recover_packet_by_fec();

    static std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>> fec_reconstruction(
            const std::pair<rtp::packet::custom_fec_packet, std::vector<uint8_t>> &fec_packet,
            const std::vector<std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>>> &media_packets);

    void display_next_frame_timer_handler(const boost::system::error_code &error);
};

}

#endif //RTSP_RTP_ENDSYSTEM_HPP
