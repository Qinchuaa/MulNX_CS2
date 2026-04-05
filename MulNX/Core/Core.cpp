#include "Core.hpp"
#include "CoreStarterBase/CoreStarterBase.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <MulNX/Systems/ISystems.hpp>

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
    return new MulNX::Core::Core(std::move(CoreName));
}

MulNX::Core::ModuleManager* MulNX::Core::Core::ModuleManager() {
    return this->pModuleManager.get();
}

// 专用初始化函数
void MulNX::Core::Core::Init() {
    // 核心启动器初始化
	this->pCoreStarter->EntryInit(this);
	// 通过核心启动器进行系统初始化
	this->pCoreStarter->SystemInit(this);
	// 初始化注册模块
	this->pModuleManager->PackedInit();
    // 开启系统
    this->pCoreStarter->StartAll();
    // 设置系统标志位
    this->ModuleManager()->FindModule<MulNX::GlobalVars>("GlobalVars")->SystemReady.store(true, std::memory_order_release);
    // 执行启动器回调
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