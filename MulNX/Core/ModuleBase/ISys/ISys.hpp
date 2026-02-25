#pragma once

#include "../../Base/Base.hpp"

namespace MulNX {
    class C_ISys {
        friend ModuleBase;
        C_ISys() = delete;
        ModuleBase* pModuleBase = nullptr;
        C_ISys(ModuleBase* pModuleBase) {
            this->pModuleBase = pModuleBase;
        }
    public:
        void LogInfo(const std::string& Msg);
        void LogSucc(const std::string& Msg);
        void LogWarning(const std::string& Msg);
        void LogError(const std::string& Msg);

        // 自动订阅消息类型
        C_ISys& SubscribeAsync(const MsgType MsgType);
        // 自动发送消息
        void PublishAsync(MulNX::Message&& Msg);
        // 根据类型自动构建消息并发送
        void PublishAsync(MulNX::MsgType Msg);

        std::filesystem::path PathGet(const std::string& Target);
        std::filesystem::path PathGetShared(const std::string& Target);
    };
}