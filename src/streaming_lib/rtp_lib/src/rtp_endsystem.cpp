/*! @file rtp_endsystem.cpp
 *
 */

#include <rtp_endsystem.hpp>

#include <array>
#include <algorithm>
#include <bitset>

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
    this->fec_generator_ = fec_generator{fec_k};
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

    if (this->handle_new_jpeg_packet(*message)) {

    } else if (this->handle_new_fec_packet(*message)) {

    } else {
        BOOST_LOG_TRIVIAL(trace) << "Got unparsable packet on RTP channel";
    }

    if (this->statistics_handler_) {
        this->statistics_handler_(statistics{this->jpeg_sequence_utility_->received_packets(),
                                             this->jpeg_sequence_utility_->expected(),
                                             this->corrected_, this->uncorrectable_
        });
    }

}

bool rtp::unicast_jpeg_rtp_receiver::handle_new_jpeg_packet(const std::vector<char> &message) {
    rtp::packet::custom_jpeg_packet_grammar<std::vector<char>::const_iterator> custom_jpeg_grammar{};
    auto begin = message.cbegin(), end = message.cend();

    rtp::packet::custom_jpeg_packet jpeg_packet;
    auto success = boost::spirit::qi::parse(begin, end, custom_jpeg_grammar, jpeg_packet);
    if (!success)
        return false;

    if (jpeg_packet.header.ssrc != this->ssrc_) {
        throw std::runtime_error{"Got jpeg rtp packet with wrong ssrc, dropping"};
    }

    if (jpeg_packet.data.at(jpeg_packet.data.size() - 2) != 0xFFu ||
        jpeg_packet.data.at(jpeg_packet.data.size() - 1) != 0xD9u) {
        BOOST_LOG_TRIVIAL(error) << "Invalid jpeg data!";
    }

    if (!jpeg_sequence_utility_) { //First packet
        jpeg_sequence_utility_ = boost::make_optional<rtp_receiver_squence_utility>(jpeg_packet.header.sequence_number);
    }
    if (jpeg_sequence_utility_->update_seq(jpeg_packet.header.sequence_number) == 0) {
        BOOST_LOG_TRIVIAL(trace) << "JPEG Sequence number invalid: " << jpeg_packet.header.sequence_number <<
                                 "Expected " << jpeg_sequence_utility_->highest_seq_seen();
        return true;
    }

    std::vector<uint8_t> raw{message.cbegin(), message.cend()};
    this->jpeg_packet_incoming_buffer_.emplace_back(std::move(jpeg_packet), std::move(raw));

    if (this->jpeg_packet_incoming_buffer_.size() < this->buffer_size) {
        return true;
    }
    const auto gap =
            (this->jpeg_packet_incoming_buffer_.end() - this->media_packet_delay)->first.header.sequence_number -
            (this->jpeg_packet_incoming_buffer_.end() -
             (this->media_packet_delay + 1))->first.header.sequence_number;
    if (gap == 2) {
        BOOST_LOG_TRIVIAL(trace) << "Gonna FEC!";
        if (this->recover_packet_by_fec()) {
            ++this->corrected_;
        } else {
            ++this->uncorrectable_;
        }
    } else if (gap > 2) {
        BOOST_LOG_TRIVIAL(trace) << "GAP >2";
        ++this->uncorrectable_;
    }

    this->jpeg_packet_frame_buffer_.push_back(jpeg_packet_incoming_buffer_.front().first);
    this->jpeg_packet_incoming_buffer_.pop_front();

    return true;
}

