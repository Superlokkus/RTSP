/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace rtsp {

    struct rtsp_session final {
        rtsp_session() {
            thread_local boost::uuids::random_generator uuid_gen{};
            this->session_identifier = uuid_gen();
        };

        boost::uuids::uuid session_identifier;

    };

}

namespace std {
template<>
struct hash<rtsp::rtsp_session> {
    typedef rtsp::rtsp_session argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const &session) const noexcept {
        return boost::uuids::hash_value(session.session_identifier);
    }
};
}


#endif //RTSP_RTSP_SESSION_HPP
