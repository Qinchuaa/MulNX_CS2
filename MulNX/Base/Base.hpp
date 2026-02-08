#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Math/Math.hpp"
#include "any_smart_ptr/any_smart_ptr.hpp"
#include "vmt/vmt.hpp"
#include "TripleBuffer/TripleBuffer.hpp"
#include "CharUtility/CharUtility.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <filesystem>
#include <functional>
#include <memory>
#include <variant>
#include <array>

typedef float GameTime_t;

// 句柄内容

// MulNX资源句柄
class MulNXHandle;

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
    //3D抽象层
    class IAbstractLayer3D;
    // MulNX UI系统
    class IUISystem;
    //MulNX消息
    class Message;
    //MulNX消息类型
    enum class MsgType :uint32_t;
    //消息管理器
    class IMessageManager;
    //消息管道
    class IMessageChannel;
}
// MulNX资源句柄
class MulNXHandle {
private:
    constexpr inline static uint64_t Invalid = 0xFFFFFFFFFFFFFFFF;
    inline static std::atomic<uint64_t> CurrentHandleValue = 16;
    uint64_t Value;
public:
    // 默认构造函数，创建无效句柄
    MulNXHandle() {
        this->Value = MulNXHandle::Invalid;
    }
    static MulNXHandle CreateHandle() {
        MulNXHandle handle{};
        handle.Value = MulNXHandle::CurrentHandleValue.fetch_add(1);
        return handle;
    }
    bool IsValid()const {
        return this->Value != MulNXHandle::Invalid;
    }
    uint64_t GetValue()const {
        return this->Value;
    }
    bool operator == (const MulNXHandle& Other)const {
        return this->Value == Other.Value;
    }
};
namespace std {
    template<>
    struct hash<MulNXHandle> {
        size_t operator()(const MulNXHandle& Handle)const noexcept {
            return std::hash<uint64_t>{}(Handle.GetValue());
        }
    };
}