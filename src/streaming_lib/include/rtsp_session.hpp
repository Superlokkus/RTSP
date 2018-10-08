/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace rtsp {

using session_identifier = boost::uuids::uuid;
    struct rtsp_session final {

        rtsp_session() : session_identifier_(make_new_session_identifier()) {
        };


        static session_identifier make_new_session_identifier() {
            thread_local boost::uuids::random_generator uuid_gen{};
            return uuid_gen();
        }

        session_identifier identifier() const noexcept {
            return this->session_identifier_;
        }

    private:
        rtsp::session_identifier session_identifier_;

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
