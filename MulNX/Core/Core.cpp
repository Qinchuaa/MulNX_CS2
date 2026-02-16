#include "CoreImpl.hpp"
#include "CoreStarterBase/CoreStarterBase.hpp"

// 构造函数和析构函数需要管理pImpl的生命周期
Core::Core() {
	// 创建Impl实例
	this->pImpl = std::make_unique<CoreImpl>();
}

Core::~Core() {
	return;
}

bool Core::SetCoreStarter(std::unique_ptr<CoreStarterBase> Starter) {
	if (!Starter) {
		return false;
	}
	this->pCoreStarter = std::move(Starter);
	return true;
}

ModuleManager* Core::ModuleManager() {
	return &this->pImpl->ModuleManager;
}

// 获取子模块的接口实现

MulNX::IHandleSystem&	Core::IHandleSystem() { return this->pImpl->HandleSystem; }
MulNX::IUISystem&		Core::IUISystem() { return this->pImpl->UISystem; }
MulNX::IPCer&			Core::IPCer() { return this->pImpl->IPCer; }
MulNX::IMessageManager& Core::IMessageManager() { return this->pImpl->MessageManager; }

// 专用初始化函数
void Core::Init() {
	// 核心启动器初始化
	this->pCoreStarter->EntryInit(this);
	// 通过核心启动器进行系统初始化
	this->pCoreStarter->SystemInit(this->pImpl.get(), this);

	// 3D抽象层初始化
	this->pImpl->AL3D.EntryInit(this);

	this->pImpl->GlobalVars.SystemReady = true;

	// 初始化注册模块
	this->pImpl->ModuleManager.PackedInit();

	this->pCoreStarter->StartAll();
	this->pCoreStarter->InitEndCall();

	return;
}

// 主逻辑
void Core::VirtualMain() {
	this->pImpl->GlobalVars.EntryVirtualMain();
	this->pImpl->AL3D.EntryVirtualMain();
	this->pImpl->Debugger.EntryVirtualMain();
	// 包装的，所有的模块的VirtualMain
	this->pImpl->ModuleManager.EntryVirtualMain();
    // 包装的，所有模块的窗口逻辑
    this->pImpl->Debugger.EntryWindows();
    this->pImpl->ModuleManager.EntryWindows();

	return;
}