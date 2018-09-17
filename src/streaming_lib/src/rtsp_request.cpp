/*! @file rtsp_request.cpp
 *
 */

#include <rtsp_request.hpp>

#include <iterator>

template
struct rtsp::rtsp_response_grammar<std::string::const_iterator>;

template
bool rtsp::generate_response<std::back_insert_iterator<std::string>>(std::back_insert_iterator<std::string> sink, const response& reponse);
