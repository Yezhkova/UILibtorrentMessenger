#pragma once

#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/extensions.hpp"
#include "DhtRequestHandler.hpp"
#include "log.hpp"

#include "libtorrent/kademlia/ed25519.hpp"
#include "libtorrent/kademlia/item.hpp"

class SessionWrapper : public SessionWrapperAbstract
{
private:
    libtorrent::session                       m_session;
    std::shared_ptr<SessionWrapperDelegate>   m_delegate;
    std::string                               m_addressAndPort;
    std::string                               m_username;

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
        Sleep(1000); // unfortunately

        libtorrent::dht::dht_state dhtState = m_session.getDhtState();
        boost::asio::ip::address nodeAddress = dhtState.nids.begin()->first;
        libtorrent::digest32<160> nodeId = dhtState.nids.begin()->second; // can there be more - ??????????????????????????????????????
        LOG(m_username << ": "<< nodeId);
//        for(auto it = dhtState.nids.begin(); it != dhtState.nids.end(); ++it)
//        {
//            LOG("Address: " << it->first << ", ID: " << it->second);
//        }

//        bootstrap_session({&dht, &dht6}, m_session); // ?????????????????????????????????????????????????????????????????????????????

//        std::array<char, 32> seed = libtorrent::dht::ed25519_create_seed(); // cryptographically random bytes - ?????????????????????
        std::array<char, 32> seed;
        lt::dht::secret_key sk;
        lt::dht::public_key pk;
//        std::tie(pk, sk) = lt::dht::ed25519_create_keypair(seed);

        // use m_username as public key
        for(int i = 0; i < m_username.length(); ++i)
        {
            pk.bytes[i] = m_username[i];
        }
        // rest is '0'
        std::fill(pk.bytes.begin()+m_username.length(), pk.bytes.end(), '0');
//        LOG("public key (" << m_username << ") :")
//        for(int i = 0; i < pk.len; ++i)
//        {
//            LOG(i << '\t' << pk.bytes[i]);
//        }
        std::tie(pk, sk) = lt::dht::ed25519_create_keypair(seed);
        m_session.dht_put_item(pk.bytes, [&](lt::entry& item, std::array<char, 64>& sig
            , std::int64_t& seq, std::string const& salt)
        {
            item = nodeId;
            seq = 1;
            std::vector<char> v;
            lt::bencode(std::back_inserter(v), item);
            lt::dht::signature sign = lt::dht::sign_mutable_item(v, salt
                , lt::dht::sequence_number(seq), pk, sk);
//            put_count++; // earlier: int put_count = 0;
            sig = sign.bytes;
        }, nodeAddress.to_string());
        Sleep(1000);

    }

    virtual void start() override
    {
        m_session.set_alert_notify( [this] { this->alertHandler(); } );
        m_session.add_extension(std::make_shared<DhtRequestHandler>(m_delegate));
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
                    LOG("Salt: " << theAlert->salt);
                    LOG("Id: " << theAlert->item);
//                    if ( theAlert->salt == "endpoint" )
                    {
                    }
                    break;
                }

                case lt::dht_direct_response_alert::alert_type:
                {
                LOG("_________________LOG");
                     if ( auto* theAlert = dynamic_cast<lt::dht_direct_response_alert*>(alert); theAlert )
                     {
                         auto response = theAlert->response();
                         LOG("response: " << response);

                         if ( response.type() == lt::bdecode_node::dict_t )
                         {
                             auto rDict = response.dict_find_dict("r");

                             if ( rDict.type() == lt::bdecode_node::dict_t )
                             {
                                 auto query = rDict.dict_find_string_value("q");

                                 if ( query == "msg" )
                                 {
                                     auto id = (uint64_t) theAlert->userdata.get<void>();
                                     m_delegate->onReply( (uint32_t)id );
                                 }
                             }
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

    virtual void sendMessage( boost::asio::ip::udp::endpoint endpoint, const std::string& text ) override
    {
        std::array<char, 32> key;
        std::string userToFind = "user1";
        for(int i = 0; i < m_username.length(); ++i)
        {
            key[i] = m_username[i];
        }
        std::fill(key.begin()+m_username.length(), key.end(), '0');
        m_session.dht_get_item(key);

//        libtorrent::entry e;
//        e["y"] = "q";
//        e["q"] = "msg";
//        e["txt"] = text;

//        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );
    }
};

