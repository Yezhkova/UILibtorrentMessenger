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
//        LOG("on_dht_request() [q]: " << dict.dict_find_string_value("q") << ", sender: " << senderEndpoint);
        if (dict.dict_find_string_value("q") == "msg")
        {
            LOG("Dictionary: " << dict);
            auto txt = dict.dict_find_string_value("txt");
            LOG("Received message \"" << std::string(txt) << "\" from " << senderEndpoint);
            response["r"]["msg"] = 1;
            response["r"]["txt"] = std::string(txt);
            response["r"]["addr"] = senderEndpoint.address().to_string();
            response["r"]["port"] = senderEndpoint.port();
            LOG("response: "<< response);
            return true;
        }
        return false;
    }
};
