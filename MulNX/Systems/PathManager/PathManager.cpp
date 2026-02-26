#include "PathManager.hpp"

#include "../../Core/Core.hpp"
#include "../../Core/ModuleManager/ModuleManager.hpp"
#include "../IPCer/IPCer.hpp"

#include <MulNXThirdParty/All_pugixml.hpp>

bool MulNX::PathManager::Init() {
    this->IPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    this->Root = this->IPCer->GetRoot();
    this->CoreName = this->Core->GetName();
    this->CoreRoot = this->Root / this->CoreName;
    this->LoadPathLists(this->Root / "PathLists.xml");
    this->CheckShared();
    return true;
}

bool MulNX::PathManager::LoadPathLists(const std::filesystem::path& xmlPath) {
    pugi::xml_document xml;
    pugi::xml_parse_result result = xml.load_file(xmlPath.c_str());
    if (!result)MulNX::ErrorTerminate("PathLists.xml加载失败，请检查");
    auto nodePath = xml.child("Path");
    auto nodeShared = nodePath.child("Shared");
    if (!nodeShared)MulNX::ErrorTerminate("PathLists.xml文件疑似损坏");
    for (const auto& nodeShardPath : nodeShared.children("Path")) {
        // 这里不需要担心损坏，最差是空
        this->Shareds.push_back(nodeShardPath.attribute("Name").as_string());
    }
    return true;
}
bool MulNX::PathManager::CheckShared() {
    for (const auto& shared : this->Shareds) {
        std::filesystem::path Path = this->Root / shared;
        if (std::filesystem::exists(Path)) {
            this->ISys().LogInfo("检测到共享目录已创建：" + Path.string());
        }
        else {
            try {
                std::filesystem::create_directory(Path);
                this->ISys().LogSucc("成功创建新的共享目录" + Path.string());
            }
            catch (const std::exception& e) {
                this->ISys().LogError("在创建共享目录" + Path.string() + "时出现错误：" + e.what());
            }
        }
    }
    return true;
}

std::filesystem::path MulNX::PathManager::PathGetForModule(const std::string& ModuleName, const std::string& Target) {
    return this->CoreRoot / ModuleName / Target;// 等价于this->Root / this->CoreName / ModuleName / Target
}

std::filesystem::path MulNX::PathManager::PathGetForShared(const std::string& Target) {
    return this->Root / Target;
}