/*! @file rtsp_server.cpp
 *
 */

#include <rtsp_server_.hpp>
#include <algorithm>
#include <iterator>

rtsp::rtsp_server_::rtsp_server_(boost::filesystem::path ressource_root,
                                 uint16_t udp_port_number,
                                 std::function<void(std::exception &)> error_handler)
        : ressource_root_{std::move(ressource_root)},
          io_context_{BOOST_ASIO_CONCURRENCY_HINT_SAFE},
          work_guard_{boost::asio::make_work_guard(io_context_)},
          udp_v4_socket_{std::forward_as_tuple(
                  boost::asio::ip::udp::socket{io_context_,
                                               boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(),
                                                                              udp_port_number}},
                  boost::asio::io_context::strand{io_context_},
                  udp_buffer{},
                  boost::asio::ip::udp::endpoint{}
          )},
          udp_v6_socket_{std::forward_as_tuple(
                  boost::asio::ip::udp::socket{io_context_,
                                               boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v6(),
                                                                              udp_port_number}},
                  boost::asio::io_context::strand{io_context_},
                  udp_buffer{},
                  boost::asio::ip::udp::endpoint{}
          )},
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

    std::get<0>(udp_v4_socket_).async_receive_from(boost::asio::buffer(std::get<2>(udp_v4_socket_)),
                                                   std::get<3>(udp_v4_socket_),
                                                   boost::asio::bind_executor(std::get<1>(udp_v4_socket_), std::bind(
                                                           &rtsp::rtsp_server_::handle_incoming_udp_traffic,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           std::ref(udp_v4_socket_)
                                                   )));

    std::get<0>(udp_v6_socket_).async_receive_from(boost::asio::buffer(std::get<2>(udp_v6_socket_)),
                                                   std::get<3>(udp_v6_socket_),
                                                   boost::asio::bind_executor(std::get<1>(udp_v6_socket_), std::bind(
                                                           &rtsp::rtsp_server_::handle_incoming_udp_traffic,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           std::ref(udp_v6_socket_)
                                                   )));


}

rtsp::rtsp_server_::~rtsp_server_() {
    work_guard_.reset();
    io_context_.stop();
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

void rtsp::rtsp_server_::handle_incoming_udp_traffic(const boost::system::error_code &error,
                                                     std::size_t received_bytes,
                                                     rtsp::rtsp_server_::shared_udp_socket &incoming_socket) {
    if (error)
        throw std::runtime_error{error.message()};

    auto data = std::make_shared<std::vector<char>>();

    std::copy_n(std::get<2>(incoming_socket).cbegin(), received_bytes, std::back_inserter(*data));

    handle_new_request(data);

    std::get<0>(incoming_socket).async_receive_from(boost::asio::buffer(std::get<2>(incoming_socket)),
                                                    std::get<3>(incoming_socket),
                                                    boost::asio::bind_executor(std::get<1>(incoming_socket), std::bind(
                                                            &rtsp::rtsp_server_::handle_incoming_udp_traffic,
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            std::ref(incoming_socket)
                                                    )));

}

void rtsp::rtsp_server_::handle_new_request(std::shared_ptr<std::vector<char>> data) {
    std::move(data->begin(), data->end(), std::ostream_iterator<char>(std::cout));
    std::cout << std::endl;
}
