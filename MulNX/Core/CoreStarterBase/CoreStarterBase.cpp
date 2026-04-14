#include "CoreStarterBase.hpp"

#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

bool MulNX::Core::CoreStarterBase::SystemInit(MulNX::Core::Core* pCore) {
    // 无依赖核心基础模块初始化
    this->Core->ModuleManager()->SetName("ModuleManager");
    this->Core->ModuleManager()->EntryInit(pCore);
    this->ISys().LogSucc("模块管理器初始化完成，即将初始化核心基础模块...");
    return true;
}

void MulNX::Core::CoreStarterBase::RegisterMainDrawWith(std::function<void(MulNXUINode*)>&& MainDrawFunc) {
    // 注册主窗口UI上下文
    auto [UINode, pUINode] = MulNX::make_any_shared<MulNXUINode>();
    pUINode->name = "MainDraw";
    pUINode->MyFunc = MainDrawFunc;
    MulNX::Message Msg("UISystem/ModulePush"_hash);
    Msg.asp = std::move(UINode);
    this->ISys().PublishAsync(std::move(Msg));
    // UI系统主界面初始化
    auto [msg, rp] = MulNX::Message::Create<std::string>("UISystem/Start"_hash, "MainDraw");
    this->ISys().PublishAsync(std::move(msg));
}