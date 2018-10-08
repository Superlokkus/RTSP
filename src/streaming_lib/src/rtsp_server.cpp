/*! @file rtsp_server.cpp
 *
 */

#include <algorithm>
#include <iterator>

#include <rtsp_server.hpp>
#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

struct rtsp::rtsp_server::tcp_connection : std::enable_shared_from_this<tcp_connection> {
    tcp_connection() = delete;

    /*! Constructs a new RTSP tcp connection
     *
     * @param io_context Boost asio io context to use for async calls on its socket
     * @param server_state RTSP server state for side effects of the tcp connection
     */
    tcp_connection(boost::asio::io_context &io_context, server::rtsp_server_state &server_state)
            : socket_(io_context), server_state_(server_state) {

    }

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    /*! @brief Starts the session by reading in the rtsp header which should be blank line terminated
     *
     * Calls @ref header_read as handler for the read in raw handler
     */
    void start() {
        boost::asio::async_read_until(this->socket_, this->in_streambuf_, boost::asio::string_view{"\r\n\r\n"},
                                      [me = shared_from_this()](
                                              const boost::system::error_code &error,
                                              std::size_t bytes_transferred) {
                                          return me->header_read(error, bytes_transferred);
                                      });
    }

    /*! @brief Parses the raw header and either sends a bad request response on failure or lets the
     * internal server state from construction handle the response and sends it's request
     * @param error Error from boost asio
     * @param bytes_transferred Bytes transfered from boost asio
     */
    void header_read(const boost::system::error_code &error,
                     std::size_t bytes_transferred) {
        if (error)
            throw std::runtime_error{error.message()};

        rtsp_request_grammar<boost::spirit::multi_pass<std::istreambuf_iterator<char>>> request_grammar{};
        rtsp::request request;
        std::istream stream(&this->in_streambuf_);
        auto begin = boost::spirit::make_default_multi_pass(std::istreambuf_iterator<char>(stream));
        const bool valid = boost::spirit::qi::phrase_parse(begin,
                                                           boost::spirit::make_default_multi_pass(
                                                                   std::istreambuf_iterator<char>()),
                                                           request_grammar, ns::space, request);

        rtsp::response response;
        if (!valid) {
            auto reason = std::string{"Bad Request"};
            response = rtsp::response{server_state_.rtsp_major_version, server_state_.rtsp_minor_version,
                                      400, std::move(reason)};
        } else {
            response = server_state_.handle_incoming_request(request);
        }

        rtsp::generate_response(std::back_inserter(last_response_string_), response);

        this->socket_.async_send(boost::asio::buffer(last_response_string_), [me = shared_from_this()]
                (const boost::system::error_code &error,
                 std::size_t bytes_transferred) {
            return me->response_sent(error, bytes_transferred);
        });
    }

    /*! @brief Handler stub to check for errors and upheld lifetime while sending async
     *
     * @param error Error from asio
     * @param bytes_transferred Bytes sent by asio
     */
    void response_sent(const boost::system::error_code &error,
                       std::size_t bytes_transferred) {
        if (error)
            throw std::runtime_error{error.message()};
        if (bytes_transferred != last_response_string_.size())
            throw std::runtime_error{"TCP bytes sent not equal"};

    }

private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf in_streambuf_;
    server::rtsp_server_state &server_state_;
    std::string last_response_string_;
};

rtsp::rtsp_server::rtsp_server(boost::filesystem::path ressource_root,
                               uint16_t port_number,
                               std::function<void(std::exception &)> error_handler)
        : ressource_root_{std::move(ressource_root)},
          io_context_{BOOST_ASIO_CONCURRENCY_HINT_SAFE},
          work_guard_{boost::asio::make_work_guard(io_context_)},
          udp_v4_socket_{std::forward_as_tuple(
                  boost::asio::ip::udp::socket{io_context_,
                                               boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(),
                                                                              port_number}},
                  boost::asio::io_context::strand{io_context_},
                  udp_buffer{},
                  boost::asio::ip::udp::endpoint{}
          )},
          udp_v6_socket_{std::forward_as_tuple(
                  boost::asio::ip::udp::socket{io_context_,
                                               boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v6(),
                                                                              port_number}},
                  boost::asio::io_context::strand{io_context_},
                  udp_buffer{},
                  boost::asio::ip::udp::endpoint{}
          )},
          tcp_v4_acceptor_{io_context_, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), port_number}},
          tcp_v6_acceptor_{io_context_, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v6(), port_number}},
          error_handler_{std::move(error_handler)} {

    if (!fileapi::exists(ressource_root_))
        throw std::runtime_error("Ressource root not existing");

    const auto thread_count{std::max<unsigned>(std::thread::hardware_concurrency(), 1)};

    std::generate_n(std::back_inserter(this->io_run_threads_),
                    thread_count,
                    [this]() {
                        return std::thread{&rtsp_server::io_run_loop,
                                           std::ref(this->io_context_), std::ref(this->error_handler_)};
                    });

    this->start_async_receive(this->udp_v4_socket_);
    this->start_async_receive(this->udp_v6_socket_);
    this->start_async_receive(this->tcp_v4_acceptor_);
    this->start_async_receive(this->tcp_v6_acceptor_);

}

