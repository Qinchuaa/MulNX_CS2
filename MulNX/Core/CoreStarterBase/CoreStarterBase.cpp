#include "CoreStarterBase.hpp"

#include "../CoreImpl.hpp"

using namespace MulNX::Core;

bool CoreStarterBase::SystemInit(CoreImpl* pImpl, MulNX::Core::Core* pCore) {
	// 无依赖核心基础模块初始化
	pImpl->ModuleManager.EntryInit(pCore);
	pImpl->MessageManager.EntryInit(pCore);
	pImpl->MessageManager.EntryCreateThread();// 包含线程创建
	pImpl->MessageManager.SetMyThreadDelta(10);// 注意，此模块内部动态调整频率
	pImpl->KT.EntryCreateThread();// 包含线程创建
	pImpl->KT.SetMyThreadDelta(3);
	pImpl->IPCer.EntryInit(pCore);
	pImpl->Debugger.EntryInit(pCore);
	pImpl->GlobalVars.EntryInit(pCore);
	pImpl->HandleSystem.EntryInit(pCore);
	// UI模块初始化
	pImpl->UISystem.EntryInit(pCore);

    this->ISys().LogSucc("核心系统组件初始化完成！");
    return true;
}

void CoreStarterBase::StartUIWith(std::string&& EntryName) {
	// UI系统主界面初始化
	auto [StartString, pStartString] = MulNX::Base::make_any_unique<std::string>(std::move(EntryName));
	MulNXHandle hStr = this->Core->IHandleSystem().RegisteUnique(std::move(StartString));
	MulNX::Message StartMsg(MulNX::MsgType::UISystem_Start);
	StartMsg.Handle = hStr;
	this->IPublish(std::move(StartMsg));
}

void CoreStarterBase::RegisteMainDrawWith(std::function<void(MulNXUINode*)>&& MainDrawFunc) {
	// 注册主窗口UI上下文
    auto [UINode, pUINode] = MulNX::Base::make_any_unique<MulNXUINode>();
	pUINode->name = "MainDraw";
	pUINode->MyMsgChannel = this->ICreateAndGetMessageChannel();
	pUINode->MyFunc = MainDrawFunc;
	MulNXHandle hContext = this->Core->IHandleSystem().RegisteUnique(std::move(UINode));
	MulNX::Message Msg(MulNX::MsgType::UISystem_ModulePush);
	Msg.Handle = hContext;
	this->IPublish(std::move(Msg));
}