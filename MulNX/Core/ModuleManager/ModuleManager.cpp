#include "ModuleManager.hpp"
#include "../Core.hpp"
#include "../../Systems/HandleSystem/IHandleSystem.hpp"
#include "../../Systems/MessageManager/IMessageManager.hpp"

#include <deque>

using namespace MulNX::Core;

bool ModuleManager::Init() {
	this->MainMsgChannel = this->ICreateAndGetMessageChannel();
	(*this->MainMsgChannel)
		.Subscribe(MulNX::MsgType::ModuleManager_RequestModuleInfo);

	return true;
}
void ModuleManager::ProcessMsg(MulNX::Message* Msg) {
	std::unique_lock lock(this->MyThreadMutex);
	switch (Msg->Type) {
	case MulNX::MsgType::ModuleManager_RequestModuleInfo: {
		auto Info = MulNX::Base::make_any_unique<ModuleInfo>();
		auto pInfo = Info.get<ModuleInfo>();
		
		for (auto& [Name, Handle] : this->NameToHandleMap) {
			pInfo->Info.push_back(std::make_pair(Name, Handle));
		}

		MulNXHandle hInfo = this->Core->IHandleSystem().RegisteUnique(std::move(Info));

		MulNX::Message ResponseMsg(MulNX::MsgType::ModuleManager_ResponseModuleInfo);
		ResponseMsg.Handle = hInfo;
		Msg->pMsgChannel->PushMessage(std::move(ResponseMsg));
	}
	}
}
void ModuleManager::VirtualMain() {
	this->EntryProcessMsg();
	this->PackedVirtualMain();
}
void ModuleManager::Windows() {
	this->PackedWindows();
}

bool ModuleManager::RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module, std::string&& Name, int Priority) {
	std::unique_lock lock(this->MyThreadMutex);
	if (!Module->HModule.IsValid()) {
		Module->HModule = MulNXHandle::CreateHandle();
	}
	MulNXHandle hModule = Module->HModule;
	this->Modules[hModule] = std::move(Module);
	this->NameToHandleMap[Name] = hModule;
	this->HandleToNameMap[hModule] = std::move(Name);
	this->InitPriority[Priority] = hModule;
	return true;
}
bool ModuleManager::RegisteModules(ModulePack&& ModulePack) {
    bool Result = true;
    for (auto& ModuleBuffer : ModulePack.Modules) {
        Result &= this->RegisteModule(
            std::move(ModuleBuffer.Module),
            std::move(ModuleBuffer.Name),
            ModuleBuffer.Priorty);
    }
    return Result;
}
MulNX::ModuleBase* ModuleManager::FindModule(const std::string& Name) {
	std::shared_lock lock(this->MyThreadMutex);
	auto it = this->NameToHandleMap.find(Name);
	if (it == this->NameToHandleMap.end()) {
		return nullptr;
	}
	MulNXHandle HModule = it->second;
	auto mit = this->Modules.find(HModule);
	if (mit == this->Modules.end()) {
		return nullptr;
	}
	return mit->second.get();
}

bool ModuleManager::PackedInit() {
	std::shared_lock lock(this->MyThreadMutex);
    // 通过有序的初始化任务列表进行初始化，尽管Modules是无序的
	for (auto& [Priority, HModule] : this->InitPriority) {
		if (!this->Modules[HModule]->EntryInit(this->Core)) {
			return false;
		}
	}
	// 完成初始化
	return true;
}

void ModuleManager::PackedVirtualMain() {
	std::shared_lock lock(this->MyThreadMutex);
	for (auto& Module : this->Modules) {
		Module.second->EntryVirtualMain();
	}
}
void ModuleManager::PackedWindows() {
	std::shared_lock lock(this->MyThreadMutex);
	for (auto& Module : this->Modules) {
		Module.second->EntryWindows();
	}
}