#pragma once

#include <MulNX/MulNX.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <MulNXThirdParty/websocketpp/server.hpp>

// A minimal echo server using websocketpp (standalone Asio)
typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketManager final :public MulNX::ModuleBase {
public:
    bool Init()override { return true; }
};