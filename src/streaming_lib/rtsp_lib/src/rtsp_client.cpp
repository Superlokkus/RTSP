/*! @file rtsp_client.cpp
 *
 */

#include <rtsp_client.hpp>

#include <algorithm>
#include <iterator>

#include <boost/optional.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <rtsp_message.hpp>

rtsp::rtsp_client::rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                               std::function<void(std::exception &)> error_handler,
                               std::function<void(const std::string &)> log_handler)
        :
        default_random_engine_(std::random_device()()),
        frame_handler_(std::move(frame_handler)),
        error_handler_(std::move(error_handler)),
        log_handler_(std::move(log_handler)),
        io_context_{BOOST_ASIO_CONCURRENCY_HINT_SAFE},
        work_guard_{boost::asio::make_work_guard(io_context_)},
        strand_(io_context_),
        rtsp_socket_(io_context_),
        rtsp_resolver_(io_context_) {
    this->log_handler_(std::string{"rtsp_client creating for URL: "} + url);

    std::uniform_int_distribution<int> uniform_dist(1, 6);

    const auto thread_count{std::max<unsigned>(std::thread::hardware_concurrency() / 2u, 1u)};
    this->process_url(url);
    std::generate_n(std::back_inserter(this->io_run_threads_),
                    thread_count,
                    [this]() {
                        return std::thread{&rtsp_client::io_run_loop,
                                           std::ref(this->io_context_), std::ref(this->error_handler_)};
                    });

    this->log_handler_(std::string{"rtsp_client created with"} + this->rtsp_settings_.host +
                       +":" + std::to_string(this->rtsp_settings_.port) + this->rtsp_settings_.remainder);
}

void rtsp::rtsp_client::set_rtp_statistics_handler(
        std::function<void(rtp::unicast_jpeg_rtp_receiver::statistics)> rtp_statistics_handler) {
    boost::asio::dispatch(this->io_context_,
                          boost::asio::bind_executor(this->strand_, [this, rtp_statistics_handler]() {
                              this->rtp_statistics_handler_ = rtp_statistics_handler;
                              if (this->rtp_receiver_) {
                                  this->rtp_receiver_->set_statistics_handler(this->rtp_statistics_handler_);
                              }
                          }));
}

rtsp::rtsp_client::~rtsp_client() {
    work_guard_.reset();
    io_context_.stop();
    std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) {
        if (thread.joinable()) thread.join();
    });
    this->log_handler_("rtsp_client destroyed");
}

void rtsp::rtsp_client::io_run_loop(boost::asio::io_context &context,
                                    const std::function<void(std::exception &)> &error_handler) {
    while (true) {
        try {
            context.run();
            break;
        } catch (std::exception &e) {
            error_handler(e);
        }
    }

}

void rtsp::rtsp_client::process_url(const std::string &url) {
    using namespace boost::spirit;
    std::string method, host, remainder;
    boost::optional<uint16_t> port{};
    bool parsed = qi::parse(url.cbegin(), url.cend(),
                            qi::omit[*ns::space]
                                    >> (ns::string("rtsp:") | ns::string("rtspu:"))
                                    >> qi::lit("//") >> (*(ns::alnum | ns::char_(".-")) ||
                                                         (qi::lit("[") >> *(ns::alnum | ns::char_(".:/"))
                                                                       >> qi::lit("]")))
                                    >> -(qi::lit(":") >> qi::uint_parser<uint16_t, 10>())
                                    >> qi::lit("/") >> *ns::print,
                            method, host, port, remainder);
    if (!parsed) {
        throw std::runtime_error{std::string{"Could not read URL \""} + url + "\""};
    }
    if (method != "rtsp:") {
        throw std::runtime_error{"Only rtsp supported"};
    }

    this->rtsp_settings_.host = host;
    this->rtsp_settings_.port = port.value_or(554u);
    this->rtsp_settings_.remainder = remainder;
    this->rtsp_settings_.original_url = url;
}

