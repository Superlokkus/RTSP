/*! @file rtp_endsystem.cpp
 *
 */

#include <rtp_endsystem.hpp>

#include <array>
#include <algorithm>

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
        destination_(std::move(destination)),
        re_(std::random_device()()) {
    BOOST_LOG_TRIVIAL(debug) << "New unicast_jpeg_rtp_sender to " <<
                             this->destination_ << " from " << this->socket_.local_endpoint();
    this->jpeg_source_->unsetf(std::ios::skipws);
    this->current_sequence_number = std::uniform_int_distribution<uint16_t>(0u, 60000u)(this->re_);
    this->current_fec_sequence_number = std::uniform_int_distribution<uint16_t>(0u, 60000u)(this->re_);
    boost::asio::socket_base::send_buffer_size option(65536u);
    socket_.set_option(option);
}

rtp::unicast_jpeg_rtp_sender::unicast_jpeg_rtp_sender(boost::asio::ip::udp::endpoint destination, uint16_t source_port,
                                                      uint32_t ssrc, std::unique_ptr<std::istream> jpeg_source,
                                                      boost::asio::io_context &io_context,
                                                      double channel_simulation_droprate,
                                                      uint16_t fec_k) :
        unicast_jpeg_rtp_sender(std::move(destination),
                                source_port, ssrc, std::move(jpeg_source), io_context) {
    BOOST_LOG_TRIVIAL(debug) << "And unicast_jpeg_rtp_sender with channel droprate of " << channel_simulation_droprate
                             << " and fec_k of " << fec_k;
    this->channel_failure_distribution_ = std::bernoulli_distribution{channel_simulation_droprate};
    this->fec_generator_ = fec_generator{fec_k, ssrc_};
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
                                                      std::placeholders::_1));
    }));
}

void rtp::unicast_jpeg_rtp_sender::stop() {
    if (!this->running_)
        return;

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->send_packet_timer_.cancel();
    }));
}


