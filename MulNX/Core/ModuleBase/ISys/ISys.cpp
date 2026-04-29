#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>

MulNX::C_ISys MulNX::ModuleBase::ISys() {
    return C_ISys(this);
}

void MulNX::C_ISys::LogInfo(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Info"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogSucc(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Succ"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogWarning(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Warning"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
}
void MulNX::C_ISys::LogError(const std::string& Msg) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Log/Error"_hash);
    rp->str1 = this->pModuleBase->GetName();
    rp->str2 = Msg;
    rp->timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
    this->PublishAsync(std::move(msg));
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
    this->pModuleBase->pMsgManager->Publish(std::move(Msg));
}
void MulNX::C_ISys::PublishAsync(MulNX::MsgType Msg) {
    this->pModuleBase->pMsgManager->Publish(MulNX::Message(Msg));
}
void MulNX::C_ISys::AsyncCommand(std::string&& cmd) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Game/Command"_hash);
    rp->str1 = std::move(cmd);
    this->PublishAsync(std::move(msg));
}

std::filesystem::path MulNX::C_ISys::PathGet(const std::string& Target) {
    return this->pModuleBase->pPathManager->PathGetForModule(this->pModuleBase->GetName(), Target);
}

MulNX::PathManager* MulNX::C_ISys::PathManager() {
    return this->pModuleBase->pPathManager;
}