/*! @file rtp_endsystem.cpp
 *
 */

#include <rtp_endsystem.hpp>

#include <array>
#include <algorithm>

#include <rtp_packet.hpp>

rtp::unicast_jpeg_rtp_session::unicast_jpeg_rtp_session(boost::asio::ip::udp::endpoint destination,
                                                        uint16_t source_port, uint32_t ssrc,
                                                        std::unique_ptr<std::istream> jpeg_source,
                                                        boost::asio::io_context &io_context) :
        io_context_(io_context),
        socket_(io_context_, boost::asio::ip::udp::endpoint{destination.protocol(), source_port}),
        jpeg_source_(std::move(jpeg_source)),
        send_packet_timer_(io_context_),
        ssrc_(ssrc),
        strand_(io_context_),
        destination_(std::move(destination)) {
    this->jpeg_source_->unsetf(std::ios::skipws);

}

rtp::unicast_jpeg_rtp_session::~unicast_jpeg_rtp_session() {
    this->stop();
    while (running_) { std::this_thread::yield(); }
}

void rtp::unicast_jpeg_rtp_session::start() {
    if (this->running_)
        return;
    this->running_ = true;

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->send_packet_timer_.expires_after(boost::asio::chrono::milliseconds(0));
        this->send_packet_timer_.async_wait(std::bind(&unicast_jpeg_rtp_session::send_next_packet_handler, this,
                                                      std::placeholders::_1, 0u));
    }));
}

void rtp::unicast_jpeg_rtp_session::stop() {
    if (!this->running_)
        return;

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->send_packet_timer_.cancel();
    }));
}


void rtp::unicast_jpeg_rtp_session::send_next_packet_handler(const boost::system::error_code &error,
                                                             frame_counter_t current_frame) {
    if (error || current_frame >= unicast_jpeg_rtp_session::frame_absolute_count) {
        this->running_ = false;
        return;
    }

    packet::custom_jpeg_packet packet{};
    packet.header.payload_type_field = unicast_jpeg_rtp_session::payload_type_number;
    packet.header.ssrc = this->ssrc_;
    packet.header.marker = true;
    packet.header.sequence_number = current_frame;
    packet.header.timestamp = current_frame * unicast_jpeg_rtp_session::frame_period;
    packet.header.image_width = 384u / 8u;
    packet.header.image_height = 288u / 8u;

    std::array<char, 5> frame_length{};
    this->jpeg_source_->read(frame_length.data(), frame_length.size());
    uint_fast32_t frame_size{};
    bool parsed = boost::spirit::qi::parse(frame_length.begin(), frame_length.end(),
                                           boost::spirit::qi::uint_parser<uint_fast32_t, 10>(), frame_size);
    if (!parsed) {
        this->running_ = false;
        throw std::runtime_error("Could not parse jpeg frame length");
    }

    packet.data.reserve(frame_size);
    std::istream_iterator<uint8_t> begin{*this->jpeg_source_}, end{};
    std::copy(begin, end, std::back_inserter(packet.data));

    std::shared_ptr<std::vector<uint8_t>> buffer;
    buffer->reserve(frame_size + 32u);
    rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
    boost::spirit::karma::generate(std::back_inserter(*buffer), gen_grammar, packet);

    this->socket_.async_send_to(boost::asio::buffer(*buffer), this->destination_,
                                std::bind([](auto buffer) {}, buffer));

    this->send_packet_timer_.expires_at(this->send_packet_timer_.expiry()
                                        + boost::asio::chrono::milliseconds(this->frame_period));
    this->send_packet_timer_.async_wait(boost::asio::bind_executor(this->strand_,
                                                                   std::bind(
                                                                           &unicast_jpeg_rtp_session::send_next_packet_handler,
                                                                           this,
                                                                           std::placeholders::_1, current_frame + 1)));
}
