#include "MulNXUINode.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MulNXUISystem/MulNXUIContext/MulNXUIContext.hpp>
#include <MulNX/Systems/MessageManager/IMessageManager.hpp>

void MulNXUINode::Draw() {
	this->MyFunc(this);
}
bool MulNXUINode::CallUINode(std::string&& Name) {
    return this->MainContext->CallUINode(Name);
}
bool MulNXUINode::SetNextUINode(std::string&& Name) {
	this->MainContext->Next = std::move(Name);
	return true;
}

MulNXUINode MulNXUINode::Create(MulNX::ModuleBase* MB) {
    MulNXUINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    node.pMsgManager = MB->GetCore()->ModuleManager()->FindModule<MulNX::IMessageManager>("MessageManager");
    node.buzy = &(MB->UIBusy);
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

bool MulNXUINode::PublishAsync(MulNX::Message&& Msg) {
    if (this->buzy->load(std::memory_order_acquire)) {
        this->MainContext->EnableErrorHandle = true;
        return false;
    }
    this->pMsgManager->Publish(std::move(Msg));
    return true;
}