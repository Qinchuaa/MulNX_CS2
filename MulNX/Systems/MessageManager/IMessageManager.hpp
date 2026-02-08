#pragma once

#include"../../Core/ModuleBase/ModuleBase.hpp"

#include"MulNXMessage/MulNXMessage.hpp"
#include"MessageChannel/IMessageChannel.hpp"

namespace MulNX {
	class IMessageManager :public ModuleBase {
			friend IMessageChannel;
			friend class MessageChannel;
		public:
			IMessageManager() : ModuleBase() {
				//this->Type = ModuleType::MessageManager;
			}

			//注册，获取消息指针
			virtual bool Registe(ModuleBase* const Module) = 0;
			//订阅，获得对应类型消息推送
			virtual bool Subscribe(const MsgType MsgType, ModuleBase* const Module) = 0;
			//创建私有消息队列（但是生命周期仍然委托给消息管理器）
			virtual MulNXHandle CreateMessageChannel() = 0;
			//获取消息管道
			virtual IMessageChannel* GetMessageChannel(const MulNXHandle& hChannel) = 0;
			//取消订阅
			virtual bool Unsubscribe(const MsgType MsgType, ModuleBase* const Module) = 0;
			//发布，在堆空间创建消息后传递，所有权转移至总线
			virtual bool Publish(Message&& Msg) = 0;
			//释放，处理消息后必须调用，减少引用计数，直到为0后由总线释放空间
			virtual bool Release() = 0;
	};
}