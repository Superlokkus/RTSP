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

#include <rtsp_definitions.hpp>

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::response,
        (uint_fast16_t, rtsp_version_major)
        (uint_fast16_t, rtsp_version_minor)
        (uint_fast16_t, status_code)
                (rtsp::string, reason_phrase)
                (rtsp::raw_headers, headers)
)
BOOST_FUSION_ADAPT_STRUCT(
        rtsp::request,
        (rtsp::method, method_or_extension)
                (rtsp::request_uri, uri)
                (uint_fast16_t, rtsp_version_major)
                (uint_fast16_t, rtsp_version_minor)
                (rtsp::raw_headers, headers)
)

BOOST_FUSION_ADAPT_STRUCT(
        rtsp::header,
        (rtsp::string, first)
                (rtsp::string, second)
)

namespace rtsp {
    namespace ns = ::boost::spirit::standard;

template<typename Iterator>
struct common_rules {

    boost::spirit::qi::rule<Iterator, rtsp::string()>
            ctl{ns::cntrl};

    boost::spirit::qi::rule<Iterator, rtsp::string()> quoted_string{
            ::boost::spirit::qi::lexeme['"' >> +(ns::char_ - (ctl | '"')) >> '"']};

    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            status_code{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 3, 3>()};

    boost::spirit::qi::rule<Iterator, uint_fast16_t()>
            at_least_one_digit{::boost::spirit::qi::uint_parser<uint_fast16_t, 10, 1>()};

    boost::spirit::qi::rule<Iterator, rtsp::string()>
            tspecials{ns::char_("()<>@,;:\\\"/[]?={} \t")};

    boost::spirit::qi::rule<Iterator, rtsp::string()>
            token{+(ns::char_ - (tspecials | ctl))};

    boost::spirit::qi::rule<Iterator, rtsp::string()>
            header_field_body_{"header_field_body"};

    boost::spirit::qi::rule<Iterator, header()>
            header_{+(token)
                            >> boost::spirit::qi::lit(":")
                            >> boost::spirit::qi::omit[*ns::space]
                            >> -(header_field_body_)
                            >> boost::spirit::qi::lit("\r\n")};

    common_rules() {
        header_field_body_ = {(quoted_string | +(ns::char_ - ctl))
                                      >> -(boost::spirit::qi::lit("\r\n")
                                              >> boost::spirit::qi::omit[+ns::space]
                                              >> header_field_body_)
        };
    }
};

    template<typename Iterator>
    struct rtsp_response_grammar
            : ::boost::spirit::qi::grammar<Iterator, response()>,
              common_rules<Iterator> {
        rtsp_response_grammar() : rtsp_response_grammar::base_type(start) {

            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;


            start %= lit("RTSP/") >> common_rules<Iterator>::at_least_one_digit >> "."
                                  >> common_rules<Iterator>::at_least_one_digit
                                  >> omit[+ns::space]
                                  >> common_rules<Iterator>::status_code >> omit[+ns::space]
                                  >> *(ns::char_ - (lit("\r") | lit("\n")))
                                  >> lit("\r\n")
                                  >> *(common_rules<Iterator>::header_)
                                  >> lit("\r\n");
        }

        boost::spirit::qi::rule<Iterator, response()> start;

    };

    template<typename Iterator>
    struct rtsp_request_grammar
            : ::boost::spirit::qi::grammar<Iterator, request()>,
              common_rules<Iterator> {
        rtsp_request_grammar() : rtsp_request_grammar::base_type(start) {

            using ::boost::spirit::qi::uint_parser;
            using ::boost::spirit::qi::lexeme;
            using ::boost::spirit::qi::lit;
            using boost::spirit::qi::omit;
            using ::boost::spirit::qi::repeat;


            start %= +(common_rules<Iterator>::token)
                    >> omit[+ns::space]
                    >> +(ns::char_ -
                         (lit("\r") | lit("\n") |
                          ns::space)) >> omit[+ns::space]
                    >> lit("RTSP/")
                    >> common_rules<Iterator>::at_least_one_digit >> "."
                    >> common_rules<Iterator>::at_least_one_digit
                    >> lit("\r\n")
                    >> *(common_rules<Iterator>::header_)
                    >> lit("\r\n");
        }

        boost::spirit::qi::rule<Iterator, request()> start;

    };

template<typename Iterator>
struct rtsp_message_grammar
        : ::boost::spirit::qi::grammar<Iterator, message()>,
          common_rules<Iterator> {
    rtsp_message_grammar() : rtsp_message_grammar::base_type(start) {


        start %= rtsp_request_grammar<Iterator>{} | rtsp_response_grammar<Iterator>{};
    }

    boost::spirit::qi::rule<Iterator, message()> start;

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
