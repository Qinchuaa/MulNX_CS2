#include "CoreStarterBase.hpp"

#include "../CoreImpl.hpp"

#include "../../Systems/Debugger/Debugger.hpp"
#include "../../Systems/HandleSystem/HandleSystem.hpp"
#include "../../Systems/IPCer/IPCer.hpp"
#include "../../Systems/KeyTracker/KeyTracker.hpp"
#include "../../Systems/MessageManager/MessageManager.hpp"
#include "../../Systems/MulNXGlobalVars/MulNXGlobalVars.hpp"
#include "../../Systems/MulNXUISystem/MulNXUISystem.hpp"
#include "../../Systems/AbstractLayer3D/AbstractLayer3D.hpp"


using namespace MulNX::Core;

bool CoreStarterBase::SystemInit(CoreImpl* pImpl, MulNX::Core::Core* pCore) {
    // 无依赖核心基础模块初始化
    pImpl->ModuleManager.SetName("ModuleManager");
    pImpl->ModuleManager.EntryInit(pCore);
    this->Core->ModuleManager()->FindModule<MulNX::MessageManager>("MessageManager")->EntryCreateThread();// 包含线程创建
	this->Core->ModuleManager()->FindModule<MulNX::MessageManager>("MessageManager")->SetMyThreadDelta(10);// 注意，此模块内部动态调整频率
	this->Core->ModuleManager()->FindModule<MulNX::KeyTracker>("KeyTracker")->EntryCreateThread();// 包含线程创建
    this->Core->ModuleManager()->FindModule<MulNX::KeyTracker>("KeyTracker")->SetMyThreadDelta(3);

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