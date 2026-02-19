#include "Core.hpp"

#include "ModuleManager/ModuleManager.hpp"

#include "../Systems/Debugger/Debugger.hpp"
#include "../Systems/HandleSystem/HandleSystem.hpp"
#include "../Systems/IPCer/IPCer.hpp"
#include "../Systems/KeyTracker/KeyTracker.hpp"
#include "../Systems/MessageManager/MessageManager.hpp"
#include "../Systems/MulNXGlobalVars/MulNXGlobalVars.hpp"
#include "../Systems/MulNXUISystem/MulNXUISystem.hpp"
#include "../Systems/AbstractLayer3D/AbstractLayer3D.hpp"

// 实现类，掌握所有的核心和系统模块实例
class CoreImpl {
public:
    // 核心模块

    MulNX::Core::ModuleManager ModuleManager;
public:
    // 构造函数
    CoreImpl() = default;
};