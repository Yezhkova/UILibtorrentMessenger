#pragma once

#include "libtorrent/extensions.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/bdecode.hpp"
#include <iostream>
#include "SessionWrapperDelegate.hpp"
#include "log.hpp"
struct DhtRequestHandler : libtorrent::plugin
{
private:
    std::shared_ptr<SessionWrapperDelegate> m_delegate = nullptr;
public:
    DhtRequestHandler() = default;
    DhtRequestHandler(std::shared_ptr<SessionWrapperDelegate> delegate) : m_delegate(delegate)
    {

    }
    feature_flags_t implemented_features() override
    {
        return plugin::dht_request_feature;
    }

    bool on_dht_request(
            lt::string_view                         query,
            boost::asio::ip::udp::endpoint const&   senderEndpoint,
            lt::bdecode_node const&                 dict,
            lt::entry&                              response )
        override
    {
        LOG("sent query "<<query<< " (" <<senderEndpoint<<")");
        return true;
//        LOG("on_dht_request() [q]: " << dict.dict_find_string_value("q") << ", sender: " << senderEndpoint);
//        LOG("Dictionary: " << dict);
//        if (dict.dict_find_string_value("q") == "msg")
//        {
//            auto txt = dict.dict_find_string_value("txt");
//            LOG("Received message \"" << std::string(txt) << "\" from " << senderEndpoint);
//            m_delegate->onMessage(std::string(txt), senderEndpoint);
//            response["r"]["msg"] = 1;
//            return true;
//        }
        return false;
    }
};
