/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <rtp_endsystem.hpp>

namespace rtsp {

using session_identifier = boost::uuids::uuid;

template<typename T>
struct rtsp_session {
    enum struct state {
        init,
        ready,
        playing,
        recording,
    };

    rtsp_session() = default;

    /*! This constructor accepts a start_state different from init
     * @throws std::runtime_error In case start_state is an invalid state to begin with
     * @param start_state State the session begins with
     */
    rtsp_session(state start_state) : state_(start_state) {
            if (state_ != state::init || state_ != state::ready)
                throw std::runtime_error("Invalid start state");
    }

    static session_identifier make_new_session_identifier() {
        thread_local boost::uuids::random_generator uuid_gen{};
        return uuid_gen();
    }

    session_identifier identifier() const noexcept {
        return this->session_identifier_;
    }

    state session_state() const noexcept {
        return this->state_;
    }

    void set_session_state(state new_state) {
        this->state_ = new_state;
    }

private:
    state state_{state::init};
    rtsp::session_identifier session_identifier_{make_new_session_identifier()};
};

struct rtsp_server_session final : public rtsp_session<rtsp_server_session> {

        std::unique_ptr<rtp::unicast_jpeg_rtp_session> rtp_session{};
        boost::asio::ip::address last_seen_request_address{};

    };

}

namespace std {
template<>
struct hash<rtsp::session_identifier> {
    typedef rtsp::session_identifier argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const &session_identifier) const noexcept {
        return boost::uuids::hash_value(session_identifier);
    }
};
}


#endif //RTSP_RTSP_SESSION_HPP
