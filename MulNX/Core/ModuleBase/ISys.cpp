#include "ModuleBase.hpp"

#include "../Core.hpp"
#include "../CoreImpl.hpp"

#include "../../Systems/MessageManager/IMessageManager.hpp"
#include "../../Systems/HandleSystem/IHandleSystem.hpp"

#include "../../ThirdParty/All_ImGui.hpp"

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