void rtp::unicast_jpeg_rtp_sender::send_next_packet_handler(const boost::system::error_code &error) {
    //BOOST_LOG_TRIVIAL(trace) << "Send next rtp packet, current seq num: " << current_sequence_number;
    if (error) {
        BOOST_LOG_TRIVIAL(debug) << "Stopping RTP sending because: " << error.message();
        this->running_ = false;
        return;
    }


    packet::custom_jpeg_packet packet{};
    packet.header.payload_type_field = unicast_jpeg_rtp_sender::payload_type_number;
    packet.header.ssrc = this->ssrc_;
    packet.header.marker = true;
    packet.header.sequence_number = current_sequence_number;
    packet.header.timestamp =
            current_sequence_number * unicast_jpeg_rtp_sender::frame_period * 90;//!<10^-3 * 10^4 = 10^1
    packet.header.image_width = 384u / 8u;
    packet.header.image_height = 288u / 8u;

    std::array<char, 5> frame_length{};
    this->jpeg_source_->read(frame_length.data(), frame_length.size());
    if (this->jpeg_source_->gcount() != 5) {
        this->running_ = false;
        BOOST_LOG_TRIVIAL(debug) << "Could not read jpeg frame length from file, probably EOF";
        return;
    }
    uint_fast32_t frame_size{};
    bool parsed = boost::spirit::qi::parse(frame_length.begin(), frame_length.end(),
                                           boost::spirit::qi::uint_parser<uint_fast32_t, 10>(), frame_size);
    if (!parsed) {
        this->running_ = false;
        throw std::runtime_error("Could not parse jpeg frame length");
    }

    //BOOST_LOG_TRIVIAL(trace) << "Frame_size " << frame_size;
    packet.data.reserve(frame_size);
    std::istream_iterator<uint8_t> begin{*this->jpeg_source_};
    if (begin == std::istream_iterator<uint8_t>{}) {
        this->running_ = false;
        BOOST_LOG_TRIVIAL(debug) << "File end already reached, ending RTP sending";
        return;
    }
    const auto past_last_element = std::copy_n(begin, frame_size, std::back_inserter(packet.data));

    auto buffer = std::make_shared<std::vector<uint8_t>>();
    buffer->reserve(frame_size + 32u);
    rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
    boost::spirit::karma::generate(std::back_inserter(*buffer), gen_grammar, packet);

    if (!this->channel_failure_distribution_ || !this->channel_failure_distribution_->operator()(re_)) {
        this->socket_.async_send_to(boost::asio::buffer(*buffer), this->destination_,
                                    std::bind([](const boost::system::error_code &error, std::size_t bytes_transferred,
                                                 auto buffer) {
                                        if (error)
                                            throw std::runtime_error{error.message()};
                                        if (bytes_transferred != buffer->size())
                                            throw std::runtime_error{"Couldn't send complete rtp packet"};
                                    }, std::placeholders::_1, std::placeholders::_2, buffer));
    } else {
        //BOOST_LOG_TRIVIAL(trace) << "Not sending rtp sequence number " << current_sequence_number;
    }

    if (this->fec_generator_) {
        auto fec_packet_buffer = this->fec_generator_->generate_next_fec_packet(*buffer, current_sequence_number);
        if (fec_packet_buffer &&
            (!this->channel_failure_distribution_ || !this->channel_failure_distribution_->operator()(re_))) {
            packet::standard_rtp_header rtp_header{};
            rtp_header.sequence_number = this->current_fec_sequence_number++;
            rtp_header.ssrc = this->ssrc_;
            rtp_header.payload_type_field = 100u;
            rtp_header.timestamp = packet.header.timestamp;
            auto full_fec_packet = std::make_shared<std::vector<uint8_t>>();
            packet::standard_rtp_header_generator<std::back_insert_iterator<std::vector<uint8_t>>> gen_grammar{};
            boost::spirit::karma::generate(std::back_inserter(*full_fec_packet), gen_grammar, rtp_header);
            std::move(fec_packet_buffer->begin(), fec_packet_buffer->end(), std::back_inserter(*full_fec_packet));

            this->socket_.async_send_to(boost::asio::buffer(*full_fec_packet), this->destination_,
                                        std::bind([](const boost::system::error_code &error,
                                                     std::size_t bytes_transferred,
                                                     auto buffer) {
                                            if (error)
                                                throw std::runtime_error{error.message()};
                                            if (bytes_transferred != buffer->size())
                                                throw std::runtime_error{"Couldn't send complete fec rtp packet"};
                                        }, std::placeholders::_1, std::placeholders::_2, full_fec_packet));
        }
    }


    current_sequence_number += 1;
    this->send_packet_timer_.expires_at(this->send_packet_timer_.expiry()
                                        + boost::asio::chrono::milliseconds(this->frame_period));
    this->send_packet_timer_.async_wait(boost::asio::bind_executor(this->strand_,
                                                                   std::bind(
                                                                           &unicast_jpeg_rtp_sender::send_next_packet_handler,
                                                                           this,
                                                                           std::placeholders::_1)));
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
        socket_v6_{std::forward_as_tuple(boost::asio::ip::udp::socket{io_context_},
                                         boost::asio::io_context::strand{io_context_},
                                         udp_buffer{},
                                         boost::asio::ip::udp::endpoint{}
        )},
        frame_handler_(std::move(frame_handler)),
        ssrc_(ssrc),
        strand_(io_context_),
        display_next_frame_timer_(io_context_) {
    BOOST_LOG_TRIVIAL(debug) << "New unicast_jpeg_rtp_receiver from :" <<
                             sink_port << " with ssrc " << ssrc;
    try {
        std::get<0>(socket_v6_) = boost::asio::ip::udp::socket{io_context_,
                                                               boost::asio::ip::udp::endpoint{
                                                                       boost::asio::ip::udp::v6(),
                                                                       sink_port}};
    } catch (boost::system::system_error &e) {
        BOOST_LOG_TRIVIAL(error) << "Could not create UDP IPv6 socket: " << e.what();
    }

    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->display_next_frame_timer_.expires_after(boost::asio::chrono::milliseconds(20));
        this->display_next_frame_timer_.async_wait(
                std::bind(&unicast_jpeg_rtp_receiver::display_next_frame_timer_handler, this,
                          std::placeholders::_1));
    }));
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
    //BOOST_LOG_TRIVIAL(trace) << "Got new udp message in unicast_jpeg_rtp_receiver";

    rtp::packet::custom_jpeg_packet_grammar<std::vector<char>::const_iterator> custom_jpeg_grammar{};
    auto begin = message->cbegin(), end = message->cend();

    rtp::packet::custom_jpeg_packet jpeg_packet;
    auto success = boost::spirit::qi::parse(begin, end, custom_jpeg_grammar, jpeg_packet);

    if (!success) {
        BOOST_LOG_TRIVIAL(debug) << "Not parseable as jpeg packet";
        return;
    }

    if (jpeg_packet.header.ssrc != this->ssrc_) {
        throw std::runtime_error{"Got jpeg rtp packet with wrong ssrc, dropping"};
    }

    if (jpeg_packet.data.at(jpeg_packet.data.size() - 2) != 0xFFu ||
        jpeg_packet.data.at(jpeg_packet.data.size() - 1) != 0xD9u) {
        BOOST_LOG_TRIVIAL(error) << "Invalid jpeg data!";
    }

    if (!sequence_utility_) { //First packet
        sequence_utility_ = boost::make_optional<rtp_receiver_squence_utility>(jpeg_packet.header.sequence_number);
    }
    if (sequence_utility_->update_seq(jpeg_packet.header.sequence_number) == 0) {
        BOOST_LOG_TRIVIAL(trace) << "Sequence number invalid: " << jpeg_packet.header.sequence_number <<
                                 "Expected " << sequence_utility_->highest_seq_seen();
    }

    if (this->statistics_handler_) {
        this->statistics_handler_(statistics{this->sequence_utility_->received_packets(),
                                             this->sequence_utility_->expected(),
                                             0, 0
        });
    }

    this->jpeg_packet_frame_buffer_.push_back(std::move(jpeg_packet));
}

