#pragma once
#define BOOTSTRAP_NODE_IP "185.157.221.247"
#define BOOTSTRAP_NODE_PORT 25401
#define SALT_ENDPOINT "endpoint"

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
#include <map>
#include <future>

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
    bool m_nodeIdFound = false;

private:
    libtorrent::session                                                 m_session;
    std::shared_ptr<SessionWrapperDelegate>                             m_delegate;
    std::string                                                         m_addressAndPort;
    std::string                                                         m_username;
    lt::dht::secret_key                                                 m_secretKey;
    lt::dht::public_key                                                 m_publicKey;
    libtorrent::digest32<160>                                           m_nodeId;
    //    for void responseHandler(const lt::dht::msg & msg):
    std::string                                                         m_nodeIdRequested;
    std::map<std::string, std::shared_ptr<std::promise<boost::asio::ip::udp::endpoint>>>          m_map;
    struct DhtClientData
    {
        DhtClientData(){}
        DhtClientData(std::string requestedNodeId) : m_requiredNodeId(requestedNodeId)
        {
            std::memset(m_responseDistribution, 0, sizeof(m_responseDistribution));
        };
        enum class Type
        {
            t_msg = 'm',
            t_find_node = 'f'
        };

        Type m_type;
        std::string m_requiredNodeId;
        uint8_t m_responseDistribution[20];
        std::shared_ptr<std::promise<void>> m_promise_ptr;
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
    friend std::ostream & operator << (std::ostream & output, const DhtClientData & data)
    {
        output << "["<< static_cast<uint8_t>(data.m_type) << ", " << data.m_requiredNodeId << "]";
        return output;
    }

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

        sp.set_bool( lt::settings_pack::enable_upnp, true );
        sp.set_bool( lt::settings_pack::enable_natpmp, true );
        return sp;
    }

