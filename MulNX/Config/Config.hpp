#pragma once

#include <cstdint>
#include <string>
#include <source_location>

using GameTime_t = float;
// 摄像机系统输入输出前置声明
class CameraSystemIO;

// 核心类前置声明

namespace MulNX {
    namespace Core {
        // 核心管理器
        class Core;
        // 模块管理器
        class ModuleManager;
        // 核心启动器基类
        class CoreStarterBase;
    }
    // 模块基类
    class ModuleBase;
    // 调试器接口
    class IDebugger;
    // MulNX句柄系统
    class IHandleSystem;
    class HandleSystem;
    // 跨进程信息接口
    class IPCer;
    // 按键追踪器
    class KeyTracker;
    // 全局变量
    class GlobalVars;
    // 3D抽象层
    class IAbstractLayer3D;
    // MulNX UI系统
    class IUISystem;
    // MulNX消息
    class Message;
    // MulNX消息类型
    using MsgType = size_t;
    // 消息管理器
    class IMessageManager;
    // 消息管道
    class IMessageChannel;
    // 路径管理器
    class PathManager;



    [[noreturn]] void ErrorTerminate(const std::string& Msg,
        const std::source_location& loc = std::source_location::current());
}
class MulNXUINode;

// 辅助模板：将函数签名 R(Args...) 转换为对应的函数指针类型 R(*)(Args...)
template<typename T>
struct MulNXFunc;

template<typename R, typename... Args>
struct MulNXFunc<R(Args...)> {
    using type = R(*)(Args...);
};