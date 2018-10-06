/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace rtsp {

    struct rtsp_session final {
        using session_identifier_t = boost::uuids::uuid;

        rtsp_session() : session_identifier_(make_new_session_identifier()) {
        };


        static session_identifier_t make_new_session_identifier() {
            thread_local boost::uuids::random_generator uuid_gen{};
            return uuid_gen();
        }

        session_identifier_t session_identifier() const noexcept {
            return this->session_identifier_;
        }

    private:
        session_identifier_t session_identifier_;

    };

}

namespace std {
template<>
struct hash<rtsp::rtsp_session> {
    typedef rtsp::rtsp_session argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const &session) const noexcept {
        return boost::uuids::hash_value(session.session_identifier());
    }
};
}


#endif //RTSP_RTSP_SESSION_HPP
