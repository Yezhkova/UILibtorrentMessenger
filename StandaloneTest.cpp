#include "SessionWrapper.hpp"
#define IP            "192.168.1.10"
#define IP_REQUESTER  "192.168.1.20"


class UIDelegate : public SessionWrapperDelegate

{
public:

    std::shared_ptr<SessionWrapper> m_sessionWrapperPtr = nullptr;
//    std::string                     m_username;
    std::string                     m_address;
    uint16_t                        m_port;

public:

    void createLtSessionPtr( const std::string& addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate, std::string username )
    {
        LOG("Creating session pointer...");
        m_sessionWrapperPtr = std::make_shared<SessionWrapper>( addressAndPort, delegate, username );
        m_sessionWrapperPtr->start();
    }

    virtual void onError( const std::error_code& ec ) override
    {
        LOG( "!ERROR! " << ec.message() )
    }

    virtual void onMessage( const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint ) override
    {
        LOG( ">> onMessage from: " << senderEndpoint << " text: " << messageText )
    }

    virtual void onReply( int messageId ) override
    {
        LOG( ">> onReply: " << messageId )
    }

};

boost::asio::ip::udp::endpoint uep(char const* ip, uint16_t port)
{
    libtorrent::error_code ec;
    boost::asio::ip::udp::endpoint ret( boost::asio::ip::make_address(ip, ec), port );
    assert(!ec);
    return ret;
}

void standaloneTest()
{
    UIDelegate responder;
    responder.createLtSessionPtr(IP ":11101", std::make_shared<UIDelegate> (responder), "user1");

    UIDelegate requester;
    requester.createLtSessionPtr(IP_REQUESTER ":11102", std::make_shared<UIDelegate> (requester), "user2");

    // successful request
    Sleep(30000);
//    requesterTestDel.m_sessionWrapperPtr->sendMessage(uep( IP, 11101 ), "I send message"); // 2 -> 1
    requester.m_sessionWrapperPtr->getDhtItem(responder.m_sessionWrapperPtr->getPublicKey());
    Sleep(5000*30);
}
