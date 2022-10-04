#pragma once

#include <boost/asio.hpp>
#include "libtorrent/kademlia/item.hpp"

class SessionWrapperAbstract : public std::enable_shared_from_this<SessionWrapperAbstract>
{
public:
    virtual void start(std::shared_ptr<std::promise<void>> p) = 0;
    virtual void sendMessage( boost::asio::ip::udp::endpoint endPoint, const std::string& text ) = 0;
    virtual boost::asio::ip::udp::endpoint getEndpointByDhtItem(const lt::dht::public_key & key) = 0;
    virtual const lt::dht::public_key & getPublicKey() const = 0;
};

class SessionWrapperDelegate : public std::enable_shared_from_this<SessionWrapperDelegate>
{
public:
    virtual void onMessage( const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint ) = 0;
    virtual void onError( const std::error_code& ec ) = 0;
    virtual void onReply( int messageId ) = 0;
};

