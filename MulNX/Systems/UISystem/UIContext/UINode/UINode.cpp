#include "UINode.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/UISystem/UIContext/UIContext.hpp>
#include <MulNX/Systems/MessageManager/IMessageManager.hpp>

void MulNX::UINode::Draw() {
	this->MyFunc(this);
}
bool MulNX::UINode::CallUINode(std::string&& Name) {
    return this->MainContext->CallUINode(Name);
}
bool MulNX::UINode::SetNextUINode(std::string&& Name) {
	this->MainContext->Next = std::move(Name);
	return true;
}

MulNX::UINode MulNX::UINode::Create(MulNX::ModuleBase* MB) {
    MulNX::UINode node;
    node.hSelf = MulNXHandle::CreateHandle();
    node.HModule = MB->HModule;
    node.pMsgManager = MB->GetCore()->ModuleManager()->FindModule<MulNX::IMessageManager>("MessageManager");
    node.buzy = &(MB->UIBusy);
    return std::move(node);
}

bool MulNX::UINode::PublishAsync(MulNX::Message&& Msg) {
    if (this->buzy->load(std::memory_order_acquire)) {
        this->MainContext->EnableErrorHandle = true;
        return false;
    }
    this->pMsgManager->Publish(std::move(Msg));
    return true;
}