/*! @file rtsp_client.cpp
 *
 */

#include <rtsp_client.hpp>

#include <algorithm>
#include <iterator>

#include <boost/optional.hpp>

#include <rtsp_message.hpp>

rtsp::rtsp_client::rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                               std::function<void(std::exception &)> error_handler,
                               std::function<void(const std::string &)> log_handler)
        :
        frame_handler_(std::move(frame_handler)),
        error_handler_(std::move(error_handler)),
        log_handler_(std::move(log_handler)),
        io_context_{BOOST_ASIO_CONCURRENCY_HINT_SAFE},
        work_guard_{boost::asio::make_work_guard(io_context_)} {
    this->log_handler_(std::string{"rtsp_client creating for URL: "} + url);
    const auto thread_count{std::max<unsigned>(std::thread::hardware_concurrency(), 1)};

    try {
        std::generate_n(std::back_inserter(this->io_run_threads_),
                        thread_count,
                        [this]() {
                            return std::thread{&rtsp_client::io_run_loop,
                                               std::ref(this->io_context_), std::ref(this->error_handler_)};
                        });
        this->process_url(url);
        this->log_handler_(std::string{"rtsp_client created with"} + this->rtsp_settings_.host +
                           +":" + std::to_string(this->rtsp_settings_.port) + this->rtsp_settings_.remainder);
    } catch (...) {
        work_guard_.reset();
        io_context_.stop();
        std::for_each(this->io_run_threads_.begin(), this->io_run_threads_.end(), [](auto &thread) {
            if (thread.joinable()) thread.join();

        });
        throw;
    }

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


void rtsp::rtsp_client::set_rtp_packet_stat_callbacks(
        std::function<void(packet_count_t, packet_count_t)> received_packet_handler,
        std::function<void(packet_count_t, packet_count_t)> fec_packet_handler) {

}

void rtsp::rtsp_client::process_url(const std::string &url) {
    using namespace boost::spirit;
    std::string method, host, remainder;
    boost::optional<uint16_t> port{};
    bool parsed = qi::parse(url.cbegin(), url.cend(),
                            qi::omit[*ns::space]
                                    >> (ns::string("rtsp:") | ns::string("rtspu:"))
                                    >> qi::lit("//") >> *(ns::alnum | ns::char_(".-"))
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

}

void rtsp::rtsp_client::setup() {

}

void rtsp::rtsp_client::play() {

}

void rtsp::rtsp_client::pause() {

}

void rtsp::rtsp_client::teardown() {

}

void rtsp::rtsp_client::option() {

}

void rtsp::rtsp_client::describe() {

}
