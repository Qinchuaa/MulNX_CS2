#include "ModuleManager.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

using namespace MulNX::Core;

bool ModuleManager::Init() {
    // 后置消息订阅，参见 PackedInit
    this->SendTask("MulNXMain", [this]()->bool {
        this->EntryProcessMsg();
        return true;
        });
    return true;
}
void ModuleManager::ProcessMsg(MulNX::Message& Msg) {
    std::unique_lock lock(this->smutex);
    switch (Msg.type) {
    case "ModuleManager/ModuleInfo/Request"_hash: {
        auto [pInfo, raw] = MulNX::make_any_shared<ModuleInfo>();
        for (const auto& [Name, Handle] : this->NameToHandleMap) {
            raw->Info.push_back(std::make_pair(Name, Handle));
        }
        MulNX::Message msg("ModuleManager/ModuleInfo/Response"_hash);
        msg.asp = std::move(pInfo);
        this->ISys().PublishAsync(std::move(msg));
    }
    }
}

bool ModuleManager::RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module, int Priority) {
    std::unique_lock lock(this->smutex);
    if (!Module->HModule.IsValid()) {
        Module->HModule = MulNXHandle::CreateHandle();
    }
    // 先保存模块句柄，因为后续需要使用
    MulNXHandle hModule = Module->HModule;
    // 再创建名称到句柄的映射
    this->NameToHandleMap[Module->GetName()] = hModule;
    // 最后将模块保存到Modules中
    this->Modules[hModule] = std::move(Module);
    // 同时记录优先级
    this->PriorityToHandleMap[Priority] = hModule;
    return true;
}
ModuleManager& ModuleManager::CreateSystemModules() {
    (*this)
        .CreateModule<MulNX::IPCer>("IPCer", 3)// IPC模块
        .CreateModule<MulNX::PathManager>("PathManager", 4)// 路径管理器模块
        .CreateModule<MulNX::I18nManager>("I18nManager", 5)
        .CreateModule<MulNX::MessageManager>("MessageManager", 6)// 消息管理器模块
        .CreateModule<MulNX::UISystem>("UISystem", 7)// UI系统模块
        .CreateModule<MulNX::Logger>("Logger", 8)
        .CreateModule<MulNX::Debugger>("Debugger", 10)// 调试器模块
        .CreateModule<MulNX::HandleSystem>("HandleSystem", 20)// 句柄系统模块
        .CreateModule<MulNX::InputSystem>("InputSystem", 50)// 输入系统模块
        .CreateModule<MulNX::GlobalVars>("GlobalVars", 70)// 全局变量模块
        .CreateModule<MulNX::TaskSystem>("TaskSystem", 80)// 任务系统
        // ID 100(最后一个模块)分配给3D抽象层
        ;

    return *this;
}
MulNX::ModuleBase* ModuleManager::FindModule(const std::string& Name) {
    std::shared_lock lock(this->smutex);
    auto it = this->NameToHandleMap.find(Name);
    if (it == this->NameToHandleMap.end()) {
        MulNX::ErrorTerminate("查找模块错误 0x1");
        return nullptr;
    }
    MulNXHandle HModule = it->second;
    auto mit = this->Modules.find(HModule);
    if (mit == this->Modules.end()) {
        MulNX::ErrorTerminate("查找模块错误 0x2");
        return nullptr;
    }
    return mit->second.get();
}
MulNX::IAbstractLayer3D* ModuleManager::FindAbstractLayer3D() {
    return this->FindModule<MulNX::IAbstractLayer3D>(this->AbstractLayer3DName);
}

bool ModuleManager::ModulesBaseInit() {
    std::shared_lock lock(this->smutex);
    // 通过有序的初始化任务列表进行初始化，尽管Modules是无序的
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
        auto* pModule = this->Modules[hModule].get();
        if (!pModule->BaseInit(this->Core)) {
            MulNX::ErrorTerminate("在模块初始化时出现错误，模块名：" + pModule->GetName());
            return false;
        }
    }
    this->ISys().LogInfo("注意: AbstractLayer3D(3D抽象层)被绑定为" + this->AbstractLayer3DName);

    // 进行后置消息订阅
    this->ISys()
        .SubscribeAsync("ModuleManager/ModuleInfo/Request");
    // 完成初始化
    return true;
}

bool ModuleManager::ModulesInit() {
    std::shared_lock lock(this->smutex);
    // 通过有序的初始化任务列表进行初始化，尽管Modules是无序的
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
        auto* pModule = this->Modules[hModule].get();
        if (!pModule->EntryInit()) {
            MulNX::ErrorTerminate("在模块初始化时出现错误，模块名：" + pModule->GetName());
            return false;
        }
    }
    this->ISys().LogInfo("注意: AbstractLayer3D(3D抽象层)被绑定为" + this->AbstractLayer3DName);

    // 完成初始化
    return true;
}