bool rtp::unicast_jpeg_rtp_receiver::handle_new_fec_packet(const std::vector<char> &message) {
    auto begin = message.cbegin(), end = message.cend();
    rtp::packet::custom_fec_packet fec_packet;
    rtp::packet::custom_fec_packet_grammar<std::vector<char>::const_iterator> custom_fec_grammar{};
    auto success = boost::spirit::qi::parse(begin, end, custom_fec_grammar, fec_packet);
    if (!success)
        return false;
    if (fec_packet.header.payload_type_field != 100u)
        return false;

    if (fec_packet.header.ssrc != this->ssrc_) {
        throw std::runtime_error{"Got fec rtp packet with wrong ssrc, dropping"};
    }
    if (!fec_sequence_utility_) { //First packet
        fec_sequence_utility_ = boost::make_optional<rtp_receiver_squence_utility>(fec_packet.header.sequence_number);
    }
    if (fec_sequence_utility_->update_seq(fec_packet.header.sequence_number) == 0) {
        BOOST_LOG_TRIVIAL(trace) << "FEC Sequence number invalid: " << fec_packet.header.sequence_number <<
                                 "Expected " << fec_sequence_utility_->highest_seq_seen();
        return true;
    }

    std::vector<uint8_t> raw{message.cbegin(), message.cend()};
    this->fec_packet_incoming_buffer_.emplace_back(std::move(fec_packet), std::move(raw));

    if (this->fec_packet_incoming_buffer_.size() > this->buffer_size)
        this->fec_packet_incoming_buffer_.pop_front();

    return true;
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

bool rtp::unicast_jpeg_rtp_receiver::recover_packet_by_fec() {
    const auto to_recover_seq_num =
            (this->jpeg_packet_incoming_buffer_.end() - this->media_packet_delay)->first.header.sequence_number - 1;

    auto fec_candidate = std::find_if(this->fec_packet_incoming_buffer_.crbegin(),
                                      this->fec_packet_incoming_buffer_.crend(),
                                      [to_recover_seq_num](const auto &fec_packet) -> bool {
                                          if (to_recover_seq_num < fec_packet.first.header.sn_base_field ||
                                              to_recover_seq_num > fec_packet.first.header.sn_base_field + 48)
                                              return false;
                                          const auto seq_i = to_recover_seq_num - fec_packet.first.header.sn_base_field;
                                          const std::bitset<64> mask_field{fec_packet.first.levels.at(
                                                  0).first.mask_field}; //For more protection levels we would iterate here
                                          if (!mask_field.test(63 - seq_i))
                                              return false;

                                          //One should compare here for the bits in the mask and the media packets in buffer

                                          return true;
                                      });
    if (fec_candidate == this->fec_packet_incoming_buffer_.crend())
        return false;

    auto fec_packet = *fec_candidate;
    const auto seq_i = to_recover_seq_num - fec_packet.first.header.sn_base_field;
    const std::bitset<64> mask_field{fec_packet.first.levels.at(
            0).first.mask_field};
    std::vector<std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>>> media_packets;

    for (unsigned u = 0; u < seq_i; ++u) {
        if (mask_field.test(63 - u)) {
            const auto media_it = (this->jpeg_packet_incoming_buffer_.end() - this->media_packet_delay) - seq_i + u;
            if (media_it->first.header.sequence_number != to_recover_seq_num - seq_i + u) {
                BOOST_LOG_TRIVIAL(trace) << "media_it->header.sequence_number != to_recover_seq_num - seq_i + u"
                                         << media_it->first.header.sequence_number << " "
                                         << to_recover_seq_num - seq_i + u;
                return false;
            }
            media_packets.push_back(*media_it);
        }
    }
    for (unsigned u = seq_i + 1; u <= 63; ++u) {
        if (mask_field.test(63 - u)) {
            const auto media_it =
                    (this->jpeg_packet_incoming_buffer_.end() - this->media_packet_delay) + (u - seq_i - 1);
            if (media_it->first.header.sequence_number != to_recover_seq_num + (u - seq_i)) {
                BOOST_LOG_TRIVIAL(trace) << "media_it->header.sequence_number != to_recover_seq_num + (u - seq_i)"
                                         << media_it->first.header.sequence_number << " "
                                         << to_recover_seq_num + (u - seq_i);
                return false;
            }
            media_packets.push_back(*media_it);
        }
    }
    auto packet = fec_reconstruction(fec_packet, media_packets);
    //Remove padding
    std::array<uint8_t, 2> magic_bytes{0xFFu, 0xD9};
    auto end_of_frame = std::find_end(packet.first.data.begin(), packet.first.data.end(), magic_bytes.begin(),
                                      magic_bytes.end());
    if (end_of_frame != packet.first.data.end()) {
        std::advance(end_of_frame, 2);
        const auto padding_count = std::distance(end_of_frame, packet.first.data.end());
        packet.first.data.erase(end_of_frame, packet.first.data.end());
        packet.second.erase(packet.second.end() - padding_count, packet.second.end());
    }

    this->jpeg_packet_incoming_buffer_.insert((this->jpeg_packet_incoming_buffer_.end() - this->media_packet_delay),
                                              packet);

    return true;
}

std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>> rtp::unicast_jpeg_rtp_receiver::fec_reconstruction(
        const std::pair<rtp::packet::custom_fec_packet, std::vector<uint8_t>> &fec_packet,
        const std::vector<std::pair<rtp::packet::custom_jpeg_packet, std::vector<uint8_t>>

        > &media_packets) {
    const uint16_t fec_k = media_packets.size() + 1;
    fec_generator fec_gen{fec_k};
    for (
        const auto &media_packet
            : media_packets) {
        fec_gen.
                generate_next_fec_packet(media_packet
                                                 .second, media_packet.first.header.sequence_number);
    }
    std::vector<uint8_t> zeros(12);
    auto step_1 = fec_gen.generate_next_fec_packet(zeros, media_packets.at(0).first.header.sequence_number);

    auto f = [](uint8_t a, uint8_t b) -> uint8_t {
        return a ^ b;
    };

    std::vector<uint8_t> bit_string;

    std::transform(step_1->begin(), step_1->begin() + 10,
                   fec_packet.second.begin() + 12 + fec_packet.first.header.csrc * 4,
                   std::back_inserter(bit_string), f);
    rtp::packet::standard_rtp_header step_4{};
    uint8_t octet_buffer = bit_string.at(0);
    step_4.padding_set = (octet_buffer & 0b00100000) >> 5;
    step_4.extension_bit = (octet_buffer & 0b00010000) >> 4;
    step_4.csrc = (octet_buffer & 0b00001111);
    octet_buffer = bit_string.at(1);
    step_4.marker = (octet_buffer & 0b10000000) >> 7;
    step_4.payload_type_field = (octet_buffer & 0b01111111);

    namespace qi = boost::spirit::qi;
    qi::parse(bit_string.begin() + 4, bit_string.begin() + 8, qi::big_dword, step_4.timestamp);
    uint16_t len_recovery{};
    qi::parse(bit_string.begin() + 8, bit_string.begin() + 10, qi::big_dword, len_recovery);

    bit_string.clear();
    const uint16_t protection_len = fec_packet.first.levels.at(0).first.protection_length_field;
    std::copy(fec_packet.first.levels.at(0).second.cbegin(), fec_packet.first.levels.at(0).second.cend(),
              std::back_inserter(bit_string));
    std::fill_n(std::back_inserter(bit_string), protection_len - bit_string.size(), 0u);
    for (const auto &media_packet : media_packets) {
        std::transform(media_packet.second.begin() + 12, media_packet.second.end(), bit_string.begin(),
                       bit_string.begin(), f);
    }
    namespace karma = boost::spirit::karma;
    std::vector<uint8_t> rtp_header_buffer;
    rtp::packet::standard_rtp_header_generator<std::back_insert_iterator<std::vector<uint8_t>>> rtp_header_grammar{};
    boost::spirit::karma::generate(std::back_inserter(rtp_header_buffer), rtp_header_grammar, step_4);

    std::copy(bit_string.begin(), bit_string.end(), std::back_inserter(rtp_header_buffer));

    rtp::packet::custom_jpeg_packet_grammar<std::vector<uint8_t>::const_iterator> custom_jpeg_grammar{};
    auto begin = rtp_header_buffer.cbegin(), end = rtp_header_buffer.cend();

    rtp::packet::custom_jpeg_packet jpeg_packet;
    auto success = boost::spirit::qi::parse(begin, end, custom_jpeg_grammar, jpeg_packet);
    if (!success) {
        BOOST_LOG_TRIVIAL(trace) << "Could not fec";
    }


    return std::make_pair(jpeg_packet, rtp_header_buffer);
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

rtp::fec_generator::fec_generator(uint16_t fec_k) :
        fec_k_(fec_k), current_fec_k_(fec_k) {
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


    if (this->current_fec_k_-- > 1) {
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
    this->fec_seq_base = 0;
    std::copy_n(sn_base.begin(), 2, inserter);
    std::advance(begin, 2);
    std::copy_n(begin, 6, inserter);
    std::advance(begin, 6);

    std::array<uint8_t, 2> protection_len;
    karma::generate(protection_len.begin(), karma::big_word, this->fec_bit_string_.size() - 12);
    std::copy(protection_len.begin(), protection_len.end(), inserter);

    std::array<uint8_t, 6> mask;
    mask.fill(0u);

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
