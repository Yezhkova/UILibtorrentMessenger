#pragma once
#define BOOTSTRAP_NODE_IP "185.157.221.247"
#define BOOTSTRAP_NODE_PORT 25401

//    for void responseHandler(const lt::dht::msg & msg):
//#include "libtorrent/kademlia/msg.hpp"

#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/extensions.hpp"
#include "DhtRequestHandler.hpp"
#include "log.hpp"
#include "libtorrent/kademlia/ed25519.hpp"
#include "libtorrent/kademlia/item.hpp"
#include "utils.h"
#include <set>

struct Cmp {
    bool operator () (const ReferenceNode & a, const ReferenceNode & b) const
    {
        assert(a.m_ref == b.m_ref);
        lt::dht::node_id const lhs = a.m_id ^ a.m_ref;
        lt::dht::node_id const rhs = b.m_id ^ b.m_ref;
        return lhs < rhs;
    }
};

class SessionWrapper : public SessionWrapperAbstract
{
public:
    uint32_t NUMNODES = 0;
    bool ACTIVATE_DIR_RESP_ALERT = true;
private:
    libtorrent::session                       m_session;
    std::shared_ptr<SessionWrapperDelegate>   m_delegate;
    std::string                               m_addressAndPort;
    std::string                               m_username;
    lt::dht::secret_key                       m_secretKey;
    lt::dht::public_key                       m_publicKey;
    libtorrent::digest32<160>                 m_nodeId;
    //    for void responseHandler(const lt::dht::msg & msg):
    std::string                               m_nodeIdRequested;
    struct DhtClientData
    {
        DhtClientData(std::string requestedNodeId) : m_requiredNodeId(requestedNodeId)
        {
            std::memset(m_responseDistribution, 0, sizeof(m_responseDistribution));
        };
        enum Type
        {
            //            t_msg,
            t_find_node
        };

        Type m_type;
        std::string m_requiredNodeId;
        uint8_t m_responseDistribution[20];
        //        int m_depth;
        int comparePrefixes(const std::string & id)
        {
            int counter = 0;
            for(int i = 0; id[i] == m_requiredNodeId[i]; ++i)
            {
                ++counter;
            }
            return counter;
        };
    };

    std::set<DhtClientData * > m_dhtClientDataSet;

    lt::settings_pack generateSessionSettings(std::string addressAndPort)
    {
        libtorrent::settings_pack sp;
        sp.set_bool(libtorrent::settings_pack::enable_lsd, false);
        sp.set_bool(libtorrent::settings_pack::enable_natpmp, false);
        sp.set_bool(libtorrent::settings_pack::enable_upnp, false);
        sp.set_bool(libtorrent::settings_pack::enable_dht, true);
        //    m_sp.set_str(libtorrent::settings_pack::dht_bootstrap_nodes, "");
        sp.set_int(libtorrent::settings_pack::max_retry_port_bind, 800);
        sp.set_int(libtorrent::settings_pack::alert_mask, ~0);
        sp.set_str(libtorrent::settings_pack::listen_interfaces, addressAndPort);
        sp.set_bool(libtorrent::settings_pack::dht_ignore_dark_internet, false);
        sp.set_bool(libtorrent::settings_pack::dht_restrict_routing_ips, false);
        return sp;
    }

public:
    SessionWrapper(std::string addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate, std::string username) :
        m_session( generateSessionSettings( addressAndPort ) ),
        m_delegate( delegate ),
        m_addressAndPort(addressAndPort),
        m_username(username)
    {
        LOG("SessionWrapper ("<<m_username<<") initialized");
    }
    lt::session * getSession()
    {
        return & m_session;
    }

    virtual void start() override
    {
        m_session.set_alert_notify( [this] { this->alertHandler(); } );
        m_session.add_extension(std::make_shared<DhtRequestHandler>(m_delegate));
        Sleep(1000); // unfortunately
        libtorrent::dht::dht_state dhtState = m_session.getDhtState();
        m_nodeId = dhtState.nids.begin()->second; // can there be more - ??????????????????????????????????????
        LOG("id (" << m_username << "): "<< m_nodeId << "    vector size: "<< dhtState.nids.size());
        m_nodeIdRequested = m_nodeId.to_string();
        boost::asio::ip::udp::endpoint bootstrapNodeEndpoint(
                    boost::asio::ip::make_address( BOOTSTRAP_NODE_IP ), BOOTSTRAP_NODE_PORT );
        DhtClientData * clientDataPtr = new DhtClientData(m_nodeIdRequested);
        clientDataPtr->m_type = DhtClientData::Type::t_find_node;
        sendFindNodeRequest(bootstrapNodeEndpoint, m_nodeIdRequested, clientDataPtr);
        Sleep(8000);
    }

