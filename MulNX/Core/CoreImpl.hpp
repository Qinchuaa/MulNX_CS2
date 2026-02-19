#include "Core.hpp"

#include "ModuleManager/ModuleManager.hpp"

// 实现类，掌握所有的核心和系统模块实例
class CoreImpl {
public:
    // 核心模块

    MulNX::Core::ModuleManager ModuleManager;
public:
    // 构造函数
    CoreImpl() = default;
};