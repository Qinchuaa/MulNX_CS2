#include "../ModuleBase.hpp"

#include "../../Core.hpp"

#include "../../../Systems/ISystems.hpp"

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

void MulNX::C_ISys::LogLine() {
    this->LogInfo("+------------------------------------------------+");
}

MulNX::C_ISys& MulNX::C_ISys::SubscribeAsync(const std::string& MsgType) {
    this->pModuleBase->MainMsgChannel->Subscribe(MsgType);
    this->LogSucc("成功订阅消息：" + MsgType);
    return *this;
}
void MulNX::C_ISys::PublishAsync(MulNX::Message&& Msg) {
    this->pModuleBase->IMsgManager->Publish(std::move(Msg));
}
void MulNX::C_ISys::PublishAsync(MulNX::MsgType Msg) {
    this->pModuleBase->IMsgManager->Publish(MulNX::Message(Msg));
}

std::filesystem::path MulNX::C_ISys::PathGet(const std::string& Target) {
    return this->pModuleBase->pPathManager->PathGetForModule(this->pModuleBase->GetName(), Target);
}

MulNX::PathManager* MulNX::C_ISys::PathManager() {
    return this->pModuleBase->pPathManager;
}