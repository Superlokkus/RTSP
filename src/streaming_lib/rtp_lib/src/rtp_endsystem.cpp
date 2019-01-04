/*! @file rtp_endsystem.cpp
 *
 */

#include <rtp_endsystem.hpp>

#include <array>
#include <algorithm>

#include <rtp_packet.hpp>

#include <boost/log/trivial.hpp>

rtp::unicast_jpeg_rtp_sender::unicast_jpeg_rtp_sender(boost::asio::ip::udp::endpoint destination,
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
    BOOST_LOG_TRIVIAL(debug) << "New unicast_jpeg_rtp_sender to " <<
                             this->destination_ << " from " << this->socket_.local_endpoint();
    this->jpeg_source_->unsetf(std::ios::skipws);

}

rtp::unicast_jpeg_rtp_sender::~unicast_jpeg_rtp_sender() {
    this->stop();
    while (running_) { std::this_thread::yield(); }
}

void rtp::unicast_jpeg_rtp_sender::start() {
    if (this->running_)
        return;
    this->running_ = true;

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->send_packet_timer_.expires_after(boost::asio::chrono::milliseconds(0));
        this->send_packet_timer_.async_wait(std::bind(&unicast_jpeg_rtp_sender::send_next_packet_handler, this,
                                                      std::placeholders::_1, 1u));
    }));
}

void rtp::unicast_jpeg_rtp_sender::stop() {
    if (!this->running_)
        return;

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->send_packet_timer_.cancel();
    }));
}


void rtp::unicast_jpeg_rtp_sender::send_next_packet_handler(const boost::system::error_code &error,
                                                             frame_counter_t current_frame) {
    BOOST_LOG_TRIVIAL(debug) << "Send next rtp packet, current frame: " << current_frame;
    if (error || current_frame >= unicast_jpeg_rtp_sender::frame_absolute_count) {
        BOOST_LOG_TRIVIAL(debug) << "Current frame " << current_frame << ">="
                                 << unicast_jpeg_rtp_sender::frame_absolute_count;
        this->running_ = false;
        return;
    }


    packet::custom_jpeg_packet packet{};
    packet.header.payload_type_field = unicast_jpeg_rtp_sender::payload_type_number;
    packet.header.ssrc = this->ssrc_;
    packet.header.marker = true;
    packet.header.sequence_number = current_frame;
    packet.header.timestamp = current_frame * unicast_jpeg_rtp_sender::frame_period * 90;//!<10^-3 * 10^4 = 10^1
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

    BOOST_LOG_TRIVIAL(debug) << "Frame_size " << frame_size;
    packet.data.reserve(frame_size);
    std::istream_iterator<uint8_t> begin{*this->jpeg_source_};
    std::copy_n(begin, frame_size, std::back_inserter(packet.data));

    auto buffer = std::make_shared<std::vector<uint8_t>>();
    buffer->reserve(frame_size + 32u);
    rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
    boost::spirit::karma::generate(std::back_inserter(*buffer), gen_grammar, packet);

    this->socket_.async_send_to(boost::asio::buffer(*buffer), this->destination_,
                                std::bind([](auto buffer) {}, buffer));

    this->send_packet_timer_.expires_at(this->send_packet_timer_.expiry()
                                        + boost::asio::chrono::milliseconds(this->frame_period));
    this->send_packet_timer_.async_wait(boost::asio::bind_executor(this->strand_,
                                                                   std::bind(
                                                                           &unicast_jpeg_rtp_sender::send_next_packet_handler,
                                                                           this,
                                                                           std::placeholders::_1, current_frame + 1)));
}

rtp::unicast_jpeg_rtp_receiver::unicast_jpeg_rtp_receiver(uint16_t sink_port,
                                                          uint32_t ssrc, std::function<void(jpeg_frame)> frame_handler,
                                                          boost::asio::io_context &io_context) :
        io_context_(io_context),
        socket_v4_{std::forward_as_tuple(
                boost::asio::ip::udp::socket{io_context_,
                                             boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(),
                                                                            sink_port}},
                boost::asio::io_context::strand{io_context_},
                udp_buffer{},
                boost::asio::ip::udp::endpoint{}
        )},
        socket_v6_{std::forward_as_tuple(
                boost::asio::ip::udp::socket{io_context_,
                                             boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v6(),
                                                                            sink_port}},
                boost::asio::io_context::strand{io_context_},
                udp_buffer{},
                boost::asio::ip::udp::endpoint{}
        )},
        frame_handler_(std::move(frame_handler)),
        ssrc_(ssrc),
        strand_(io_context_) {
    BOOST_LOG_TRIVIAL(debug) << "New unicast_jpeg_rtp_receiver from :" <<
                             sink_port << " with ssrc " << ssrc;
    this->start_async_receive(this->socket_v4_);
    this->start_async_receive(this->socket_v6_);
}

void rtp::unicast_jpeg_rtp_receiver::start_async_receive(rtp::unicast_jpeg_rtp_receiver::shared_udp_socket &socket) {
    std::get<0>(socket).async_receive_from(boost::asio::buffer(std::get<2>(socket)),
                                           std::get<3>(socket),
                                           boost::asio::bind_executor(std::get<1>(socket), std::bind(
                                                   &rtp::unicast_jpeg_rtp_receiver::handle_incoming_udp_traffic,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::ref(socket)
                                           )));
}

void rtp::unicast_jpeg_rtp_receiver::handle_incoming_udp_traffic(const boost::system::error_code &error,
                                                                 std::size_t received_bytes,
                                                                 rtp::unicast_jpeg_rtp_receiver::shared_udp_socket &incoming_socket) {
    if (error)
        throw std::runtime_error{error.message()};

    auto data = std::make_shared<std::vector<char>>();

    std::copy_n(std::get<2>(incoming_socket).cbegin(), received_bytes, std::back_inserter(*data));
    boost::asio::ip::udp::endpoint received_from_endpoint = std::get<3>(incoming_socket);

    boost::asio::dispatch(std::get<1>(incoming_socket).get_io_context(),
                          boost::asio::bind_executor(this->strand_,
                                                     std::bind(
                                                             &rtp::unicast_jpeg_rtp_receiver::handle_new_incoming_message,
                                                             this,
                                                             data, std::ref(incoming_socket),
                                                             received_from_endpoint))
    );

    start_async_receive(incoming_socket);
}

void rtp::unicast_jpeg_rtp_receiver::handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                                                 rtp::unicast_jpeg_rtp_receiver::shared_udp_socket &socket_received_from,
                                                                 boost::asio::ip::udp::endpoint received_from_endpoint) {
    BOOST_LOG_TRIVIAL(debug) << "Got new udp message in unicast_jpeg_rtp_receiver";
    BOOST_LOG_TRIVIAL(trace) << message;
}

rtp::unicast_jpeg_rtp_receiver::~unicast_jpeg_rtp_receiver() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->socket_v4_), [this]() {
        std::get<0>(this->socket_v4_).cancel();
    }));
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->socket_v6_), [this]() {
        std::get<0>(this->socket_v6_).cancel();
    }));
}

