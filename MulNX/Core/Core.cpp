#include "Core.hpp"
#include "CoreStarterBase/CoreStarterBase.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <MulNX/Systems/ISystems.hpp>

MulNX::Core::Core::Core(std::string&& Name) :
    CoreName(Name) {
    // 创建模块管理器
    this->pModuleManager = std::make_unique<MulNX::Core::ModuleManager>();
}

MulNX::Core::Core* MulNX::Core::Core::Create(std::string&& coreName) {
    auto core = std::make_unique<MulNX::Core::Core>(std::move(coreName));
    auto& Myself = core->pMyself;
    core->pMyself = std::move(core);
    return Myself.get();
}

MulNX::Core::ModuleManager* MulNX::Core::Core::ModuleManager() {
    return this->pModuleManager.get();
}

// 专用初始化函数
void MulNX::Core::Core::Init() {
	// 通过核心启动器进行系统初始化
	this->pCoreStarter->SystemInit(this);
	return;
}

std::string MulNX::Core::Core::GetName() {
    return this->CoreName;
}