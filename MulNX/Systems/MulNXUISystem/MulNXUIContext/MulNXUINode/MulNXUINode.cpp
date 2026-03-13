#include "MulNXUINode.hpp"

#include "../MulNXUIContext.hpp"

#include "../../../MessageManager/IMessageManager.hpp"

#include "../../../../Core/Core.hpp"

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

MulNX::any_unique_ptr MulNXUINode::Create(const MulNX::ModuleBase* const MB) {
    auto [Node, pNode] = MulNX::make_any_unique<MulNXUINode>();
	pNode->HModule = MB->HModule;
	pNode->OwnerMsgChannel = MB->MainMsgChannel;
	MulNX::Core::Core* pCore = MB->GetCore();
	pNode->MyMsgChannel = pCore->IMessageManager()
		.GetMessageChannel(pCore->IMessageManager().CreateMessageChannel());
	return std::move(Node);
}
bool MulNXUINode::CreateAndRegiste(MulNX::ModuleBase* const MB, std::string&& Name, std::function<void(MulNXUINode*)>MyFunc) {
    auto Node = MulNXUINode::Create(MB);
    MulNXUINode* pNode = Node.get<MulNXUINode>();
    pNode->name = std::move(Name);
    pNode->MyFunc = MyFunc;
    MulNX::Core::Core* pCore = MB->GetCore();
    MulNXHandle hContext = pCore->IHandleSystem().RegisteUnique(std::move(Node));
    MulNX::Message Msg("UISystem/ModulePush"_hash);
    Msg.Handle = hContext;
    MB->ISys().PublishAsync(std::move(Msg));
    return true;
}