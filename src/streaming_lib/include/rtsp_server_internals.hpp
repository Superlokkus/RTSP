/*!
 * @file rtsp_server_internals.hpp
 */

#ifndef STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP
#define STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP

#include <cstdint>
#include <utility>
#include <functional>
#include <unordered_map>
#include <mutex>

#include <rtsp_message.hpp>
#include <rtsp_session.hpp>

namespace rtsp {
namespace server {

/*! @brief This class handles the rtsp dialog from a server perspective, on the level of parsed rtsp messages
 *  while maintaining the necessary state and side-effects in a thread safe manner
 *
 *  This class will likely be a bottle neck but is not optimized yet, for demonstration purposes
 *  Likely good solution would be hashing sessions to specific threads, and let reponses be given by via
 *  a callback or future
 *
 *  @par Thread Safety
 *  Distinct Objects: Safe
 *  Shared Objectts: Safe for @ref handle_incoming_request method and static member functions
 */
class rtsp_server_state final {
public:
    explicit rtsp_server_state();

    const uint_fast16_t rtsp_major_version = 1;
    const uint_fast16_t rtsp_minor_version = 0;


    response handle_incoming_request(const request &request);

    /*! @brief The duty of generating session ids and destroying them is done by @ref rtsp_server_state
     *
     */
    using method_implementation = std::function<
            std::pair<response, body>(rtsp::rtsp_session &, const internal_request &)
    >;

    std::pair<response, body> options(const internal_request &request) const;

private:
    std::mutex sessions_mutex_;
    std::unordered_map<rtsp::session_identifier, rtsp::rtsp_session> sessions_;
    std::unordered_map<rtsp::method, method_implementation> methods_;

};

}
}

#endif //STREAMING_LIB_RTSP_SERVER_INTERNALS_HPP
