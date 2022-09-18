#include "SessionWrapper.hpp"
#define IP            "192.168.1.10"
#define IP_REQUESTER  "192.168.1.20"


// for void responseHandler(const lt::dht::msg & msg)

//lt::session * g_sessionToSendFindNode = nullptr;
lt::dht::public_key g_publicKeyRequested;
//std::string g_nodeIdRequested = "";

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

//    void dhtDirectRequest(boost::asio::ip::udp::endpoint endpoint,lt::entry entry, libtorrent::client_data_t data)
//    {
//        m_sessionWrapperPtr->dhtDirectRequest(endpoint, entry, data);
//    }

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

/*void responseHandler(const lt::dht::msg & msg)
{
    // if k = id -> find node

    LOG("response: message: "<< msg.message << ", address: " << msg.addr);
    LOG("message type: "<< msg.message.type() );


    try
    {
        lt::bdecode_node responseInfoMap = msg.message.dict_find_dict("r");
        if(responseInfoMap.type() != lt::bdecode_node::type_t::dict_t)
        {
            return;
        }
        auto publicKeyAcquired = responseInfoMap.dict_find("k");
        if(publicKeyAcquired.type() != lt::bdecode_node::type_t::string_t)
        {
            return;
        }
        auto keyAcq = toString( publicKeyAcquired.string_value().data(), publicKeyAcquired.string_value().size() );
        LOG("publicKeyAcquired : " << keyAcq);
        auto keyReq = toString(g_publicKeyRequested.bytes);
        LOG("keyReq: " << keyReq);
            if (keyAcq == keyReq)
            {
                lt::entry requestEntry;
                requestEntry["y"] = "q";
                requestEntry["q"] = "find_node";
                requestEntry["a"]["target"] = msg.message.dict_find("r").dict_find("v");
                g_sessionToSendFindNode->dht_direct_request( msg.addr, requestEntry, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );
                //80dcb7986db4406e1247aed45cb3e70894e131f3
            }
    }
    catch (...)
    {
    }
}
*/

void standaloneTest()
{
    UIDelegate responder;
    responder.createLtSessionPtr(IP ":11101", std::make_shared<UIDelegate> (responder), "user1");
    Sleep(1000);
    UIDelegate requester;
    requester.createLtSessionPtr(IP_REQUESTER ":11102", std::make_shared<UIDelegate> (requester), "user2");
    Sleep(1000);
    g_publicKeyRequested = responder.m_sessionWrapperPtr->getPublicKey();
    requester.m_sessionWrapperPtr->getDhtItem(g_publicKeyRequested);
    Sleep(500000);

    // successful request
//    Sleep(30000);
//    requesterTestDel.m_sessionWrapperPtr->sendMessage(uep( IP, 11101 ), "I send message"); // 2 -> 1
//    g_sessionToSendFindNode = requester.m_sessionWrapperPtr->getSession();
//    g_publicKeyRequested = requester.m_sessionWrapperPtr->getPublicKey();
//    requester.m_sessionWrapperPtr->getDhtItem(g_publicKeyRequested);
//    g_nodeIdRequested = requester.m_sessionWrapperPtr->getNodeIdRequested();

//    for void responseHandler(const lt::dht::msg & msg):
//    requester.m_sessionWrapperPtr->setResponseHandler(responseHandler);

//    Sleep();
}
