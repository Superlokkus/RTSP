/*! @file rtsp_parser.hpp
 *
 */

#ifndef RTSP_GUI_RTSP_PARSER_HPP
#define RTSP_GUI_RTSP_PARSER_HPP

#include <string>
#include <cstdint>

#include <boost/spirit/include/qi.hpp>


namespace rtsp {
    struct request {
    };
    struct response {
        uint_fast16_t rtsp_version_major;
        uint_fast16_t rtsp_version_minor;
        uint_fast16_t status_code;
        std::string reason_phrase;
    };
    using message = boost::variant<request, response>;
}
BOOST_FUSION_ADAPT_STRUCT(
        rtsp::response,
        (uint_fast16_t, rtsp_version_major)
        (uint_fast16_t, rtsp_version_minor)
        (uint_fast16_t, status_code)
        (std::string, reason_phrase)
)
namespace rtsp {

    template<typename Iterator>
    struct rtsp_response_grammar
            : ::boost::spirit::qi::grammar<Iterator, response()> {
        rtsp_response_grammar() : rtsp_response_grammar::base_type(start) {
            namespace ns = ::boost::spirit::standard;
            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;
            quoted_string %= lexeme['"' >> +(ns::char_ - '"') >> '"'];
            status_code = uint_parser<uint_fast16_t, 10, 3, 3>();
            at_least_one_digit = uint_parser<uint_fast16_t, 10, 1>();

            start %= lit("RTSP/") >> at_least_one_digit >> "." >> at_least_one_digit >> omit[+ns::space]
                    >> status_code >> omit[+ns::space]
                    >> *(ns::char_ - (lit("\r") | lit("\n")))
                    >> lit("\r\n");
        }

        boost::spirit::qi::rule<Iterator, std::string()> quoted_string;
        boost::spirit::qi::rule<Iterator, uint_fast16_t()> status_code;
        boost::spirit::qi::rule<Iterator, uint_fast16_t()> at_least_one_digit;
        boost::spirit::qi::rule<Iterator, response()> start;

    };
}


#endif //RTSP_GUI_RTSP_PARSER_HPP
