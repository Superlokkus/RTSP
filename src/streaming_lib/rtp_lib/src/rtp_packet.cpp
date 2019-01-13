/*! @file rtp_packet.cpp
 *
 */

#include <rtp_packet.hpp>

template
struct rtp::packet::custom_jpeg_packet_grammar<std::vector<uint8_t>::const_iterator>;

template
struct rtp::packet::custom_jpeg_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>>;

template
struct rtp::packet::custom_fec_packet_grammar<std::vector<uint8_t>::const_iterator>;

template
struct rtp::packet::custom_fec_packet_generator<std::back_insert_iterator<std::vector<uint8_t>>>;

