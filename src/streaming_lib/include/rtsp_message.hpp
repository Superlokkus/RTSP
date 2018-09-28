/*! @file rtsp_message.hpp
 *
 */

#ifndef STREAMING_LIB_RTSP_MESSAGE_HPP
#define STREAMING_LIB_RTSP_MESSAGE_HPP

#include <string>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

namespace rtsp {
    using method = std::string;
    using request_uri = std::string;
    using header = std::pair<std::string, std::string>;

    const std::unordered_set<method> rtsp_methods{
            "DESCRIBE",
            "ANNOUNCE",
            "GET_PARAMETER",
            "OPTIONS",
            "PAUSE",
            "PLAY",
            "RECORD",
            "REDIRECT",
            "SETUP",
            "SET_PARAMETER",
            "TEARDOWN",
    };

    struct request {
        method method_or_extension;
        request_uri uri;
        uint_fast16_t rtsp_version_major;
        uint_fast16_t rtsp_version_minor;
        std::vector<header> headers;
    };
    struct response {
        uint_fast16_t rtsp_version_major;
        uint_fast16_t rtsp_version_minor;
        uint_fast16_t status_code;
        std::string reason_phrase;
        std::vector<header> headers;
    };
    using message = boost::variant<request, response>;
}

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::response,
        (uint_fast16_t, rtsp_version_major)
        (uint_fast16_t, rtsp_version_minor)
        (uint_fast16_t, status_code)
        (std::string, reason_phrase)
                (std::vector<rtsp::header>, headers)
)
BOOST_FUSION_ADAPT_STRUCT(
        rtsp::request,
        (rtsp::method, method_or_extension)
                (rtsp::request_uri, uri)
                (uint_fast16_t, rtsp_version_major)
                (uint_fast16_t, rtsp_version_minor)
                (std::vector<rtsp::header>, headers)
)

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::header,
        (std::string, first)
                (std::string, second)
)

namespace rtsp {
    namespace ns = ::boost::spirit::standard;

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()>
            ctl{ns::cntrl};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()> quoted_string{
            ::boost::spirit::qi::lexeme['"' >> +(ns::char_ - (ctl<Iterator> | '"')) >> '"']};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            status_code{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 3, 3>()};
    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            at_least_one_digit{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 1>()};
    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()>
            tspecials{ns::char_("()<>@,;:\\\"/[]?={} \t")};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()>
            token{+(ns::char_ - (tspecials<Iterator> | ctl<Iterator>))};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()>
            header_field_body_{"header_field_body"};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, header()>
            header_{+(token<Iterator>)
                            >> boost::spirit::qi::lit(":")
                            >> boost::spirit::qi::omit[*ns::space]
                            >> -(header_field_body_<Iterator>)
                            >> boost::spirit::qi::lit("\r\n")};

    template<typename Iterator>
    struct rtsp_response_grammar
            : ::boost::spirit::qi::grammar<Iterator, response()> {
        rtsp_response_grammar() : rtsp_response_grammar::base_type(start) {

            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;
            header_field_body_<Iterator> = {(quoted_string<Iterator> | +(ns::char_ - ctl<Iterator>))
                                                    >> -(boost::spirit::qi::lit("\r\n")
                                                            >> boost::spirit::qi::omit[+ns::space]
                                                            >> header_field_body_<Iterator>)
            };

            start %= lit("RTSP/") >> at_least_one_digit<Iterator> >> "." >> at_least_one_digit<Iterator>
                                  >> omit[+ns::space]
                                  >> status_code<Iterator> >> omit[+ns::space]
                                  >> *(ns::char_ - (lit("\r") | lit("\n")))
                                  >> lit("\r\n")
                                  >> *(header_<Iterator>)
                                  >> lit("\r\n");
        }

        boost::spirit::qi::rule<Iterator, response()> start;

    };

    template<typename Iterator>
    struct rtsp_request_grammar
            : ::boost::spirit::qi::grammar<Iterator, request()> {
        rtsp_request_grammar() : rtsp_request_grammar::base_type(start) {

            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;

            header_field_body_<Iterator> = {(quoted_string<Iterator> | +(ns::char_ - ctl<Iterator>))
                                                    >> -(boost::spirit::qi::lit("\r\n")
                                                            >> boost::spirit::qi::omit[+ns::space]
                                                            >> header_field_body_<Iterator>)
            };

            start %= +(token<Iterator>)
                    >> omit[+ns::space]
                    >> +(ns::char_ -
                         (lit("\r") | lit("\n") |
                          ns::space)) >> omit[+ns::space]
                    >> lit("RTSP/")
                    >> at_least_one_digit<Iterator> >> "."
                    >> at_least_one_digit<Iterator>
                    >> lit("\r\n")
                    >> *(header_<Iterator>)
                    >> lit("\r\n");
        }

        boost::spirit::qi::rule<Iterator, request()> start;

    };

    template<typename OutputIterator>
    boost::spirit::karma::rule<OutputIterator, rtsp::header> header_generator{
            boost::spirit::karma::string << ":" << " " << boost::spirit::karma::string
    };

    template <typename OutputIterator>
    bool generate_response(OutputIterator sink, const response& reponse)
    {
        using ::boost::spirit::karma::lit;
        using ::boost::spirit::karma::uint_;
        using ::boost::spirit::karma::string;

        return boost::spirit::karma::generate(sink, lit("RTSP/")
                << uint_ << "." << uint_ << " " << uint_ << " " << string << "\r\n"
                << *(header_generator<OutputIterator> << "\r\n")
                << "\r\n", reponse);
    }
    template<typename OutputIterator>
    bool generate_request(OutputIterator sink, const request &request) {
        using ::boost::spirit::karma::lit;
        using ::boost::spirit::karma::uint_;
        using ::boost::spirit::karma::string;

        return boost::spirit::karma::generate(sink, string << " " << string
                                                           << " " << lit("RTSP/") << uint_ << "." << uint_ << "\r\n"
                                                           << *(header_generator<OutputIterator> << "\r\n")
                                                           << "\r\n", request);
    }
}


#endif //STREAMING_LIB_RTSP_MESSAGE_HPP
