#pragma once

#include <boost/asio.hpp>

class SessionWrapperAbstract : public std::enable_shared_from_this<SessionWrapperAbstract>
{
public:
    virtual void start() = 0;
    virtual void sendMessage( boost::asio::ip::udp::endpoint endPoint, const std::string& text ) = 0;
};

class SessionWrapperDelegate : public std::enable_shared_from_this<SessionWrapperDelegate>
{
public:
    virtual void onMessage( const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint ) = 0;
    virtual void onError( const std::error_code& ec ) = 0;
    virtual void onReply( int messageId ) = 0;
};

