#pragma once

#include "libtorrent/session.hpp"
#include "libtorrent/extensions.hpp"
#include "DhtRequestHandler.hpp"
#include "log.hpp"

class SessionWrapper : public SessionWrapperAbstract
{
private:
    libtorrent::session                       m_session;
    std::shared_ptr<SessionWrapperDelegate>   m_delegate;

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
        return sp;
    }

public:
    SessionWrapper(std::string addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate) :
        m_session( generateSessionSettings( addressAndPort ) ),
        m_delegate( delegate )
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

    }

    virtual void sendMessage( boost::asio::ip::udp::endpoint endpoint, const std::string& text ) override
    {
        libtorrent::entry e;
        e["q"] = "msg";
        e["txt"] = text;

        m_session.dht_direct_request( endpoint, e, libtorrent::client_data_t(reinterpret_cast<int*>(12345)) );

    }
};

