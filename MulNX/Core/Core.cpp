#include "Core.hpp"
#include "CoreStarterBase/CoreStarterBase.hpp"
#include "ModuleManager/ModuleManager.hpp"

#include "../Systems/Debugger/Debugger.hpp"
#include "../Systems/HandleSystem/HandleSystem.hpp"
#include "../Systems/IPCer/IPCer.hpp"
#include "../Systems/KeyTracker/KeyTracker.hpp"
#include "../Systems/MessageManager/MessageManager.hpp"
#include "../Systems/MulNXGlobalVars/MulNXGlobalVars.hpp"
#include "../Systems/MulNXUISystem/MulNXUISystem.hpp"
#include "../Systems/AbstractLayer3D/AbstractLayer3D.hpp"

MulNX::Core::Core::Core(std::string&& Name) :
    CoreName(Name) {
    // 创建模块管理器
    this->pModuleManager = std::make_unique<MulNX::Core::ModuleManager>();
}

bool MulNX::Core::Core::SetCoreStarter(std::unique_ptr<CoreStarterBase> Starter) {
	if (!Starter) {
		return false;
	}
	this->pCoreStarter = std::move(Starter);
	return true;
}

MulNX::Core::Core* MulNX::Core::Core::Create(std::string&& CoreName) {
    static MulNX::Core::Core* pCore = nullptr;
    if (pCore) {
        MulNX::ErrorTerminate("核心创建函数只允许被调用一次！");
    }
    pCore = new MulNX::Core::Core(std::move(CoreName));
    return pCore;
}

MulNX::Core::ModuleManager* MulNX::Core::Core::ModuleManager() {
    return this->pModuleManager.get();
}

// 获取子模块的接口实现

MulNX::IHandleSystem& MulNX::Core::Core::IHandleSystem() {
    static MulNX::IHandleSystem* pIHandleSystem = nullptr;
    if (!pIHandleSystem) {
        pIHandleSystem = this->ModuleManager()->FindModule<MulNX::IHandleSystem>("HandleSystem");
    }
    return *pIHandleSystem;
}
MulNX::IUISystem& MulNX::Core::Core::IUISystem() {
    static MulNX::IUISystem* pIUISystem = nullptr;
    if (!pIUISystem) {
        pIUISystem = this->ModuleManager()->FindModule<MulNX::IUISystem>("UISystem");
    }
    return *pIUISystem;
}
MulNX::IPCer& MulNX::Core::Core::IPCer() {
    static MulNX::IPCer* pIPCer = nullptr;
    if (!pIPCer) {
        pIPCer = this->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    }
    return *pIPCer;
}
MulNX::IMessageManager& MulNX::Core::Core::IMessageManager() {
    static MulNX::IMessageManager* pIMessageManager = nullptr;
    if (!pIMessageManager) {
        pIMessageManager = this->ModuleManager()->FindModule<MulNX::IMessageManager>("MessageManager");
    }
    return *pIMessageManager;
}

// 专用初始化函数
void MulNX::Core::Core::Init() {
    // 核心启动器初始化
	this->pCoreStarter->EntryInit(this);
	// 通过核心启动器进行系统初始化
	this->pCoreStarter->SystemInit(this);

	this->ModuleManager()->FindModule<MulNX::GlobalVars>("GlobalVars")->SystemReady = true;

	// 初始化注册模块
	this->pModuleManager->PackedInit();

	this->pCoreStarter->StartAll();
	this->pCoreStarter->InitEndCall();

	return;
}

// 主逻辑
void MulNX::Core::Core::VirtualMain() {
	// 包装的，所有的模块的VirtualMain
	this->pModuleManager->EntryVirtualMain();
	return;
}

std::string MulNX::Core::Core::GetName() {
    return this->CoreName;
}