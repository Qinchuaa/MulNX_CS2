#include "MulNXUINode.hpp"

#include "../MulNXUIContext.hpp"

#include "../../../MessageManager/IMessageManager.hpp"

#include "../../../../Core/Core.hpp"

void MulNXUINode::Draw() {
	if (this->MyMsgChannel->HasMessage()) {
		MulNX::Message Msg(MulNX::MsgType::Null);
		while(this->MyMsgChannel->PullMessage(Msg)){
			if(Msg.Type == MulNX::MsgType::UISystem_ModuleResponse){
				this->WaitingResponse = false;
			}
		}
	}
	this->MyFunc(this);
}
bool MulNXUINode::CallUINode(std::string&& Name) {
    return this->MainContext->CallUINode(Name);
}
bool MulNXUINode::SetNextUINode(std::string&& Name) {
	this->MainContext->next = std::move(Name);
	return true;
}

bool MulNXUINode::SendToOwner(MulNX::Message&& Msg) {
	if (this->WaitingResponse) {
		this->MainContext->EnableErrorHandle = true;
		return false;
	}
	this->OwnerMsgChannel->PushMessage(std::move(Msg));
	this->WaitingResponse = true;
	return true;
}
MulNX::Message MulNXUINode::CreateMsg(uint32_t SubType) {
	MulNX::Message Msg(MulNX::MsgType::UISystem_UICommand);
	Msg.SubType = SubType;
	Msg.pMsgChannel = this->MyMsgChannel;
	return Msg;
}
MulNXHandle MulNXUINode::CreateStringHandle(std::string&& Str) {
	MulNX::Core::Core* pCore = this->MainContext->Core;
	auto pStr = MulNX::Base::make_any_unique<std::string>(std::move(Str));
	return pCore->IHandleSystem().RegisteUnique(std::move(pStr));
}

MulNX::Base::any_unique_ptr MulNXUINode::Create(const MulNX::ModuleBase* const MB) {
	auto SContext = MulNX::Base::make_any_unique<MulNXUINode>();
	MulNXUINode* SContextPtr = SContext.get<MulNXUINode>();
	SContextPtr->HModule = MB->HModule;
	SContextPtr->OwnerMsgChannel = MB->MainMsgChannel;
	MulNX::Core::Core* pCore = MB->GetCore();
	SContextPtr->MyMsgChannel = pCore->IMessageManager()
		.GetMessageChannel(pCore->IMessageManager().CreateMessageChannel());
	return SContext;
}
bool MulNXUINode::CreateAndRegiste(MulNX::ModuleBase* const MB, std::string&& Name, std::function<void(MulNXUINode*)>MyFunc) {
    auto SContext = MulNXUINode::Create(MB);
    MulNXUINode* SContextPtr = SContext.get<MulNXUINode>();
    SContextPtr->name = std::move(Name);
    SContextPtr->MyFunc = MyFunc;
    MulNX::Core::Core* pCore = MB->GetCore();
    MulNXHandle hContext = pCore->IHandleSystem().RegisteUnique(std::move(SContext));
    MulNX::Message Msg(MulNX::MsgType::UISystem_ModulePush);
    Msg.Handle = hContext;
    MB->IPublish(std::move(Msg));
    return true;
}