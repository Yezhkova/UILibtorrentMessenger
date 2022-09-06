#pragma once

#include <array>
#include <cstdint>
#include <string>
#include "libtorrent/sha1_hash.hpp"
#include <boost/asio/ip/udp.hpp>

std::string toString( const std::array<uint8_t,32>& key );

std::string toString( const std::array<char,32>& key );

std::string toString( const std::array<char,64>& key );

std::string toString( const char * ptr, size_t size);

struct ReferenceNode
{
    lt::digest32<160> m_id;
    boost::asio::ip::udp::endpoint m_endpoint;

    ReferenceNode(lt::digest32<160> id, boost::asio::ip::udp::endpoint endpoint): m_id(id), m_endpoint(endpoint) {}
    bool operator < (const ReferenceNode & b)
    {
        return false;
    }
};


