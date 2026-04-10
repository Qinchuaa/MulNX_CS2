#include "WebSocketManager.hpp"

bool WebSocketManager::Init() {
    this->ISys()
        .SubscribeAsync("WebSocketManager/Post");

    // 关闭它自带的日志功能
    this->server.clear_access_channels(websocketpp::log::alevel::all);
    this->server.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化服务器
    this->server.init_asio();

    this->server.set_open_handler([this](ConnectionHandle handle) {
        this->connectionHandles.insert(handle);
        });

    this->server.set_close_handler([this](ConnectionHandle handle) {
        this->connectionHandles.erase(handle);
        });
    
    // 注册句柄
    this->server.set_message_handler(
        [this](websocketpp::connection_hdl hdl, Server::message_ptr msg) {
            try {
                this->AL3D->ExecuteCommand(msg->get_payload());
                this->server.send(hdl, "已经执行控制台指令：" + msg->get_payload(), msg->get_opcode());
            }
            catch (const websocketpp::exception& e) {
                this->ISys().LogError("网络回调接口错误: " + *e.what());
            }
        });

    this->SendTask("网络线程（阻塞！）", [this]()->bool {
        try {
            while (!this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            this->server.listen(this->port);
            this->server.start_accept();
            this->ISys().LogSucc("正在监听端口：" + std::to_string(port));
            // 阻塞调用
            this->server.run();
        }
        catch (const std::exception& e) {
            MulNX::ErrorTerminate("网络功能启动失败！\n" + *e.what());
        }
        return true;
        });
    return true;
}

void WebSocketManager::VirtualMain() {
    this->EntryProcessMsg();
}

void WebSocketManager::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "WebSocketManager/Post"_hash: {
        MulNX::any_shared_ptr pPayLoad = Msg.asp;
        auto& ios = this->server.get_io_service();
        ios.post([this, pPayLoad]()mutable {

            // 提取要广播的消息内容
            std::string payLoad = std::move(*pPayLoad.get<std::string>());

            // 遍历所有连接并发送
            for (auto hdl : connectionHandles) {
                try {
                    this->server.send(hdl, payLoad, websocketpp::frame::opcode::text);
                }
                catch (const websocketpp::exception& e) {
                    this->ISys().LogError("网络消息发送失败！内容：" + payLoad);
                }
            }

            });

        break;
    }
    }
}