rtsp::rtsp_server::~rtsp_server() {
    work_guard_.reset();
    io_context_.stop();
    std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) {
        if (thread.joinable()) thread.join();
    });
}

void rtsp::rtsp_server::graceful_shutdown() {
    this->work_guard_.reset();
    this->tcp_v4_acceptor_.close();
    this->tcp_v6_acceptor_.close();
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->udp_v4_socket_), [this]() {
        std::get<0>(this->udp_v4_socket_).close();
    }));
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(std::get<1>(this->udp_v6_socket_), [this]() {
        std::get<0>(this->udp_v6_socket_).close();
    }));
    std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) { thread.join(); });
}

void rtsp::rtsp_server::io_run_loop(boost::asio::io_context &context,
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

void rtsp::rtsp_server::start_async_receive(rtsp::rtsp_server::shared_udp_socket &socket) {
    std::get<0>(socket).async_receive_from(boost::asio::buffer(std::get<2>(socket)),
                                           std::get<3>(socket),
                                           boost::asio::bind_executor(std::get<1>(socket), std::bind(
                                                   &rtsp::rtsp_server::handle_incoming_udp_traffic,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::ref(socket)
                                           )));
}

void rtsp::rtsp_server::start_async_send_to(rtsp::rtsp_server::shared_udp_socket &socket,
                                            boost::asio::ip::udp::endpoint to_endpoint,
                                            std::shared_ptr<std::string> message) {
    std::get<0>(socket).async_send_to(boost::asio::buffer(*message),
                                      to_endpoint,
                                      boost::asio::bind_executor(std::get<1>(socket), [message](auto error,
                                                                                                auto bytes_transfered) {
                                                                     if (error) {
                                                                         throw std::runtime_error{error.message()};
                                                                     }
                                                                     if (bytes_transfered != message->size())
                                                                         throw std::runtime_error{std::string{"Send udp: "} +
                                                                                                  std::to_string(bytes_transfered) +
                                                                                                  " message: " + std::to_string(message->size())};
                                                                 }
                                      ));
}

void rtsp::rtsp_server::start_async_receive(boost::asio::ip::tcp::acceptor &acceptor) {
    auto new_connection = std::make_shared<tcp_connection>(acceptor.get_io_context(), this->server_state_);
    acceptor.async_accept(new_connection->socket(),
                          std::bind(&rtsp_server::handle_new_tcp_connection, this,
                                    std::placeholders::_1,
                                    std::ref(acceptor),
                                    new_connection)
    );
}

void rtsp::rtsp_server::handle_new_tcp_connection(const boost::system::error_code &error,
                                                  boost::asio::ip::tcp::acceptor &acceptor,
                                                  std::shared_ptr<tcp_connection> new_connection) {
    if (error)
        throw std::runtime_error(error.message());
    new_connection->start();
    start_async_receive(acceptor);
}

void rtsp::rtsp_server::handle_incoming_udp_traffic(const boost::system::error_code &error,
                                                    std::size_t received_bytes,
                                                    rtsp::rtsp_server::shared_udp_socket &incoming_socket) {
    if (error)
        throw std::runtime_error{error.message()};

    auto data = std::make_shared<std::vector<char>>();

    std::copy_n(std::get<2>(incoming_socket).cbegin(), received_bytes, std::back_inserter(*data));
    boost::asio::ip::udp::endpoint received_from_endpoint = std::get<3>(incoming_socket);

    boost::asio::post(std::get<1>(incoming_socket).get_io_context(),
                      std::bind(&rtsp::rtsp_server::handle_new_incoming_message,
                                data, std::ref(incoming_socket),
                                received_from_endpoint,
                                std::ref(this->server_state_))
    );

    start_async_receive(incoming_socket);
}

void rtsp::rtsp_server::handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                                    shared_udp_socket &socket_received_from,
                                                    boost::asio::ip::udp::endpoint received_from_endpoint,
                                                    server::rtsp_server_state &server_state) {
    rtsp_request_grammar<std::vector<char>::const_iterator> request_grammar{};
    request request{};

    auto begin = message->cbegin();
    const auto valid = boost::spirit::qi::phrase_parse(begin, message->cend(),
                                                       request_grammar, ns::space, request);

    rtsp::response response;
    if (!valid) {
        auto reason = std::string("Bad Request, could not parse after \"") + std::string(begin, message->cend()) + "\"";
        response = rtsp::response{server_state.rtsp_major_version, server_state.rtsp_minor_version,
                                  400, std::move(reason)};
    } else {
        response = server_state.handle_incoming_request(request);
    }
    auto response_string = std::make_shared<std::string>();
    rtsp::generate_response(std::back_inserter(*response_string), response);

    rtsp_server::start_async_send_to(socket_received_from, received_from_endpoint, response_string);
}
