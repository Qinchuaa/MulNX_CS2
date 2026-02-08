#include "Core.hpp"

#include "ModuleManager/ModuleManager.hpp"

#include "../Systems/Debugger/Debugger.hpp"
#include "../Systems/HandleSystem/HandleSystem.hpp"
#include "../Systems/IPCer/IPCer.hpp"
#include "../Systems/KeyTracker/KeyTracker.hpp"
#include "../Systems/MessageManager/MessageManager.hpp"
#include "../Systems/MulNXiGlobalVars/MulNXiGlobalVars.hpp"
#include "../Systems/MulNXUISystem/MulNXUISystem.hpp"
#include "../Systems/AbstractLayer3D/AbstractLayer3D.hpp"

using namespace MulNX::Core;

// 实现类，掌握所有的核心和系统模块实例
class CoreImpl {
public:
    // 核心模块

    MulNX::Core::ModuleManager ModuleManager;

    // 系统模块

    MulNX::KeyTracker KT;
    MulNX::HandleSystem HandleSystem;
    MulNX::MessageManager MessageManager;
    MulNX::GlobalVars GlobalVars;
    MulNX::UISystem UISystem;
    MulNX::IPCer IPCer;
    MulNX::Debugger Debugger;
    MulNX::AbstractLayer3D AL3D;

public:
    // 构造函数
    CoreImpl() = default;
};