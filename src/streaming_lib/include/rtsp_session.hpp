/*! @file rtsp_session.hpp
 *
 * */

#ifndef RTSP_RTSP_SESSION_HPP
#define RTSP_RTSP_SESSION_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace rtsp {

    struct rtsp_session final {
        boost::uuids::uuid session_identifier;

        rtsp_session() {
            thread_local boost::uuids::random_generator uuid_gen{};
            this->session_identifier = uuid_gen();
        };

    };

}


#endif //RTSP_RTSP_SESSION_HPP
