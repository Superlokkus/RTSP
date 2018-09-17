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
          udp_socket_{io_context_, boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(), udp_port_number}},
          error_handler_{std::move(error_handler)} {
    if (!fileapi::exists(ressource_root_))
        throw std::runtime_error("Ressource root not existing");

    const auto thread_count{std::max<unsigned>(std::thread::hardware_concurrency(), 1)};
    std::generate_n(std::back_inserter(this->io_run_threads_),
                    thread_count,
                    [this]() { return std::thread{&rtsp_server_::io_run_loop, this}; });

}

rtsp::rtsp_server_::~rtsp_server_() {
    work_guard_.reset();
    std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) { thread.join(); });
}

void rtsp::rtsp_server_::io_run_loop() {
    while (true) {
        try {
            this->io_context_.run();
            break;
        } catch (std::exception &e) {
            this->error_handler_(e);

        }
    }

}
