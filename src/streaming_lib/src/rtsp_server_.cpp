/*! @file rtsp_server.cpp
 *
 */

#include <rtsp_server_.hpp>
#include <algorithm>
#include <iterator>

struct rtsp::rtsp_server_::tcp_connection : std::enable_shared_from_this<tcp_connection> {
    tcp_connection(boost::asio::io_context &io_context)
            : socket_(io_context) {

    }

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    void start() {
        std::cout << "New connection" << std::endl;
        boost::asio::ip::tcp::iostream stream(std::move(this->socket_));
        std::cout << stream.rdbuf();
    }

private:
    boost::asio::ip::tcp::socket socket_;
};

rtsp::rtsp_server_::rtsp_server_(boost::filesystem::path ressource_root,
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
                        return std::thread{&rtsp_server_::io_run_loop,
                                           std::ref(this->io_context_), std::ref(this->error_handler_)};
                    });

    this->start_async_receive(this->udp_v4_socket_);
    this->start_async_receive(this->udp_v6_socket_);
    this->start_async_receive(this->tcp_v4_acceptor_);
    this->start_async_receive(this->tcp_v6_acceptor_);

}

rtsp::rtsp_server_::~rtsp_server_() {
    work_guard_.reset();
    io_context_.stop();
    std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) {
        if (thread.joinable()) thread.join();
    });
}

void rtsp::rtsp_server_::graceful_shutdown() {
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

void rtsp::rtsp_server_::io_run_loop(boost::asio::io_context &context,
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

void rtsp::rtsp_server_::start_async_receive(rtsp::rtsp_server_::shared_udp_socket &socket) {
    std::get<0>(socket).async_receive_from(boost::asio::buffer(std::get<2>(socket)),
                                           std::get<3>(socket),
                                           boost::asio::bind_executor(std::get<1>(socket), std::bind(
                                                   &rtsp::rtsp_server_::handle_incoming_udp_traffic,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::ref(socket)
                                           )));
}

void rtsp::rtsp_server_::start_async_receive(boost::asio::ip::tcp::acceptor &acceptor) {
    auto new_connection = std::make_shared<tcp_connection>(acceptor.get_io_context());
    acceptor.async_accept(new_connection->socket(),
                          std::bind(&rtsp_server_::handle_new_tcp_connection, std::placeholders::_1,
                                    std::ref(acceptor),
                                    new_connection)
    );
}

void rtsp::rtsp_server_::handle_new_tcp_connection(const boost::system::error_code &error,
                                                   boost::asio::ip::tcp::acceptor &acceptor,
                                                   std::shared_ptr<tcp_connection> new_connection) {
    if (error)
        throw std::runtime_error(error.message());
    new_connection->start();
    start_async_receive(acceptor);
}

void rtsp::rtsp_server_::handle_incoming_udp_traffic(const boost::system::error_code &error,
                                                     std::size_t received_bytes,
                                                     rtsp::rtsp_server_::shared_udp_socket &incoming_socket) {
    if (error)
        throw std::runtime_error{error.message()};

    auto data = std::make_shared<std::vector<char>>();

    std::copy_n(std::get<2>(incoming_socket).cbegin(), received_bytes, std::back_inserter(*data));

    boost::asio::post(std::get<1>(incoming_socket).get_io_context(),
                      std::bind(&rtsp::rtsp_server_::handle_new_incoming_message,
                                data, std::ref(incoming_socket))
    );

    start_async_receive(incoming_socket);
}

void rtsp::rtsp_server_::handle_new_incoming_message(std::shared_ptr<std::vector<char>> message,
                                                     shared_udp_socket &socket_received_from) {
    std::move(message->begin(), message->end(), std::ostream_iterator<char>(std::cout));
    std::cout << std::endl;
}
