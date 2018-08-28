/*! @file rtsp_parser.hpp
 *
 */

#ifndef RTSP_GUI_RTSP_PARSER_HPP
#define RTSP_GUI_RTSP_PARSER_HPP

#include <string>

#include <boost/spirit/include/qi.hpp>

namespace rtsp {
    struct request {
    };
    struct response {
        std::string rtsp_version_major;
        std::string rtsp_version_minor;
        std::string status_code;
        std::string reason_phrase;
    };
    using message = boost::variant<request, response>;
}
BOOST_FUSION_ADAPT_STRUCT(
        rtsp::response,
        (std::string, rtsp_version_major)
        (std::string, rtsp_version_minor)
        (std::string, status_code)
        (std::string, reason_phrase)
)
namespace rtsp {

    template<typename Iterator>
    struct rtsp_response_grammar
            : ::boost::spirit::qi::grammar<Iterator, response()> {
        rtsp_response_grammar() : rtsp_response_grammar::base_type(start) {
            namespace ns = ::boost::spirit::standard;
            using ::boost::spirit::qi::uint_;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;
            quoted_string %= lexeme['"' >> +(ns::char_ - '"') >> '"'];
            start %= lit("RTSP/") >> +ns::digit >> "." >> +ns::digit >> omit[+ns::space]
                    >> repeat(3)[ns::digit] >> omit[+ns::space]
                    >> *(ns::char_ - (lit("\r") | lit("\n")));
        }

        boost::spirit::qi::rule<Iterator, std::string()> quoted_string;
        boost::spirit::qi::rule<Iterator, response()> start;
    };
}


#endif //RTSP_GUI_RTSP_PARSER_HPP
