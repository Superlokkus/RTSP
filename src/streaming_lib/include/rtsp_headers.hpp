/*! @file rtsp_headers.hpp
 * @brief Declarations for complex rtsp headers, which need normalized headers as input
 * @notice This file may not contain functionality rtsp headers, like CSeq or Session
 */

#ifndef RTSP_RTSP_HEADERS_HPP
#define RTSP_RTSP_HEADERS_HPP

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/spirit/include/qi_as.hpp>

#include <rtsp_definitions.hpp>
#include <rtsp_message.hpp>

namespace rtsp {
namespace headers {

/*! @brief RFC 2616  4.2 compliant harmonization of headers
 *
 * @param raw_headers Headers as parsed in
 * @return Harmonized headers
 */
normalized_headers normalize_headers(const raw_headers &raw_headers);

struct transport {
    struct transport_spec {
        using ttl = uint_fast16_t;
        using port_number = uint_fast32_t;
        using ssrc = uint32_t;
        using port_range = std::tuple<port_number, port_number>;
        using port = boost::variant<port_number, port_range>;
        using client_port = port;
        using server_port = port;
        using mode = string;
        using parameter = boost::variant<string, ttl, port, ssrc, mode, client_port, server_port>;
        string transport_protocol;
        string profile;
        boost::optional<string> lower_transport;
        std::vector<parameter> parameters;
    };

    transport() = default;

    explicit transport(std::vector<transport_spec> specs) : specifications(std::move(specs)) {}

    std::vector<transport_spec> specifications;
};
}
}

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::headers::transport::transport_spec,
        (rtsp::string, transport_protocol)
                (rtsp::string, profile)
                (boost::optional<rtsp::string>, lower_transport)
                (std::vector<rtsp::headers::transport::transport_spec::parameter>, parameters)
)

namespace rtsp {
namespace headers {

namespace ns = ::rtsp::ns;
template<typename Iterator>
struct common_rules : rtsp::common_rules<Iterator> {

    boost::spirit::qi::rule<Iterator, rtsp::string()> transport_protocol_{
            +rtsp::common_rules<Iterator>::token
    };

    boost::spirit::qi::rule<Iterator, rtsp::string()> profile_{
            +rtsp::common_rules<Iterator>::token
    };

    boost::spirit::qi::rule<Iterator, rtsp::string()> lower_transport_{
            +rtsp::common_rules<Iterator>::token
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::ttl()> ttl_{
            boost::spirit::qi::lit("ttl=")
                    >> boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::ttl, 10, 1, 3>()
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::port_range()> port_range_{
            boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>()
                    >> boost::spirit::qi::lit("-")
                    >> boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>()

    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::port()> port_{
            boost::spirit::qi::lit("port=") >>
                                            (port_range_ |
                                             boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>())
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::server_port()> server_port_{
            boost::spirit::qi::lit("server_port=") >>
                                                   (port_range_ |
                                                    boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>())
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::client_port()> client_port_{
            boost::spirit::qi::lit("client_port=") >>
                                                   (port_range_ |
                                                    boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>())
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::ssrc()> ssrc_{
            boost::spirit::qi::lit("ssrc=") >>
                                            boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 16, 8, 8>()
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::mode()> mode_{
            boost::spirit::qi::lit("mode=") >> (rtsp::common_rules<Iterator>::token
                                                | rtsp::common_rules<Iterator>::quoted_string)
    };


    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::parameter()> parameter_{
            boost::spirit::qi::lit(";") >> (
                    ttl_
                    | server_port_
                    | client_port_
                    | port_
                    | ssrc_
                    | mode_
                    | rtsp::common_rules<Iterator>::token
            )
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec()> transport_spec_{
            transport_protocol_ >> ::boost::spirit::qi::lit("/")
                                >> profile_
                                >> -(::boost::spirit::qi::lit("/") >> lower_transport_)
                                >> *(parameter_)
    };


};

template<typename Iterator>
struct transport_grammar
        : ::boost::spirit::qi::grammar<Iterator, rtsp::headers::transport()>,
          common_rules<Iterator> {
    transport_grammar() : transport_grammar::base_type(start) {

        using ::boost::spirit::qi::uint_parser;
        using ::boost::spirit::qi::lexeme;
        using ::boost::spirit::qi::lit;
        using boost::spirit::qi::omit;
        using ::boost::spirit::qi::repeat;

        //qi::as because of https://stackoverflow.com/a/19824426/3537677
        start %= boost::spirit::qi::as<std::vector<transport::transport_spec>>()[
                common_rules<Iterator>::transport_spec_ % ","
        ];
    }

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport()> start;

};

}
}

#endif //RTSP_RTSP_HEADERS_HPP
