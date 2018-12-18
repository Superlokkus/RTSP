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
        work_guard_{boost::asio::make_work_guard(io_context_)},
        strand_(io_context_),
        rtsp_socket_(io_context_),
        rtsp_resolver_(io_context_) {
    this->log_handler_(std::string{"rtsp_client creating for URL: "} + url);
    const auto thread_count{std::max<unsigned>(std::thread::hardware_concurrency(), 1)};
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
                [then] (auto error) {
            if (error)
                throw std::runtime_error{std::string{"Could not connect: "} + error.message()};
            then();
        }));

    }));
}

void rtsp::rtsp_client::setup() {
    boost::asio::dispatch(this->io_context_, boost::asio::bind_executor(this->strand_, [this]() {

        auto send_setup_request = [this]() {
            rtsp::request request{"SETUP", this->rtsp_settings_.original_url, 1, 0,
                                  {
                                          {"CSeq", std::to_string(this->session_.next_cseq++)}
                                          //Cursor insert transport header
                                  }};

        };

        if (!this->rtsp_socket_.is_open()) {
            this->open_socket(send_setup_request);
        } else {
            send_setup_request();
        }
    }));
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
