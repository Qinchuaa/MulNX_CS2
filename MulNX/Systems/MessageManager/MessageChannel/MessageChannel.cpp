#include "MessageChannel.hpp"
#include "../MessageManager.hpp"

MulNX::MessageChannel::MessageChannel(MessageManager* MsgManager) {
	this->MsgManager = MsgManager;
}
MulNX::IMessageChannel& MulNX::MessageChannel::Subscribe(const std::string& MsgType) {
	std::unique_lock lock(this->MsgManager->GetMutex());
    this->MsgManager->Subscribe(this, MsgType);
	return *this;
}
bool MulNX::MessageChannel::PullMessage(Message& OutMsg) {
	std::unique_lock lock(this->ChannelMutex);
	if (!this->Messages.empty()) {
		OutMsg = std::move(this->Messages.front());
		this->Messages.pop_front();
		return true;
	}
	this->bHasMessage = false;
	return false;
}
bool MulNX::MessageChannel::PushMessage(Message&& Msg) {
	std::unique_lock lock(this->ChannelMutex);
	this->Messages.push_back(std::move(Msg));
	this->bHasMessage.store(true);
	return true;
}
bool MulNX::MessageChannel::HasMessage()const {
	return this->bHasMessage.load();
}

MulNXHandle MulNX::MessageChannel::GetHandle()const {
	return this->hChannel;
}