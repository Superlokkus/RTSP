/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <rtp_endsystem.hpp>
#include <rtsp_definitions.hpp>

namespace rtsp {

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

    string identifier() const noexcept {
        return static_cast<const T *>(this)->identifier_();
    }

    state session_state() const noexcept {
        return this->state_;
    }

    void set_session_state(state new_state) {
        this->state_ = new_state;
    }

protected:
    state state_{state::init};
};

struct rtsp_server_session final : public rtsp_session<rtsp_server_session> {
    friend class rtsp_session<rtsp_server_session>;

    std::unique_ptr<rtp::unicast_jpeg_rtp_session> rtp_session{};
    boost::asio::ip::address last_seen_request_address{};

private:
    boost::uuids::uuid session_identifier_{make_new_session_identifier_()};

    string identifier_() const noexcept {
        return boost::uuids::to_string(this->session_identifier_);
    }

    static boost::uuids::uuid make_new_session_identifier_() {
        thread_local boost::uuids::random_generator uuid_gen{};
        return uuid_gen();
    }

};

struct rtsp_client_session final : public rtsp_session<rtsp_client_session> {
    friend class rtsp_session<rtsp_client_session>;

    void set_identifier(string new_identifier) {
        this->session_identifier_ = std::move(new_identifier);
    }

    uint_fast32_t next_cseq{0u};

private:
    string session_identifier_;

    string identifier_() const noexcept {
        return this->session_identifier_;
    }
};

}

namespace std {
template<>
struct hash<rtsp::rtsp_server_session> {
    typedef rtsp::rtsp_server_session argument_type;
    typedef std::string result_type;

    result_type operator()(argument_type const &session_identifier) const noexcept {
        return session_identifier.identifier();
    }
};
}


#endif //RTSP_RTSP_SESSION_HPP
