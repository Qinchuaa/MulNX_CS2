#include "CoreStarterBase.hpp"

#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

using namespace MulNX::Core;

bool CoreStarterBase::SystemInit(MulNX::Core::Core* pCore) {
    // 无依赖核心基础模块初始化
    this->Core->ModuleManager()->SetName("ModuleManager");
    this->Core->ModuleManager()->EntryInit(pCore);

    this->ISys().LogSucc("核心系统组件初始化完成！");
    return true;
}

void CoreStarterBase::StartUIWith(std::string&& EntryName) {
	// UI系统主界面初始化
	auto [StartString, pStartString] = MulNX::make_any_shared<std::string>(std::move(EntryName));
	MulNX::Message StartMsg("UISystem/Start"_hash);
	StartMsg.asp = std::move(StartString);
	this->ISys().PublishAsync(std::move(StartMsg));
}

void CoreStarterBase::RegisterMainDrawWith(std::function<void(MulNXUINode*)>&& MainDrawFunc) {
	// 注册主窗口UI上下文
    auto [UINode, pUINode] = MulNX::make_any_shared<MulNXUINode>();
	pUINode->name = "MainDraw";
	pUINode->MyFunc = MainDrawFunc;
	MulNX::Message Msg("UISystem/ModulePush"_hash);
    Msg.asp = std::move(UINode);
	this->ISys().PublishAsync(std::move(Msg));
}