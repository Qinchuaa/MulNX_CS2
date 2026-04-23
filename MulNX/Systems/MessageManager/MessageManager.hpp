#pragma once
// MessageManager.hpp
// 跨线程安全的消息管理器
// 管理整个系统消息的发送和接受
// 消息由发布者创建，经过发布后，生命周期的管理即委托给消息管理器

#include "IMessageManager.hpp"
#include "MessageChannel/MessageChannel.hpp"
#include <unordered_map>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class MsgMeta {
    public:
        std::vector<MessageChannel*>Subscribers;
        std::string RawString;
    };
    class MessageManager final :public IMessageManager {
        friend MessageChannel;
        friend IMessageManager;
    private:
        // 存储类
        std::unordered_map<MulNX::MsgType, MsgMeta>MsgMap{};
        std::unordered_map<MulNXHandle, std::unique_ptr<MessageChannel>>Channels;
        moodycamel::ConcurrentQueue<MulNX::Message>sharedBuffer;
    public:
        bool Init()override;
        // 返回true表示正在处理消息，false表示没有消息可处理
        bool NextMsg();

        // 接口实现：

        // 创建私有消息队列（但是生命周期仍然委托给消息管理器）
        MulNXHandle CreateMessageChannel()override;
        // 获取消息管道
        IMessageChannel* GetMessageChannel(const MulNXHandle& hChannel)override;
        // 需要在堆中构建消息，消息的创建由发送者负责，消息的销毁由消息总线负责
        bool Publish(Message&& Msg)override;

        bool Subscribe(MessageChannel* const pChannel, const std::string& Type);

        void HandleDispatch();
    };
}