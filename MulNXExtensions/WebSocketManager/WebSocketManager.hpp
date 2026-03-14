#pragma once

#include "IWebSocketManager.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using Server = websocketpp::server<websocketpp::config::asio>;

class WebSocketManager final :public IWebSocketManager {
    Server server;
    uint16_t port = 55202;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message* Msg)override;
    void VirtualMain()override;
    void ThreadMain()override;
};