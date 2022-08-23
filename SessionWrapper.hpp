#pragma once
#define LIBTORRENT_MESSENGER_PET_PROJECT
#include "libtorrent/session.hpp"
#include "libtorrent/extensions.hpp"
#include "DhtRequestHandler.hpp"
#include "log.hpp"

#include "libtorrent/aux_/session_impl.hpp"
#include "libtorrent/kademlia/dht_tracker.hpp"

class SessionWrapper : public SessionWrapperAbstract
{
private:
    libtorrent::session                       m_session;
    std::shared_ptr<SessionWrapperDelegate>   m_delegate;
    std::string                               m_addressAndPort;

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
//        std::string c = "abcdefghijklmnopqrst";
//        sp.set_str(libtorrent::settings_pack::peer_fingerprint, c);
        return sp;
    }

public:
    SessionWrapper(std::string addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate) :
        m_session( generateSessionSettings( addressAndPort ) ),
        m_delegate( delegate ),
        m_addressAndPort(addressAndPort)
    {
        LOG("SessionWrapper initialized");
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
//            if(alert->type() == 88)
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
//                    auto* theAlert = dynamic_cast<lt::dht_mutable_item_alert*>(alert);
//                    if ( theAlert->salt == "endpoint" )
//                    {
//                    }
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
        libtorrent::entry e;
        e["y"] = "q";
        e["q"] = "msg";
        e["txt"] = text;
        std::shared_ptr<libtorrent::aux::session_impl> sessionImpl = m_session.native_handle();
        // in session_impl add function to reach dht -> to comments
        libtorrent::dht::dht_state dhtState = sessionImpl->getDhtState();
//        libtorrent::dht::dht_tracker * dht = session_impl->dht();
//        libtorrent::dht::dht_state dhtState = dht->state();
//        libtorrent::dht::node_ids_t nidsVec = dhtState.nids;
//        for(auto it = nidsVec.begin(); it != nidsVec.end(); ++it)
//        {
//            LOG("ID: " << it->second.to_string());
//        }

//        LOG("ID: " << m_session.get_settings().get_str(lt::settings_pack::peer_fingerprint));
        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );

    }
};