void rtp::unicast_jpeg_rtp_receiver::display_next_frame_timer_handler(const boost::system::error_code &error) {
    if (error)
        throw std::runtime_error{error.message()};

    if (!this->jpeg_packet_frame_buffer_.empty()) {
        this->frame_handler_(this->jpeg_packet_frame_buffer_.front().data);
        this->jpeg_packet_frame_buffer_.pop_front();
    }

    this->display_next_frame_timer_.expires_at(this->display_next_frame_timer_.expiry()
                                               + boost::asio::chrono::milliseconds(this->frame_period));
    this->display_next_frame_timer_.async_wait(boost::asio::bind_executor(this->strand_,
                                                                          std::bind(
                                                                                  &unicast_jpeg_rtp_receiver::display_next_frame_timer_handler,
                                                                                  this,
                                                                                  std::placeholders::_1)));
}

rtp::unicast_jpeg_rtp_receiver::~unicast_jpeg_rtp_receiver() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->socket_v4_), [this]() {
        std::get<0>(this->socket_v4_).cancel();
    }));
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->socket_v6_), [this]() {
        std::get<0>(this->socket_v6_).cancel();
    }));
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        this->display_next_frame_timer_.cancel();
    }));
}

void rtp::unicast_jpeg_rtp_receiver::set_statistics_handler(std::function<void(statistics)> statistics_handler) {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this, statistics_handler]() {
        this->statistics_handler_ = statistics_handler;
    }));
}

rtp::rtp_receiver_squence_utility::rtp_receiver_squence_utility(uint16_t seq) {
    this->init_seq(seq);
    this->max_seq = seq - 1;
    this->probation = MIN_SEQUENTIAL;
}

void rtp::rtp_receiver_squence_utility::init_seq(uint16_t seq) {
    this->base_seq = seq;
    this->max_seq = seq;
    this->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
    this->cycles = 0;
    this->received = 0;
    this->received_prior = 0;
    this->expected_prior = 0;
}

int rtp::rtp_receiver_squence_utility::update_seq(uint16_t seq) {
    uint16_t udelta = seq - this->max_seq;
    /*
     * Source is not valid until MIN_SEQUENTIAL packets with
     * sequential sequence numbers have been received.
     */
    if (this->probation) {
        /* packet is in sequence */
        if (seq == this->max_seq + 1) {
            this->probation--;
            this->max_seq = seq;
            if (this->probation == 0) {
                init_seq(seq);
                this->received++;
                return 1;
            }
        } else {
            this->probation = MIN_SEQUENTIAL - 1;
            this->max_seq = seq;
        }
        return 0;
    } else if (udelta < MAX_DROPOUT) {
        /* in order, with permissible gap */
        if (seq < this->max_seq) {
            /*
             * Sequence number wrapped - count another 64K cycle.
             */
            this->cycles += RTP_SEQ_MOD;
        }
        this->max_seq = seq;
    } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
        /* the sequence number made a very large jump */
        if (seq == this->bad_seq) {
            /*
             * Two sequential packets -- assume that the other side
             * restarted without telling us so just re-sync
             * (i.e., pretend this was the first packet).
             */
            init_seq(seq);
        } else {
            this->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
            return 0;
        }
    } else {
        /* duplicate or reordered packet */
    }
    this->received++;
    return 1;
}

