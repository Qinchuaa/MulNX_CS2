#include "PathManager.hpp"

#include "../../Core/Core.hpp"
#include "../../Core/ModuleManager/ModuleManager.hpp"
#include "../IPCer/IPCer.hpp"

bool MulNX::PathManager::Init() {
    this->IPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    this->Root = this->IPCer->GetRoot();
    this->CoreName = this->Core->GetName();
    this->CoreRoot = this->Root / this->CoreName;
    return true;
}

std::filesystem::path MulNX::PathManager::PathGetForModule(const std::string& ModuleName, const std::string& Target) {
    return this->CoreRoot / ModuleName / Target;
}

std::filesystem::path MulNX::PathManager::PathGetForShared(const std::string& Target) {
    return this->Root / Target;
}