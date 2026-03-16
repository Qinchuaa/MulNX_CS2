#include "MulNXUINode.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/MulNXUISystem/MulNXUIContext/MulNXUIContext.hpp>
#include <MulNX/Systems/MessageManager/IMessageManager.hpp>



void MulNXUINode::Draw() {
	if (this->MyMsgChannel->HasMessage()) {
		MulNX::Message Msg;
		while(this->MyMsgChannel->PullMessage(Msg)){
			if(Msg.type == "UISystem/ModuleResponse"_hash){
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
	this->MainContext->Next = std::move(Name);
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
MulNX::Message MulNXUINode::CreateMsg(int SubType) {
	MulNX::Message Msg("UISystem/UICommand"_hash);
	Msg.p2.i = SubType;
	Msg.pMsgChannel = this->MyMsgChannel;
	return Msg;
}
MulNXHandle MulNXUINode::CreateStringHandle(std::string&& Str) {
	MulNX::Core::Core* pCore = this->MainContext->Core;
    auto [str, pstr] = MulNX::make_any_unique<std::string>(std::move(Str));
    return pCore->IHandleSystem().RegisteUnique(std::move(str));
}

MulNXUINode MulNXUINode::Create(const MulNX::ModuleBase* const MB) {
    MulNXUINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    node.OwnerMsgChannel = MB->MainMsgChannel;
	MulNX::Core::Core* pCore = MB->GetCore();
    node.MyMsgChannel = pCore->IMessageManager()
		.GetMessageChannel(pCore->IMessageManager().CreateMessageChannel());
    return std::move(node);
}
bool MulNXUINode::CreateAndRegiste(MulNX::ModuleBase* const MB, std::string&& Name, std::function<void(MulNXUINode*)>MyFunc) {
    auto node = MulNXUINode::Create(MB);
    node.name = std::move(Name);
    node.MyFunc = MyFunc;
    MulNX::Core::Core* pCore = MB->GetCore();
    auto [pNode, raw] = MulNX::make_any_shared<MulNXUINode>(std::move(node));
    MulNX::Message Msg("UISystem/ModulePush"_hash);
    Msg.asp = std::move(pNode);
    MB->ISys().PublishAsync(std::move(Msg));
    return true;
}