#include "CoreStarterBase.hpp"

#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

bool MulNX::Core::CoreStarterBase::SystemInit(MulNX::Core::Core* pCore) {
    // 一阶段初始化核心启动器
    this->BaseInit(pCore);
    // 一阶段初始化模块管理器
    this->Core->ModuleManager()->SetName("ModuleManager");
    this->Core->ModuleManager()->BaseInit(this->Core);
    // 一阶段初始化注册模块
    this->Core->ModuleManager()->ModulesBaseInit();
    // 二阶段初始化模块管理器
    this->Core->ModuleManager()->EntryInit();
    // 二阶段初始化核心启动器
    this->EntryInit();
    // 二阶段初始化注册模块
    this->Core->ModuleManager()->ModulesInit();
    // 开启系统
    this->ActiveSystem();
    // 设置系统标志位
    this->Core->ModuleManager()->FindModule<MulNX::GlobalVars>("GlobalVars")->SystemReady.store(true, std::memory_order_release);
    // 执行启动器回调
    this->InitEndCall();
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