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
		auto [Info, pInfo] = MulNX::Base::make_any_unique<ModuleInfo>();
		
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
    // 先保存模块句柄，因为后续需要使用
    MulNXHandle hModule = Module->HModule;
    // 再创建名称到句柄的映射
    this->NameToHandleMap[Name] = hModule;
    // 然后安全地移动Name到模块内部
    Module->SetName(std::move(Name));
    // 最后将模块保存到Modules中
    this->Modules[hModule] = std::move(Module);
    // 同时记录优先级
    this->PriorityToHandleMap[Priority] = hModule;
	return true;
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
	for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
		if (!this->Modules[hModule]->EntryInit(this->Core)) {
			return false;
		}
	}
	// 完成初始化
	return true;
}

void ModuleManager::PackedVirtualMain() {
	std::shared_lock lock(this->MyThreadMutex);
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
		this->Modules[hModule]->EntryVirtualMain();
	}
}
void ModuleManager::PackedWindows() {
	std::shared_lock lock(this->MyThreadMutex);
    for (auto& [Priority, hModule] : this->PriorityToHandleMap) {
        this->Modules[hModule]->EntryWindows();
    }
}