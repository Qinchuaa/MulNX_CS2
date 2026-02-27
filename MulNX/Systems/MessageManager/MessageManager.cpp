#include"MessageManager.hpp"

#include"../../Core/Core.hpp"

#include"MessageChannel/MessageChannel.hpp"
#include"../HandleSystem/HandleSystem.hpp"

bool MulNX::MessageManager::Registe(ModuleBase* const Module) {
    std::unique_lock lock(this->GetMutex());
	// 指针引用以重定向，得到访问当前消息服务
	// 存储注册信息
	// this->RegisteMsg[Pack.ptrID] = std::move(Pack);
	return true;
}

bool MulNX::MessageManager::Subscribe(const MsgType MsgType, ModuleBase* const Module) {
    std::unique_lock lock(this->GetMutex());
	this->SubscribeMap[MsgType].push_back(Module);
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


bool MulNX::MessageManager::Unsubscribe(const MsgType MsgType, ModuleBase* const Module) {
    std::unique_lock lock(this->GetMutex());
	auto MapIt = this->SubscribeMap.find(MsgType);
	if (MapIt == this->SubscribeMap.end()) return false;
	auto& Vector = MapIt->second;
	auto VectorIt = std::find(Vector.begin(), Vector.end(), Module);
	if (VectorIt == Vector.end())return false;
	Vector.erase(VectorIt);

	return true;
}
bool MulNX::MessageManager::Publish(Message&& Msg) {
	// 先获取锁
    std::unique_lock lock(this->GetMutex());
	// 总线订阅
	bool HasMain = false;
	// 这个索引同时用来判断是否需要转发，是否需要拷贝，还是数组索引
	// 状态机变量说明：
	// Index的多重含义：
	// [-1]: 无订阅者
	// [0]: 情况1: 只有总线订阅 (HasMain == true)
	//      情况2: 只有一个管道订阅 (HasMain == false)
	// [N>0]: 总线+管道或纯管道订阅总数-1
	int Index = -1;
	// 检查是否有在总线的订阅者
	auto SubscribeMapIt = this->SubscribeMap.find(Msg.Type);
	if (SubscribeMapIt != this->SubscribeMap.end()) {
		HasMain = true;
		++Index;
	}
	// 检查是否存在管道订阅者
	auto ChannelSubscribeMapIt = this->ChannelSubscribeMap.find(Msg.Type);
	if (ChannelSubscribeMapIt != this->ChannelSubscribeMap.end()) {
		// 如果有管道订阅者
		auto& SubscriberVector = ChannelSubscribeMapIt->second;// 获取订阅者容器
		Index += SubscriberVector.size();
	}
	if (Index < 0)return false;
	if (HasMain) {
		if (Index) {
			// 索引非0，这里需要拷贝
			this->Messages.push_back(Msg);
			--Index;
		}
		else {
			// 索引是0，那么只有总线订阅，直接返回
			this->Messages.push_back(std::move(Msg));// 添加进入异步队列，等待消息管理器线程管理
			return true;
		}
	}
	auto& SubscriberVector = ChannelSubscribeMapIt->second;// 获取订阅者容器，这里不可能是空指针
	// 按需复制，这里倒序发送，私有管道不在意一份消息被给予的顺序
	for (; Index >= 0; --Index) {
		if (Index) {
			// 其他订阅者使用克隆的消息
			SubscriberVector[Index]->PushMessage(Message(Msg));
		}
		else {
			// 最后一个订阅者获得原始消息
			SubscriberVector[Index]->PushMessage(std::move(Msg));
			return true;
		}
	}
    return true;
}

bool MulNX::MessageManager::Release() {
	// 原子变量不需要锁
	// 只会由那些被通知的组件调用，不用担心向下溢出风险
	this->RefCount.fetch_sub(1);
	return true;
}

bool MulNX::MessageManager::NextMsg() {
    std::unique_lock lock(this->GetMutex());//只有切换消息时加锁，而等待组件处理消息时不加锁，不阻塞发布订阅
	if (this->Messages.empty())return false;
	
	// 检测当前引用计数
	if (this->RefCount.load()) {
		// 有引用计数
		// 说明当前还有消息在被处理
		// 执行空闲操作，目前没有
		return true;
	}
	else {
		// 如果没有引用计数
		// 说明当前没有消息在被处理
		// 获取锁
		// 先检查当前是否存在消息，如果存在则弹出
		if (this->CurrentMessage) {
			this->Messages.pop_front();
			this->CurrentMessage = nullptr;
			if (this->Messages.empty())return false;
		}
		// 处理下一条消息
		// 分发消息给订阅者
		
		auto it = this->SubscribeMap.find(this->Messages.front().Type);
		if (it == this->SubscribeMap.end()) {
			this->Messages.pop_front();
			this->RefCount.store(0);
		}
		else {
			// 获取vector容器的大小
			size_t subscriberCount = it->second.size();
			// 设置引用计数为订阅者数量
			this->RefCount.store(subscriberCount);
			// 修改当前消息指针传递消息
			this->CurrentMessage = &this->Messages.front();
			for (auto& Module : it->second) {
				Module->CurrentMsg.store(this->CurrentMessage); //通知订阅者
			}
		}
		return true;
	}
	
	//return;
}

bool MulNX::MessageManager::Init() {
	return true;
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
MulNX::MessageManager::~MessageManager() {
    std::unique_lock lock(this->GetMutex());
	// 释放所有剩余消息
	while (!this->Messages.empty()) {
		this->Messages.pop_front();
	}
	this->SubscribeMap.clear();
	return;
}