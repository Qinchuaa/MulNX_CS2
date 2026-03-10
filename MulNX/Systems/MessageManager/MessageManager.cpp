#include "MessageManager.hpp"

#include "../../Core/Core.hpp"

#include "MessageChannel/MessageChannel.hpp"
#include "../HandleSystem/HandleSystem.hpp"

// FNV-1a 64位常数
constexpr std::size_t fnv_basis = 14695981039346656037ULL;
constexpr std::size_t fnv_prime = 1099511628211ULL;

// FNV-1a 核心算法（constexpr）
constexpr std::size_t fnv1a_hash(const std::string& sv) noexcept {
    std::size_t hash = fnv_basis;
    for (unsigned char c : sv) {
        hash = (hash ^ static_cast<std::size_t>(c)) * fnv_prime;
    }
    return hash;
}

MulNX::MessageManager& MulNX::MessageManager::DeclareType(const std::string& Type) {
    size_t hashed = fnv1a_hash(Type);
    auto it = map.find(hashed);
    if (it == map.end()) {
        map[hashed] = 1;
        return *this;
    }
    else {
        MulNX::ErrorTerminate("哈希碰撞！   " + Type);
    }
}

// 定义字面量操作符
consteval size_t operator"" _hash(const char* str, size_t n) {
    std::string Str(str, n);
    return fnv1a_hash(Str);
}

bool MulNX::MessageManager::Init() {
    this->NeedThread(10);
    (*this)
        .DeclareType("你好")
        .DeclareType("你好2")
        .DeclareType("你好2")
        .DeclareType("你好3");
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
	auto ChannelSubscribeMapIt = this->ChannelSubscribeMap.find(Msg.Type);
    if (ChannelSubscribeMapIt == this->ChannelSubscribeMap.end())return false;
    auto& SubscriberVector = ChannelSubscribeMapIt->second;// 获取订阅者容器，这里不可能是空指针
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