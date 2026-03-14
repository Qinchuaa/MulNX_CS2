#include "MessageManager.hpp"

#include "../../Core/Core.hpp"

#include "MessageChannel/MessageChannel.hpp"
#include "../HandleSystem/HandleSystem.hpp"

bool MulNX::MessageManager::Init() {
    this->NeedThread(10);
    return true;
}

// 创建私有消息队列（但是生命周期仍然委托给消息管理器）
MulNXHandle MulNX::MessageManager::CreateMessageChannel() {
    std::unique_lock lock(this->GetMutex());
	std::unique_ptr<MessageChannel> Channel = std::make_unique<MessageChannel>(this);
	MulNXHandle hChannel = MulNXHandle::CreateHandle();
	Channel->hChannel = hChannel;
	this->Channels[hChannel] = std::move(Channel);
	return hChannel;
}
MulNX::IMessageChannel* MulNX::MessageManager::GetMessageChannel(const MulNXHandle& hChannel) {
    std::unique_lock lock(this->GetMutex());
	auto it = this->Channels.find(hChannel);
	if (it == this->Channels.end())return nullptr;
	return it->second.get();
}

bool MulNX::MessageManager::Publish(Message&& Msg) {
    std::unique_lock lock(this->GetMutex());
	// 检查是否存在管道订阅者
    auto& SubscriberVector = this->MsgMap[Msg.type].Subscribers;// 获取订阅者容器，这里不可能是空指针
    size_t size = SubscriberVector.size();
    if (size == 0)return false;
    --size;
    // 按需复制
	for (size_t Index=0; Index <size; ++Index) {
        // 其他订阅者使用克隆的消息
        SubscriberVector[Index]->PushMessage(Message(Msg));
    }
    // 最后一个订阅者获得原始消息
    SubscriberVector[size]->PushMessage(std::move(Msg));
    return true;
}
bool MulNX::MessageManager::Subscribe(MessageChannel* const pChannel, const std::string& Type) {
    MulNX::MsgType hashed = MulNX::HashString(Type);
    auto& Meta = this->MsgMap[hashed];
    if (Meta.RawString.empty()) {
        Meta.RawString = Type;
    }
    else if (Meta.RawString == Type) {
        
    }
    else {
        MulNX::ErrorTerminate(
            std::string("哈希碰撞!"
                "\n想要声明的: " + Type +
                "\n已有的: " + Meta.RawString));
    }

    Meta.Subscribers.push_back(pChannel);
    return true;
}

bool MulNX::MessageManager::NextMsg() {
    std::unique_lock lock(this->GetMutex());//只有切换消息时加锁，而等待组件处理消息时不加锁，不阻塞发布订阅
    return false;
}


void MulNX::MessageManager::ThreadMain() {
	if (this->NextMsg()) {
		// 有消息在处理,快速轮询
		this->SetMyThreadDelta(1);
	}
	else {
		// 没有消息在处理，降低轮询频率
		this->SetMyThreadDelta(10);
	}
}