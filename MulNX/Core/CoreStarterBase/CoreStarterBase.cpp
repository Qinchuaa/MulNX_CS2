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

void MulNX::Core::CoreStarterBase::RegisterMainDrawWith(std::function<void(MulNX::UINode*)>&& MainDrawFunc) {
    // 注册主窗口UI上下文 
    this->SendUINode("MainDraw", std::move(MainDrawFunc));
    this->ISys().LogInfo("发送了主窗口注册指令");
    // UI系统主界面初始化
    auto [msg2, rp] = MulNX::Message::Create<std::string>("UISystem/Start"_hash, "MainDraw");
    this->ISys().PublishAsync(std::move(msg2));
    this->ISys().LogWarning("发送了UI启动指令！渲染即将开始！");
}