public:
    SessionWrapper(std::string addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate, std::string username) :
        m_session( generateSessionSettings( addressAndPort ) ),
        m_delegate( delegate ),
        m_addressAndPort(addressAndPort),
        m_username(username) { }
    lt::session * getSession()
    {
        return & m_session;
    }

    virtual void start(std::shared_ptr<std::promise<void>> p) override
    {
        m_session.set_alert_notify( [this] { this->alertHandler(); } );
        m_session.add_extension(std::make_shared<DhtRequestHandler>(m_delegate));
        Sleep(1000); // unfortunately
        libtorrent::dht::dht_state dhtState = m_session.getDhtState();
        m_nodeId = dhtState.nids.begin()->second; // can there be more - ??????????????????????????????????????
        LOG("id (" << m_username << "): "<< m_nodeId << "    vector size: "<< dhtState.nids.size());
        m_nodeIdRequested = m_nodeId.to_string();
        std::array<char, 32> seed = libtorrent::dht::ed25519_create_seed();
        std::tie(m_publicKey, m_secretKey) = lt::dht::ed25519_create_keypair(seed);
        LOG("public key ("<<m_username<<"): "<<toString(m_publicKey.bytes));
        boost::asio::ip::udp::endpoint bootstrapNodeEndpoint(
                    boost::asio::ip::make_address( BOOTSTRAP_NODE_IP ), BOOTSTRAP_NODE_PORT );
        DhtClientData * clientDataPtr = new DhtClientData(m_nodeIdRequested);
        clientDataPtr->m_type = DhtClientData::Type::t_find_node;
        clientDataPtr->m_promise_ptr = p;
        sendFindNodeRequest(bootstrapNodeEndpoint, m_nodeIdRequested, clientDataPtr);
    }

    void alertHandler()
    {
        std::vector<lt::alert *> alerts;
        m_session.pop_alerts(&alerts);
        for (auto &alert : alerts)
        {

            // For debugging!!!
            //
//                        LOG(  m_addressAndPort << ":  " << alert->what() << " (type="<< alert->type() <<"):  " << alert->message() );
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
                DhtDirectResponseAlertHandler(alert);
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
        LOG(theAlert->item)
        std::string address = theAlert->item.find_key("address")->string();
        std::string strPort = theAlert->item.find_key("port")->string();
        boost::asio::ip::udp::endpoint endpointRequested(boost::asio::ip::make_address(address),
                                                             std::stoi(strPort) );
        m_map[theAlert->item.find_key("pubKey")->string()]->set_value(endpointRequested);
    }

    void DhtDirectResponseAlertHandler(lt::alert * alert)
    {
        if ( auto* theAlert = dynamic_cast<lt::dht_direct_response_alert*>(alert); theAlert )
        {
            auto response = theAlert->response();
//            LOG(response);
            if ( response.type() != lt::bdecode_node::dict_t )
            {
//                LOG( "*** bad dht_direct_response_alert: " << theAlert->what() << ":("<< alert->type() <<")  " << theAlert->message() );
//                LOG("bad response: "<<response);
                return;
            }
            lt::bdecode_node rDict = response.dict_find_dict("r");
            if(rDict.type() != lt::bdecode_node::type_t::dict_t)
            {
                //                    LOG("response.dict_find_dict(\"r\") is not dict_t");
                return;
            }

            auto clientDataPtr = theAlert->userdata.get<DhtClientData>();

            if(clientDataPtr->m_type == DhtClientData::Type::t_find_node && !m_nodeIdFound)
            {
                processFindNodeQuery(rDict, clientDataPtr);
            }
            else if(clientDataPtr->m_type == DhtClientData::Type::t_msg)
            {
                LOG("About to processMsgQuery");
                processMsgQuery(rDict, clientDataPtr);
            }

        }
    }

    /*void setResponseHandler (std::function<void(const lt::dht::msg &)> f)
    {
        m_session.setResponseHandler(f);
    }
    */
    void processFindNodeQuery(lt::bdecode_node & rDict, DhtClientData * clientDataPtr)
    {
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
                m_nodeIdFound = true;
                std::string myAddress = it->m_endpoint.address().to_string();
                std::string myPort = std::to_string(it->m_endpoint.port());
                LOG("found " << it->m_id <<" ("<< NUMNODES << ")");
                LOG("address: " << myAddress << ", port: " << myPort);
                m_session.dht_put_item(m_publicKey.bytes, [this, myAddress, myPort, clientDataPtr](lt::entry& item, std::array<char, 64>& sig
                                       , std::int64_t& seq, std::string const& salt)
                {
                    item["node"] = m_nodeId.to_string();
                    item["address"] = myAddress;
                    item["port"] = myPort;
                    item["pubKey"] = toString(m_publicKey.bytes);
                    seq=0;
                    std::vector<char> v;
                    lt::bencode(std::back_inserter(v), item);
                    lt::dht::signature sign = lt::dht::sign_mutable_item(v, salt
                                                                         , lt::dht::sequence_number(seq), m_publicKey, m_secretKey);
                    sig = sign.bytes;
                    LOG("in processFindNodeQuery: item: "<<item);
                    clientDataPtr->m_promise_ptr->set_value();
                }, SALT_ENDPOINT);
            }
            else
            {
                sendFindNodeRequest(it->m_endpoint, m_nodeIdRequested, clientDataPtr);
            }
        }
    }

    void processMsgQuery(lt::bdecode_node & rDict, DhtClientData * clientDataPtr)
    {
        lt::bdecode_node receiverId = rDict.dict_find("id");
        lt::bdecode_node senderAddr = rDict.dict_find("addr");
        lt::bdecode_node senderPort = rDict.dict_find("port");
        lt::bdecode_node msgText = rDict.dict_find("txt");
        if(receiverId.type() != lt::bdecode_node::type_t::string_t ||
           senderAddr.type() != lt::bdecode_node::type_t::string_t ||
           senderPort.type() != lt::bdecode_node::type_t::int_t ||
           senderAddr.type() != lt::bdecode_node::type_t::string_t )
        {
            LOG("in processMsgQuery: type incongruity");
            return;
        }
        int result = rDict.dict_find("msg").int_value();

        if(result == 1)
        {
            LOG(receiverId << " received message "<<msgText<<" from "<<senderAddr<<":"<<senderPort<<" successfully" );
        }
    }


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
        LOG("Sending "<<endpoint);
        libtorrent::entry e;
//        e["y"] = "q";
        e["q"] = "msg";
        e["txt"] = text;
        std::string senderEndpoint = endpoint.address().to_string() + ":";
        senderEndpoint += std::to_string(endpoint.port());
        e["addr"] = senderEndpoint;
        DhtClientData * clientDataPtr = new DhtClientData;
        clientDataPtr->m_type = DhtClientData::Type::t_msg;
        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(clientDataPtr) );
    }

    virtual boost::asio::ip::udp::endpoint getEndpointByDhtItem(const lt::dht::public_key & key) override
    {
        //    promise& operator=(promise&& _Other) noexcept {

        std::shared_ptr< std::promise<boost::asio::ip::udp::endpoint> > prom =
                std::make_shared<std::promise<boost::asio::ip::udp::endpoint>>();
        try{
            std::future<boost::asio::ip::udp::endpoint> f = prom->get_future();  // can't read state:
            LOG("mapping "<<toString(key.bytes));
            m_map.insert(std::make_pair(toString(key.bytes), prom));
            LOG("inside getEndpointByDhtItem");
            std::thread t([key, this](){
                m_session.dht_get_item(key.bytes, SALT_ENDPOINT);
            });
            boost::asio::ip::udp::endpoint edp = f.get();
            t.join();
            return edp;
        } catch(std::exception & e){
            LOG("ERROR: " << e.what());
        }
    }

    virtual const lt::dht::public_key & getPublicKey() const override
    {
        return m_publicKey;
    }
};




