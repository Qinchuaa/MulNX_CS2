#include "ModuleManager.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Systems.hpp>

using namespace MulNX::Core;

bool ModuleManager::Init() {
	this->MainMsgChannel = this->ICreateAndGetMessageChannel();
	// 后置消息订阅，参见 PackedInit
	return true;
}
void ModuleManager::ProcessMsg(MulNX::Message* Msg) {
	std::unique_lock lock(this->GetMutex());
	switch (Msg->type) {
	case "ModuleManager/ModuleInfo/Request"_hash: {
		auto [pInfo, raw] = MulNX::make_any_shared<ModuleInfo>();
		
		for (auto& [Name, Handle] : this->NameToHandleMap) {
			raw->Info.push_back(std::make_pair(Name, Handle));
		}

		MulNX::Message msg("ModuleManager/ModuleInfo/Response"_hash);
        msg.asp = std::move(pInfo);
        this->ISys().PublishAsync(std::move(msg));
	}
	}
}
void ModuleManager::VirtualMain() {
	this->EntryProcessMsg();
	this->PackedVirtualMain();
}

bool ModuleManager::RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module, int Priority) {
	std::unique_lock lock(this->GetMutex());
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
        .CreateModule<MulNX::MessageManager>("MessageManager", 5)// 消息管理器模块
        .CreateModule<MulNX::UISystem>("UISystem", 7)// UI系统模块
        .CreateModule<MulNX::Debugger>("Debugger", 10)// 调试器模块
        .CreateModule<MulNX::HandleSystem>("HandleSystem", 20)// 句柄系统模块
        .CreateModule<MulNX::KeyTracker>("KeyTracker", 50)// 按键追踪器模块
        .CreateModule<MulNX::GlobalVars>("GlobalVars", 70)// 全局变量模块
        // ID 100(最后一个模块)分配给3D抽象层
        ;

    return *this;
}
MulNX::ModuleBase* ModuleManager::FindModule(const std::string& Name) {
	std::shared_lock lock(this->GetMutex());
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

bool ModuleManager::PackedInit() {
	std::shared_lock lock(this->GetMutex());
    // 通过有序的初始化任务列表进行初始化，尽管Modules是无序的
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
        auto* pModule = this->Modules[hModule].get();
        if (!pModule->EntryInit(this->Core)) {
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

void ModuleManager::PackedVirtualMain() {
	std::shared_lock lock(this->GetMutex());
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
        auto* pModule = this->Modules[hModule].get();
        if (!pModule->HasParent()) {
            pModule->EntryVirtualMain();
        }
    }
}