template <typename callback>
void rtsp::rtsp_client::open_socket(callback&& then) {
    this->rtsp_resolver_.async_resolve(this->rtsp_settings_.host,std::to_string(this->rtsp_settings_.port),
            boost::asio::bind_executor(this->strand_,
            [this,then] (auto error, auto results) {
        if (error)
            throw std::runtime_error{error.message()};

        std::stringstream log;
        log << "Resolved " << this->rtsp_settings_.host << " Port " << this->rtsp_settings_.port << "\n";
        for (const auto& entry : results) {
            log << "Found entry \"" << entry.endpoint() << "\"";
        }
        log << "\n" << "Simple resolve logic: try to use first entry: ";

        auto first_entry = results.cbegin();
        if (first_entry == results.cend()){
            throw std::runtime_error{std::string{"Could not resolve host: "} + this->rtsp_settings_.original_url};
        }

        log << first_entry->endpoint() << "\"\n Connecting...\n";

        this->log_handler_(log.str());

        this->rtsp_socket_.async_connect(first_entry->endpoint(), boost::asio::bind_executor(this->strand_,
                [this,then] (auto error) {
            if (error)
                throw std::runtime_error{std::string{"Could not connect: "} + error.message()};

            boost::asio::async_read_until(this->rtsp_socket_, this->in_streambuf_,
                            boost::asio::string_view{"\r\n\r\n"},
                                                  boost::asio::bind_executor(this->strand_,std::bind(
                                                          &rtsp::rtsp_client::header_read,
                                                          this, std::placeholders::_1, std::placeholders::_2
                                                          )));

            then();
        }));

    }));
}

void rtsp::rtsp_client::send_request(rtsp::request request, std::function<void(rtsp::response)> reponse_handler) {
    const auto cseq = this->session_.next_cseq++;
    request.headers.insert(request.headers.begin(),{"CSeq", std::to_string(cseq)});
    auto request_string_buffer = std::make_shared<std::string>();
    rtsp::generate_request(std::back_inserter(*request_string_buffer), request);
    this->outstanding_requests_[cseq] = std::move(reponse_handler);

    this->rtsp_socket_.async_send(boost::asio::buffer(*request_string_buffer), boost::asio::bind_executor(this->strand_,
            [this,request_string_buffer]
            (const boost::system::error_code &error,
             std::size_t bytes_transferred) {
        if (error)
            throw std::runtime_error(std::string{"Could not send request "} + error.message());
        if (bytes_transferred != request_string_buffer->size())
            throw std::runtime_error("Could not send whole request");
    }));

}

void rtsp::rtsp_client::header_read(const boost::system::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        throw std::runtime_error{error.message()};
    }

    using namespace boost::spirit;
    rtsp_response_grammar<std::string::const_iterator> response_grammar{};
    rtsp::response response;
    std::istream stream(&this->in_streambuf_);

    auto stream_it = std::istreambuf_iterator<char>(stream);
    this->parser_buffer_.clear();
    std::copy_n(stream_it, bytes_transferred, std::back_inserter(parser_buffer_));
    if (bytes_transferred > 0)
        ++stream_it;

    auto begin = parser_buffer_.cbegin(), end = parser_buffer_.cend();
    this->log_handler_(parser_buffer_);
    bool valid = qi::parse(begin, end, response_grammar, response);

    if (!valid)
        this->log_handler_(std::string{"Could not read response: after \""} +
                           std::string{begin, end}
                           + "\"");

    const auto normalized_headers = rtsp::headers::normalize_headers(response.headers);
    rtsp::cseq cseq{};
    valid = qi::parse(normalized_headers.at("cseq").begin(), normalized_headers.at("cseq").end(),
                      qi::uint_parser<rtsp::cseq, 10>(), cseq);

    if (!valid)
        this->log_handler_(std::string{"Could not read CSeq: \""} +
                           normalized_headers.at("cseq") + "\"");

    if (response.status_code < 200) {
        this->log_handler_("Statuscode of response 1xx, waiting for final response");
    } else {
        try {
            this->outstanding_requests_.at(cseq)(std::move(response));
            this->outstanding_requests_.erase(cseq);
        } catch (std::out_of_range &e) {
            this->log_handler_(
                    std::string{"Handler for Cseq "} + std::to_string(cseq) + " not found because either" +
                    "already invoked and this is a legal duplicate or wrong cseq handling");
        };
    }
    boost::asio::async_read_until(this->rtsp_socket_, this->in_streambuf_,
                                  boost::asio::string_view{"\r\n\r\n"},
                                  boost::asio::bind_executor(this->strand_,std::bind(
                                          &rtsp::rtsp_client::header_read,
                                          this, std::placeholders::_1, std::placeholders::_2
                                  )));
}

void rtsp::rtsp_client::set_mkn_options(bool general_switch, double bernoulli_p, uint16_t fec_k, uint16_t fec_p) {
    if (!general_switch) {
        this->mkn_option_.reset();
        return;
    }
    this->mkn_option_ = rtsp::headers::mkn_option_parameters{bernoulli_p, fec_k, fec_p};
}