rtp::fec_generator::fec_generator(uint16_t fec_k, uint32_t ssrc) :
        fec_k_(fec_k), current_fec_k_(fec_k), ssrc_(ssrc) {
}

std::shared_ptr<std::vector<uint8_t>>
rtp::fec_generator::generate_next_fec_packet(const std::vector<uint8_t> &media_packet, uint16_t seq_num) {
    auto f = [](uint8_t a, uint8_t b) -> uint8_t {
        return a ^ b;
    };

    if (media_packet.size() < 12)
        throw std::logic_error("Mediapacket size < 10");

    if (this->fec_seq_base != 0) {
        this->fec_seq_base = std::min(seq_num, this->fec_seq_base); //Account for wraparound?! how?! whole RTP logic?!
    } else {
        this->fec_seq_base = seq_num;
    }

    auto begin = this->fec_bit_string_.begin(), end = this->fec_bit_string_.end();
    if (this->fec_bit_string_.empty()) {
        std::copy_n(media_packet.begin(), 8u, std::back_inserter(this->fec_bit_string_));
        begin = this->fec_bit_string_.begin(), end = this->fec_bit_string_.end();
    } else {
        std::transform(begin, begin + 8, media_packet.begin(), begin, f);
    }

    namespace karma = boost::spirit::karma;
    if (this->fec_bit_string_.size() == 8) {
        karma::generate(std::back_inserter(this->fec_bit_string_), karma::big_word, media_packet.size() - 12u);
        begin = this->fec_bit_string_.begin(), end = this->fec_bit_string_.end();
    } else {
        std::array<uint8_t, 2> len;
        karma::generate(len.begin(), karma::big_word, media_packet.size() - 12u);
        std::transform(begin + 8, begin + 10, len.begin(), begin + 8, f);
    }

    if (media_packet.size() <= this->fec_bit_string_.size() - 2) {
        std::transform(media_packet.begin() + 8, media_packet.end(), begin + 10, begin + 10, f);
        std::transform(begin + 10 + media_packet.size() - 8, end, begin + 10 + media_packet.size() - 8, [](auto a) {
            return a ^ 0u;
        });
    } else {
        std::transform(begin + 10, end, media_packet.begin() + 8, begin + 10, f);
        std::transform(media_packet.begin() + this->fec_bit_string_.size() - 2, media_packet.end(), std::back_inserter(
                this->fec_bit_string_), [](auto a) {
                           return a ^ 0u;
                       }
        );
        begin = this->fec_bit_string_.begin(), end = this->fec_bit_string_.end();
    }


    if (this->current_fec_k_-- >= 1) {
        return std::shared_ptr<std::vector<uint8_t>>{};
    }
    this->current_fec_k_ = this->fec_k_;

    auto packet = std::make_shared<std::vector<uint8_t>>();
    auto inserter = std::back_inserter(*packet);
    uint8_t octet_buffer{0u};
    octet_buffer = *begin++ & 0b00111111;
    if (this->fec_k_ > 16) {
        octet_buffer |= 0b01000000;
    }
    *inserter++ = (octet_buffer);
    *inserter++ = *begin++;

    std::array<uint8_t, 2> sn_base;
    karma::generate(sn_base.begin(), karma::big_word, this->fec_seq_base);
    std::copy_n(begin, 2, inserter);
    std::advance(begin, 2);
    std::copy_n(begin, 6, inserter);
    std::advance(begin, 6);

    std::array<uint8_t, 2> protection_len;
    karma::generate(protection_len.begin(), karma::big_word, this->fec_bit_string_.size() - 12);
    std::copy(protection_len.begin(), protection_len.end(), inserter);

    std::array<uint8_t, 6> mask;

    for (unsigned u = 0; u < this->fec_k_; ++u) {
        mask.at(u / 8u) |= 0b10000000 >> u % 8u;
    }
    if (this->fec_k_ > 16) {
        std::copy(mask.begin(), mask.end(), inserter);
    } else {
        std::copy(mask.begin(), mask.begin() + 2, inserter);
    }

    std::copy(this->fec_bit_string_.begin() + 12, this->fec_bit_string_.end(), inserter);
    this->fec_bit_string_.clear();


    return packet;
}
