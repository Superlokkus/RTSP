/*! @file rtsp_request.hpp
 *
 */

#ifndef RTSP_GUI_RTSP_PARSER_HPP
#define RTSP_GUI_RTSP_PARSER_HPP

#include <string>
#include <cstdint>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

namespace rtsp {
    enum struct method {
        describe,
        announce,
        get_parameter,
        options,
        pause,
        play,
        record,
        redirect,
        setup,
        set_parameter,
        teardown,
    };

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
BOOST_FUSION_ADAPT_STRUCT(
        rtsp::request
)

namespace rtsp {
    namespace ns = ::boost::spirit::standard;

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, std::string()> quoted_string{
            ::boost::spirit::qi::lexeme['"' >> +(ns::char_ - '"') >> '"']};

    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            status_code{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 3, 3>()};
    template<typename Iterator>
    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            at_least_one_digit{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 1>()};

    template<typename Iterator>
    struct rtsp_response_grammar
            : ::boost::spirit::qi::grammar<Iterator, response()> {
        rtsp_response_grammar() : rtsp_response_grammar::base_type(start) {

            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;

            start %= lit("RTSP/") >> at_least_one_digit<Iterator> >> "." >> at_least_one_digit<Iterator>
                                  >> omit[+ns::space]
                                  >> status_code<Iterator> >> omit[+ns::space]
                                  >> *(ns::char_ - (lit("\r") | lit("\n")))
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


        }

        boost::spirit::qi::rule<Iterator, request()> start;

    };

    template <typename OutputIterator>
    bool generate_response(OutputIterator sink, const response& reponse)
    {
        using ::boost::spirit::karma::lit;
        using ::boost::spirit::karma::uint_;
        using ::boost::spirit::karma::string;

        return boost::spirit::karma::generate(sink, lit("RTSP/")
                << uint_ << "." << uint_ << " " << uint_ << " " << string << "\r\n", reponse);
    }

    template<typename OutputIterator>
    bool generate_request(OutputIterator sink, const request &request) {
        using ::boost::spirit::karma::lit;
        using ::boost::spirit::karma::uint_;
        using ::boost::spirit::karma::string;

        return false;
        //return boost::spirit::karma::generate(sink, lit("\r\n"), request);
    }
}


#endif //RTSP_GUI_RTSP_PARSER_HPP