void rtsp::rtsp_client::setup() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_setup_request = [this]() {
            std::uniform_int_distribution<headers::transport::transport_spec::port_number>
                    port_distribution(49152u, 65525u);
            const auto chosen_rtp_port_by_me{port_distribution(this->default_random_engine_)};

            headers::transport my_transport{std::vector<headers::transport::transport_spec>{
                    headers::transport::transport_spec{"RTP", "AVP", boost::make_optional<rtsp::string>("UDP")}}};

            my_transport.specifications.at(0).parameters.emplace_back(string{"unicast"});
            headers::transport::transport_spec::port my_port{
                    headers::transport::transport_spec::port::port_type::client,
                    headers::transport::transport_spec::port_number{chosen_rtp_port_by_me}};
            my_transport.specifications.at(0).parameters.emplace_back(std::move(my_port));

            std::string transport_string{};
            rtsp::headers::generate_transport_header_grammar<std::back_insert_iterator<std::string>> gen_grammar{};
            bool success = boost::spirit::karma::generate(std::back_inserter(transport_string), gen_grammar, my_transport);
            if (!success)
                throw std::logic_error{"Could not generate rtsp transport string"};

            rtsp::request request{"SETUP", this->rtsp_settings_.original_url, 1, 0,
                                  {
                                          {"Transport", std::move(transport_string)}
                                  }};
            if (this->mkn_option_) {
                request.headers.emplace_back("Require", rtsp::headers::mkn_option_tag);
                std::string mkn_option_string{};
                rtsp::headers::generate_mkn_bernoulli_channel_parameter_grammar<std::back_insert_iterator<std::string>>
                        gen_grammar{};
                bool success = boost::spirit::karma::generate(std::back_inserter(mkn_option_string),
                                                              gen_grammar, *this->mkn_option_);
                if (!success)
                    throw std::logic_error{"Could not generate rtsp mln option string"};
                request.headers.emplace_back(rtsp::headers::mkn_option_header, std::move(mkn_option_string));
            }

            this->send_request(std::move(request), [this](rtsp::response response) {
                if (response.status_code < 200 || response.status_code >= 300) {
                    throw std::runtime_error(std::string("Got non ok status code: ") +
                                             std::to_string(response.status_code));
                }
                const auto normalized_headers = rtsp::headers::normalize_headers(response.headers);

                if (normalized_headers.count("session") == 0 ||
                    normalized_headers.count("transport") == 0)
                    throw std::runtime_error("Missing session or transport header in setup response");

                this->session_.set_identifier(normalized_headers.at("session"));

                this->log_handler_(std::string("Got rtsp status ") + std::to_string(response.status_code)
                                   + "\nUsing session id \"" + this->session_.identifier() + "\" "
                                   + " switching state to ready\n");


                const auto parameters = process_server_transport_header(normalized_headers.at("transport"));
                this->rtp_receiver_ = std::make_unique<rtp::unicast_jpeg_rtp_receiver>(
                        std::get<0>(parameters),
                        std::get<1>(parameters),
                        this->frame_handler_,
                        this->io_context_
                );
                this->rtp_receiver_->set_statistics_handler(this->rtp_statistics_handler_);

                this->session_.set_session_state(rtsp_client_session::state::ready);
            });
        };

        if (this->session_.session_state() != rtsp_client_session::state::init)
            throw std::runtime_error{"This rtsp client does not like to change transport while playing, "
                                     "which is also optional by rfc"};

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_setup_request);
        } else {
            send_setup_request();
        }
    }));
}

