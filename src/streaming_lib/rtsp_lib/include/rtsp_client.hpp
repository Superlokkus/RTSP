/*! @file rtsp_client.hpp
 *
 */

#ifndef RTSP_RTSP_CLIENT_HPP
#define RTSP_RTSP_CLIENT_HPP

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>
#include <string>
#include <thread>
#include <random>

#include <boost/asio.hpp>

#include <rtsp_session.hpp>
#include <rtsp_definitions.hpp>
#include <rtp_endsystem.hpp>

namespace rtsp {

/*! @brief Class to get jpeg frames from an RTSP 1 source, in order to play use teardown after that setup
 *
 * In general uses its own threads and functions are returning immediately
 */
class rtsp_client final {
public:
    using jpeg_frame = std::vector<uint8_t>;

    /*!
     *
     * @param url RTSP URL to host which should be used to connect to
     * @param frame_handler Will be called whenever a new frame should be displayed (timed)
     * @param error_handler Runtime errors from network or server, or incorrect state for rtsp methods
     * @param log_handler Diagnostic and rtsp dialogs
     */
    rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                std::function<void(std::exception &)> error_handler = [](auto) {},
                std::function<void(const std::string &)>
                log_handler = [](auto) {}
    );

    void set_rtp_statistics_handler(std::function<void(rtp::unicast_jpeg_rtp_receiver::statistics)>
                                    rtp_statistics_handler);

    ~rtsp_client();

    void setup();

    void play();

    void pause();

    void teardown();

    void option();

    void describe();

private:
    std::function<void(std::exception &)> error_handler_;
    std::function<void(const std::string &)> log_handler_;
    std::function<void(jpeg_frame)> frame_handler_;
    std::function<void(rtp::unicast_jpeg_rtp_receiver::statistics)> rtp_statistics_handler_;

    std::default_random_engine default_random_engine_;
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::vector<std::thread> io_run_threads_;
    boost::asio::io_context::strand strand_;

    boost::asio::ip::tcp::socket rtsp_socket_;
    boost::asio::ip::tcp::resolver rtsp_resolver_;

    struct {
        std::string host;
        std::string remainder;
        uint16_t port{};
        std::string original_url;
    } rtsp_settings_;

    rtsp_client_session session_;
    std::unordered_map<rtsp::cseq, std::function<void(rtsp::response)>> outstanding_requests_;
    boost::asio::streambuf in_streambuf_;
    std::string parser_buffer_;

    std::unique_ptr<rtp::unicast_jpeg_rtp_receiver> rtp_receiver_;

    /*! For use in the constructor, unsychronized
     *
     * @param url
     */
    void process_url(const std::string &url);

    static void
    io_run_loop(boost::asio::io_context &context, const std::function<void(std::exception &)> &error_handler);


    /*! For use in already synchronized methods, unsychronized
     * @todo Currently just returns the first resolved entry, complex search working rtsp option probed entry logic
     * could improve robustness
     * @param then Will be called after socket has been connected, sychronized with strand
     */
    template<typename callback>
    void open_socket(callback &&then);

    /*! For use in already synchronized methods, unsychronized
     * @param request Request to send, "CSeq" will be inserted as first header
     * @param response_callback Will be called when a non 1xx response to the request was received, already
     * sychronized with strand
     */
    void send_request(rtsp::request request, std::function<void(rtsp::response)> reponse_handler);

    /*! @brief Parses the raw header
     * @param error Error from boost asio
     * @param bytes_transferred Bytes transfered from boost asio
     */
    void header_read(const boost::system::error_code &error,
                     std::size_t bytes_transferred);

    /*!
     * @param transport_header Header string to parse
     * @returns To be used client port and ssrc
     * @throws std::runtime_error when parsing fails or information is missing or not suitable
     */
    static std::tuple<uint16_t, uint32_t> process_server_transport_header(const rtsp::string &transport_header);
};

}

#endif //RTSP_RTSP_CLIENT_HPP