    void alertHandler()
    {
        std::vector<lt::alert *> alerts;
        m_session.pop_alerts(&alerts);
        for (auto &alert : alerts)
        {

            // For debugging!!!
            //
            //            LOG(  m_addressAndPort << ":  " << alert->what() << " (type="<< alert->type() <<"):  " << alert->message() );
            //            continue;

            switch (alert->type())
            {

            case lt::dht_announce_alert::alert_type:
            {
                break;
            }

            case lt::dht_immutable_item_alert::alert_type:
            {
                break;
            }

            case lt::dht_mutable_item_alert::alert_type:
            {
                DhtMutableItemAlertHandler(alert);
                break;
            }

            case lt::dht_direct_response_alert::alert_type:
            {
                if(ACTIVATE_DIR_RESP_ALERT)
                {
                    DhtDirectResponseAlertHandler(alert);
                }
                break;
            }


            case lt::listen_failed_alert::alert_type:
            {
                if ( auto *theAlert = dynamic_cast<lt::listen_failed_alert *>(alert); theAlert )
                {
                    LOG(  "listen error: " << theAlert->message())
                            m_delegate->onError( theAlert->error );
                }
                break;
            }

            case lt::portmap_error_alert::alert_type:
            {
                if ( auto *theAlert = dynamic_cast<lt::portmap_error_alert *>(alert); theAlert )
                {
                    LOG(  "portmap error: " << theAlert->message())
                }
                break;
            }

            case lt::dht_error_alert::alert_type:
            {
                if ( auto *theAlert = dynamic_cast<lt::dht_error_alert *>(alert); theAlert )
                {
                    LOG(  "dht error: " << theAlert->message())
                }
                break;
            }

            case lt::session_error_alert::alert_type:
            {
                if ( auto *theAlert = dynamic_cast<lt::session_error_alert *>(alert); theAlert )
                {
                    LOG(  "session error: " << theAlert->message())
                }
                break;
            }

            case lt::udp_error_alert::alert_type:
            {
                if ( auto *theAlert = dynamic_cast<lt::udp_error_alert *>(alert); theAlert )
                {
                    LOG(  "udp error: " << theAlert->message())
                }
                break;
            }

                //                case lt::log_alert::alert_type:
                //                {
                //                    LOG(  ": session_log_alert: " << alert->message())
                //                    break;
                //                }
                //
                //                case lt::dht_bootstrap_alert::alert_type: {
                //                    LOG( "dht_bootstrap_alert: " << alert->message() )
                //                    break;
                //                }
                //
                //                case lt::external_ip_alert::alert_type: {
                //                    auto* theAlert = dynamic_cast<lt::external_ip_alert*>(alert);
                //                    LOG( "External Ip Alert " << " " << theAlert->message())
                //                    break;
                //                }

            default:
                break;
            }
        }

    }

    void DhtMutableItemAlertHandler(lt::alert * alert)
    {
        auto* theAlert = dynamic_cast<lt::dht_mutable_item_alert*>(alert);
        LOG(theAlert->item);
//        if(theAlert->type() != lt::bdecode_node::dict_t)
//        {
//            LOG("theAlert is not dict_t");
//            return;
//        }
//        std::string address = theAlert->item.find_key("address")->string();
//        std::string strPort = theAlert->item.find_key("port")->string();
//        LOG("in DhtMutableItemAlertHandler: address:" << address << ", port: "<< strPort);
//        int port = std::stoi(strPort);
//        boost::asio::ip::udp::endpoint receiverEndpoint(
//                    boost::asio::ip::make_address(address), port );
//        std::string message = "i write message";
//        sendMessage(receiverEndpoint, message);
    }