std::tuple<uint16_t, uint32_t>
rtsp::rtsp_client::process_server_transport_header(const rtsp::string &transport_header) {
    std::tuple<uint16_t, uint32_t> return_value{};
    headers::transport transport_from_server;
    using namespace boost::spirit;
    rtsp::headers::transport_grammar<std::string::const_iterator> grammar{};
    bool valid = qi::parse(transport_header.begin(),
                           transport_header.end(),
                           grammar, transport_from_server);
    if (!valid)
        throw std::runtime_error(std::string{"Could not parse transport from server: \""} +
                                 transport_header + "\"");

    auto transport_spec = std::find_if(transport_from_server.specifications.cbegin(),
                                       transport_from_server.specifications.cend(),
                                       [](const headers::transport::transport_spec &value) {
                                           return value.transport_protocol == "RTP" &&
                                                  value.profile == "AVP" &&
                                                  value.lower_transport.value_or("UDP") == "UDP" &&
                                                  value.parameters.size() > 0 &&
                                                  value.parameters.at(0).which() == 4 &&
                                                  boost::get<string>(value.parameters.at(0)) == "unicast";
                                       });
    if (transport_spec == transport_from_server.specifications.cend())
        throw std::runtime_error{"No RTP/AVP/UDP unicast transport option offered by server"};

    auto parameter_begin = transport_spec->parameters.cbegin();
    auto parameter_end = transport_spec->parameters.cend();

    auto parameter = std::find_if(parameter_begin, parameter_end,
                                  [](const auto &value) {
                                      return value.which() == 1 && (
                                              boost::get<headers::transport::transport_spec::port>(value).type ==
                                              headers::transport::transport_spec::port::port_type::client
                                              ||
                                              boost::get<headers::transport::transport_spec::port>(value).type ==
                                              headers::transport::transport_spec::port::port_type::general
                                      );
                                  });
    if (parameter == parameter_end)
        throw std::runtime_error{"No client or general port parameter found in server transport specification"};


    const auto &port = boost::get<headers::transport::transport_spec::port>(*parameter);
    if (port.port_numbers.which() == 0) {
        std::get<0>(return_value) = boost::get<headers::transport::transport_spec::port_number>(port.port_numbers);
    } else {
        std::get<0>(return_value) = boost::get<headers::transport::transport_spec::port_range>
                (port.port_numbers).first;
    }

    parameter = std::find_if(parameter_begin, parameter_end,
                             [](const auto &value) {
                                 return value.which() == 2;
                             });
    if (parameter == parameter_end)
        throw std::runtime_error{"No ssrc parameter found in server transport specification"};

    std::get<1>(return_value) = boost::get<headers::transport::transport_spec::ssrc>(*parameter);

    return return_value;
}

void rtsp::rtsp_client::play() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_request = [this]() {

            rtsp::request request{"PLAY", this->rtsp_settings_.original_url, 1, 0};
            request.headers.emplace_back("session", this->session_.identifier());
            this->send_request(std::move(request), [this](rtsp::response response) {
                this->log_handler_("Got play response");
                this->session_.set_session_state(rtsp_client_session::state::playing);
            });
        };

        if (this->session_.session_state() != rtsp_client_session::state::ready &&
            this->session_.session_state() != rtsp_client_session::state::playing)
            throw std::runtime_error{"Can request play, not in rtsp ready or playing state"};

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_request);
        } else {
            send_request();
        }
    }));
}

void rtsp::rtsp_client::pause() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_request = [this]() {

            rtsp::request request{"PAUSE", this->rtsp_settings_.original_url, 1, 0};
            request.headers.emplace_back("session", this->session_.identifier());
            this->send_request(std::move(request), [this](rtsp::response response) {
                this->log_handler_("Got pause response");
                this->session_.set_session_state(rtsp_client_session::state::ready);
            });
        };

        if (this->session_.session_state() != rtsp_client_session::state::playing)
            throw std::runtime_error{"Can request pause, not in rtsp playing state"};

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_request);
        } else {
            send_request();
        }
    }));
}

void rtsp::rtsp_client::teardown() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_request = [this]() {

            rtsp::request request{"TEARDOWN", this->rtsp_settings_.original_url, 1, 0};
            request.headers.emplace_back("session", this->session_.identifier());
            this->send_request(std::move(request), [this](rtsp::response response) {
                this->log_handler_("Got Teardown response");
                this->session_.set_session_state(rtsp_client_session::state::init);
            });
        };

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_request);
        } else {
            send_request();
        }
    }));
}

void rtsp::rtsp_client::option() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_option_request = [this]() {

            rtsp::request request{"OPTIONS", this->rtsp_settings_.original_url, 1, 0};
            this->send_request(std::move(request), [this](rtsp::response response) {
                this->log_handler_("Got options response");
            });
        };

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_option_request);
        } else {
            send_option_request();
        }
    }));
}

void rtsp::rtsp_client::describe() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {
        auto send_describe_request = [this]() {

            rtsp::request request{"DESCRIBE", this->rtsp_settings_.original_url, 1, 0};
            this->send_request(std::move(request), [this](rtsp::response response) {
                this->log_handler_("Got DESCRIBE response");
            });
        };

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_describe_request);
        } else {
            send_describe_request();
        }
    }));
}
