#include "ModuleBase.hpp"

#include "../Core.hpp"
#include "../CoreImpl.hpp"

#include "../../Systems/Systems.hpp"

#include <MulNXThirdParty/All_ImGui.hpp>

MulNX::C_ISys MulNX::ModuleBase::ISys() {
    return C_ISys(this);
}

void MulNX::C_ISys::LogInfo(const std::string& Msg) {
    this->pModuleBase->IDebugger->AddInfo("[" + this->pModuleBase->GetName() + "]" + Msg);
}
void MulNX::C_ISys::LogSucc(const std::string& Msg) {
    this->pModuleBase->IDebugger->AddSucc("[" + this->pModuleBase->GetName() + "]" + Msg);
}
void MulNX::C_ISys::LogWarning(const std::string& Msg) {
    this->pModuleBase->IDebugger->AddWarning("[" + this->pModuleBase->GetName() + "]" + Msg);
}
void MulNX::C_ISys::LogError(const std::string& Msg) {
    this->pModuleBase->IDebugger->AddError("[" + this->pModuleBase->GetName() + "]" + Msg);
}

MulNX::C_ISys& MulNX::C_ISys::SubscribeAsync(const MsgType MsgType) {
    this->pModuleBase->MainMsgChannel->Subscribe(MsgType);
    return *this;
}
void MulNX::C_ISys::PublishAsync(MulNX::Message&& Msg) {
    this->pModuleBase->IMsgManager->Publish(std::move(Msg));
}
void MulNX::C_ISys::PublishAsync(MulNX::MsgType Msg) {
    this->pModuleBase->IMsgManager->Publish(MulNX::Message(Msg));
}