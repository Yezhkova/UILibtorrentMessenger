#pragma once

#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/extensions.hpp"
#include "DhtRequestHandler.hpp"
#include "log.hpp"

#include "libtorrent/kademlia/ed25519.hpp"
#include "libtorrent/kademlia/item.hpp"
#include "libtorrent/kademlia/msg.hpp"

class SessionWrapper : public SessionWrapperAbstract
{
private:
    libtorrent::session                       m_session;
    std::shared_ptr<SessionWrapperDelegate>   m_delegate;
    std::string                               m_addressAndPort;
    std::string                               m_username;
    lt::dht::secret_key                       m_secretKey;
    lt::dht::public_key                       m_publicKey;
    libtorrent::digest32<160>                 m_nodeId;

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

    virtual void start() override
    {
        m_session.set_alert_notify( [this] { this->alertHandler(); } );
        m_session.add_extension(std::make_shared<DhtRequestHandler>(m_delegate));
        Sleep(1000); // unfortunately
        libtorrent::dht::dht_state dhtState = m_session.getDhtState();
        boost::asio::ip::address nodeAddress = dhtState.nids.begin()->first;
        m_nodeId = dhtState.nids.begin()->second; // can there be more - ??????????????????????????????????????
        LOG(m_username << ": "<< m_nodeId);
//        for(auto it = dhtState.nids.begin(); it != dhtState.nids.end(); ++it)
//        {
//            LOG("Address: " << it->first << ", ID: " << it->second);
//        }
        std::array<char, 32> seed = libtorrent::dht::ed25519_create_seed();
        std::tie(m_publicKey, m_secretKey) = lt::dht::ed25519_create_keypair(seed);
        LOG("public key (" << m_username << ") :"<< toString(m_publicKey.bytes));
        m_session.dht_put_item(m_publicKey.bytes, [this](lt::entry& item, std::array<char, 64>& sig
            , std::int64_t& seq, std::string const& salt)
        {
            item = m_nodeId.to_string();
            seq=0;
//            LOG("item after init: "<<item);
//            LOG("public key after init:" << toString(m_publicKey.bytes));
//            LOG("secret key after init:" << toString(m_secretKey.bytes));
            std::vector<char> v;
            lt::bencode(std::back_inserter(v), item);
            lt::dht::signature sign = lt::dht::sign_mutable_item(v, salt
                , lt::dht::sequence_number(seq), m_publicKey, m_secretKey);
            sig = sign.bytes;
        }, "id");
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
                    auto* theAlert = dynamic_cast<lt::dht_mutable_item_alert*>(alert);
                    auto nodeId = theAlert->item;
                    LOG("nodeId length: "<<nodeId.to_string()<<"     "<< nodeId.string().length());
                    lt::entry requestEntry;
                    requestEntry["y"] = "q";
                    requestEntry["q"] = "find_node";
                    requestEntry["a"]["target"] = nodeId.to_string();
                    boost::asio::ip::udp::endpoint bootstrapNodeEdp( boost::asio::ip::make_address("185.157.221.247"), 25401 );
//                    boost::asio::ip::udp::endpoint bootstrapNodeEdp( boost::asio::ip::make_address("192.168.1.10"), 11101 );

                    m_session.dht_direct_request( bootstrapNodeEdp, requestEntry, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );
//                    LOG("Salt:" << theAlert->salt);
//                    LOG("Item:" << theAlert->item);
//                    LOG("Key:" << toString(theAlert->key));
//                    LOG("Sequence :"<<theAlert->seq);
//                    LOG("DHT state");
//                    libtorrent::dht::dht_state dhtState = m_session.getDhtState();
//                    boost::asio::ip::address nodeAddress = dhtState.nids.begin()->first;
//                    auto nodeId = dhtState.nids.begin()->second; // can there be more - ??????????????????????????????????????
//                    LOG("Size of dhtState.nids vector: "<<dhtState.nids.size());
//                    LOG(m_username << ": "<< m_nodeId);
//                    for(auto it = dhtState.nids.begin(); it != dhtState.nids.end(); ++it)
//                    {
//                        LOG("In .nids: Address: " << it->first << ", ID: " << it->second);
//                    }

//                    if ( theAlert->salt == "endpoint" )
                    {
                    }
                    break;
                    //send dht request "find_node" from here
                }

                case lt::dht_direct_response_alert::alert_type:
                {
//                m_session.setReplyHandle(f())

//                LOG("direct_response_alert__________________________________LOG");
                     if ( auto* theAlert = dynamic_cast<lt::dht_direct_response_alert*>(alert); theAlert )
                     {
                         auto response = theAlert->response();

                         if ( response.type() == lt::bdecode_node::dict_t )
                         {
                             auto rDict = response.dict_find_dict("r");
                             auto nodesInResponse = rDict.dict_find("nodes");
                             auto idInResponse = rDict.dict_find("id");
                             LOG("rDict: "<<rDict);
                             LOG("idInResponse: "<<idInResponse);

                             LOG("type: "<<nodesInResponse.type());
                             auto sth = nodesInResponse.string_length();
                             LOG("string_length: "<<sth);
                             LOG("nodesInResponse: "<<nodesInResponse);
                             lt::string_view strVal = nodesInResponse.string_value();

//                             for(size_t offset = 0; offset < nodesInResponse.string_length(); offset += 26)
//                             {
//                                 LOG("BGYUI");
//                                 lt::digest32<160> id;
//                                 std::memcpy(id.data(), strVal.data()+offset, 20);

//                                 std::string addr = std::string()
//                                         + std::to_string(strVal[offset+20]) + "."
//                                         + std::to_string(strVal[offset+21]) + "."
//                                         + std::to_string(strVal[offset+22]) + "."
//                                         + std::to_string(strVal[offset+23]);
//                                 int port = (strVal[offset+20]<<8) | strVal[offset+20];
//                                 boost::asio::ip::udp::endpoint endpoint( boost::asio::ip::make_address(addr.c_str()), port);

//                                 LOG( "id: " << id << " port: " << port << " addr " << addr );
//                             }
                        }
                        else
                        {
                            LOG( "*** bad dht_direct_response_alert: " << theAlert->what() << ":("<< alert->type() <<")  " << theAlert->message() );
                        }
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

    void setResponseHandler (std::function<void(const lt::dht::msg &)> f)
    {
        m_session.setResponseHandler(f);
    }

    virtual void sendMessage( boost::asio::ip::udp::endpoint endpoint, const std::string& text ) override
    {
//        m_session.dht_get_item(key);

        libtorrent::entry e;
        e["y"] = "q";
        e["q"] = "msg";
        e["txt"] = text;

        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );
    }

    virtual void getDhtItem(const lt::dht::public_key & key) override
    {
        m_session.dht_get_item(key.bytes, "id");
    }

    virtual const lt::dht::public_key & getPublicKey() const override
    {
        return m_publicKey;
    }

};

