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

namespace rtsp {

class rtsp_client final {
public:
    using jpeg_frame = std::vector<uint8_t>;

    rtsp_client(std::string url, std::function<void(rtsp::rtsp_client::jpeg_frame)> frame_handler,
                std::function<void(std::exception &)> error_handler = [](auto) {},
                std::function<void(const std::string &)>
                log_handler = [](auto) {}
    );

    ~rtsp_client();


    using packet_count_t = uint32_t;

/*!
 *
 * @param received_packet_handler Gets received/expected rtp packet count for session
 * @param fec_packet_handler Gets corrected/not correctable rtp packet count for session
 */
    void set_rtp_packet_stat_callbacks(std::function<void(packet_count_t, packet_count_t)> received_packet_handler,
                                       std::function<void(packet_count_t, packet_count_t)> fec_packet_handler);

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
     * @param request Request to send, cseq ist required
     * @param response_callback Will be called when a non 1xx response to the request was received, already
     * sychronized with strand
     */
    void send_request(rtsp::request request, std::function<void(rtsp::response)> reponse_handler);
};

}

#endif //RTSP_RTSP_CLIENT_HPP
