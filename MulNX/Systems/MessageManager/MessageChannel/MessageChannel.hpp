#pragma once

#include "IMessageChannel.hpp"
#include <MulNX/Systems/MessageManager/MulNXMessage/MulNXMessage.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
	class MessageChannel final :public IMessageChannel {
		friend class IMessageManager;
		friend class MessageManager;
		MessageManager* MsgManager = nullptr;
		//自身句柄
		MulNXHandle hChannel{};
        moodycamel::ConcurrentQueue<MulNX::Message>Messages;
    public:
		MessageChannel(MessageManager* MsgManager);
	private:
        IMessageChannel& Subscribe(const std::string& MsgType)override;
		bool PushMessage(Message&& Msg)override;
		bool PullMessage(Message& OutMsg)override;
		bool HasMessage()const override;
		MulNXHandle GetHandle()const override;
	};
}