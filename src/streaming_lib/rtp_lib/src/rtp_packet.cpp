/*! @file rtp_packet.cpp
 *
 */

#include <rtp_packet.hpp>

template
struct rtp::packet::custom_jpeg_packet_grammar<std::vector<uint8_t>::const_iterator>;