    void DhtDirectResponseAlertHandler(lt::alert * alert)
    {
        if ( auto* theAlert = dynamic_cast<lt::dht_direct_response_alert*>(alert); theAlert )
        {
            auto response = theAlert->response();
            if ( response.type() == lt::bdecode_node::dict_t )
            {
                lt::bdecode_node rDict = response.dict_find_dict("r");
                if(rDict.type() != lt::bdecode_node::type_t::dict_t)
                {
//                    LOG("response.dict_find_dict(\"r\") is not dict_t");
                    return;
                }
                lt::bdecode_node nodesInResponse = rDict.dict_find("nodes");
                if(nodesInResponse.type() != lt::bdecode_node::type_t::string_t)
                {
//                    LOG("rDict.dict_find(\"nodes\") is not string_t");
                    return;
                }
                std::set<ReferenceNode, Cmp> refNodes = parseNodes(nodesInResponse);
                if(refNodes.size() == 0)
                {
//                    LOG("No nodes to ask");
                    return;
                }
                bool b = false;
                for(auto it = refNodes.begin(); it != refNodes.end() && b == false; ++it)
                {
                    b = idEqual(it->m_id, m_nodeIdRequested);
                    if(b)
                    {
                        ACTIVATE_DIR_RESP_ALERT = false;
                        LOG("found " << it->m_id <<" ("<< NUMNODES << ")");
                        std::string myAddress = it->m_endpoint.address().to_string();
                        std::string myPort = std::to_string(it->m_endpoint.port());
                        m_session.dht_put_item(m_publicKey.bytes, [this, myAddress, myPort](lt::entry& item, std::array<char, 64>& sig
                                               , std::int64_t& seq, std::string const& salt)
                        {

                            item = m_nodeId.to_string();
//                            item["node"] = m_nodeId.to_string();
//                            item["address"] = myAddress; //"78.26.151.81";
//                            item["port"] = myPort; //"11101";
                            seq=0;
                            std::vector<char> v;
                            lt::bencode(std::back_inserter(v), item);
                            lt::dht::signature sign = lt::dht::sign_mutable_item(v, salt
                                                                                 , lt::dht::sequence_number(seq), m_publicKey, m_secretKey);
                            sig = sign.bytes;
                        }, "endpoint");
                        LOG(m_username<<" dht_put_item done");
                    }
                    else
                    {
                        auto clientDataPtr = new DhtClientData(m_nodeIdRequested);
                        sendFindNodeRequest(it->m_endpoint, m_nodeIdRequested, clientDataPtr);
                    }
                }
            }
            else
            {
                LOG( "*** bad dht_direct_response_alert: " << theAlert->what() << ":("<< alert->type() <<")  " << theAlert->message() );
                LOG("bad response: "<<response);
            }
        }
    }

    /*void setResponseHandler (std::function<void(const lt::dht::msg &)> f)
    {
        m_session.setResponseHandler(f);
    }
    */

    bool idEqual(const lt::digest32<160> & id, const std::string & ref)
    {
        for(int i = 0; i < 20; ++i)
        {
            if (id[i] != uint8_t(ref[i]))
            {
                return false;
            }
        }
        return true;
    }

    std::set<ReferenceNode, Cmp> parseNodes(lt::bdecode_node nodesStr)
    {
        std::set<ReferenceNode, Cmp> ans;
        lt::string_view strVal = nodesStr.string_value();
        for(size_t offset = 0; offset < nodesStr.string_length(); offset += 26)
        {
            lt::digest32<160> id;
            std::memcpy(id.data(), strVal.data()+offset, 20);
            uint32_t ip = uint8_t( strVal[offset+20] ) * 256 * 256 * 256 +
                    uint8_t( strVal[offset+21] ) * 256 * 256 +
                    uint8_t( strVal[offset+22] ) * 256 +
                    uint8_t( strVal[offset+23] );
            boost::asio::ip::address_v4 addr(ip);
            uint16_t port = uint8_t( strVal[offset+24] ) * 256 + uint8_t( strVal[offset+25] );  // a * 256 + b
            boost::asio::ip::udp::endpoint endpoint( addr, port);
            ans.emplace(ReferenceNode{id, endpoint,  lt::digest32<160>(m_nodeIdRequested)});
        }
        return ans;
    }

    void sendFindNodeRequest(boost::asio::ip::udp::endpoint endpoint, const std::string & node, DhtClientData * clientDataPtr)
    {
        ++NUMNODES;
        lt::entry requestEntry;
        requestEntry["y"] = "q";
        requestEntry["q"] = "find_node";
        requestEntry["a"]["target"] = node;
        m_session.dht_direct_request( endpoint, requestEntry, libtorrent::client_data_t(clientDataPtr) );
    }

    virtual void sendMessage( boost::asio::ip::udp::endpoint endpoint, const std::string& text ) override
    {
        libtorrent::entry e;
        e["y"] = "q";
        e["q"] = "msg";
        e["txt"] = text;
        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(reinterpret_cast<int*>(12346)) );
    }

    virtual void getDhtItem(const lt::dht::public_key & key) override
    {
        m_session.dht_get_item(key.bytes, "endpoint");
    }

    virtual const lt::dht::public_key & getPublicKey() const override
    {
        return m_publicKey;
    }

};




