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

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

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
        using port_range = std::pair<port_number, port_number>;
        struct port {
            enum struct port_type {
                general, server, client
            };
            using port_number_type = boost::variant<port_number, port_range>;
            port_type type;
            port_number_type port_numbers;
        };
        using mode = string;
        using parameter = boost::variant<string, ttl, port, ssrc, mode>;
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
        rtsp::headers::transport::transport_spec::port_range,
        (rtsp::headers::transport::transport_spec::port_number, first)
                (rtsp::headers::transport::transport_spec::port_number, second)
)

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::headers::transport::transport_spec::port,
        (rtsp::headers::transport::transport_spec::port::port_type, type)
                (rtsp::headers::transport::transport_spec::port::port_number_type, port_numbers)
)

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

    struct port_type_rule
            : boost::spirit::qi::symbols<rtsp::char_t, rtsp::headers::transport::transport_spec::port::port_type> {
        port_type_rule() {
            this->add("port", rtsp::headers::transport::transport_spec::port::port_type::general)
                    ("server_port", rtsp::headers::transport::transport_spec::port::port_type::server)
                    ("client_port", rtsp::headers::transport::transport_spec::port::port_type::client);
        }
    } port_type_;

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::port_range()> port_range_{
            boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>()
                    >> boost::spirit::qi::lit("-")
                    >> boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>()

    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::port()> port_{
            port_type_ >> boost::spirit::qi::lit("=") >>
                       (port_range_ |
                        boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::port_number, 10, 1, 5>())
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::ssrc()> ssrc_{
            boost::spirit::qi::lit("ssrc=") >>
                                            boost::spirit::qi::uint_parser<rtsp::headers::transport::transport_spec::ssrc, 16, 8, 8>()
    };

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::mode()> mode_{
            boost::spirit::qi::lit("mode=") >> (rtsp::common_rules<Iterator>::token
                                                | rtsp::common_rules<Iterator>::quoted_string)
    };


    boost::spirit::qi::rule<Iterator, rtsp::headers::transport::transport_spec::parameter()> parameter_{
            boost::spirit::qi::lit(";") >> (
                    ttl_
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
                common_rules<Iterator>::transport_spec_ % ","];
    }

    boost::spirit::qi::rule<Iterator, rtsp::headers::transport()> start;

};

template<typename OutputIterator>
struct transport_generators {

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::ttl()>
            ttl_gen{
            boost::spirit::karma::lit("ttl=") <<
                                              boost::spirit::karma::uint_generator<rtsp::headers::transport::transport_spec::ttl, 10>()
    };

    struct port_type_gen :
            boost::spirit::karma::symbols<rtsp::headers::transport::transport_spec::port::port_type, rtsp::string> {
        port_type_gen() {
            this->add(rtsp::headers::transport::transport_spec::port::port_type::general, "port")
                    (rtsp::headers::transport::transport_spec::port::port_type::server, "server_port")
                    (rtsp::headers::transport::transport_spec::port::port_type::client, "client_port");
        }
    } port_type_gen_;

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::port_range()>
            port_range_gen{
            boost::spirit::karma::uint_generator<rtsp::headers::transport::transport_spec::port_number, 10>() <<
                                                                                                              boost::spirit::karma::lit(
                                                                                                                      "-")
                                                                                                              <<
                                                                                                              boost::spirit::karma::uint_generator<rtsp::headers::transport::transport_spec::port_number, 10>()
    };

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::port()>
            port_gen{
            port_type_gen_ << boost::spirit::karma::lit("=") << (
                    port_range_gen
                    |
                    boost::spirit::karma::uint_generator<rtsp::headers::transport::transport_spec::port_number, 10>()
            )
    };

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::ssrc()>
            ssrc_gen{
            boost::spirit::karma::lit("ssrc=") <<
                                               boost::spirit::karma::uint_generator<rtsp::headers::transport::transport_spec::ssrc, 10>()
    };

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::mode()>
            mode_gen{
            boost::spirit::karma::lit("mode=\"") <<
                                                 boost::spirit::karma::string
                                                 << boost::spirit::karma::lit("\"")
    };

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec::parameter()>
            parameter_gen{
            boost::spirit::karma::lit(";") <<
                                           (ttl_gen
                                            | port_gen
                                            | ssrc_gen
                                            | mode_gen
                                            | boost::spirit::karma::string)
    };

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport::transport_spec()> transport_spec{
            boost::spirit::karma::string << boost::spirit::karma::lit("/")
                                         << boost::spirit::karma::string
                                         << -(boost::spirit::karma::lit("/") << boost::spirit::karma::string)
                                         << *(parameter_gen)
    };
};
}
}

namespace boost {
namespace spirit {
namespace traits {
template<>
struct transform_attribute<rtsp::headers::transport const, std::vector<rtsp::headers::transport::transport_spec>, karma::domain> {

    static std::vector<rtsp::headers::transport::transport_spec>
    pre(rtsp::headers::transport const &d) { return d.specifications; }
};
}
}
}

namespace rtsp {
namespace headers {
template<typename OutputIterator>
struct generate_transport_header_grammar : boost::spirit::karma::grammar<OutputIterator, rtsp::headers::transport()>,
                                           transport_generators<OutputIterator> {
    generate_transport_header_grammar() : generate_transport_header_grammar::base_type(start) {

        start = boost::spirit::karma::attr_cast<std::vector<rtsp::headers::transport::transport_spec>>(
                transport_generators<OutputIterator>::transport_spec % ",");

    }

    boost::spirit::karma::rule<OutputIterator, rtsp::headers::transport()> start;
};

}
}

#endif //RTSP_RTSP_HEADERS_HPP
