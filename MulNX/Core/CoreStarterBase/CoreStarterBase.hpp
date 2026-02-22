#pragma once

#include "../ModuleBase/ModuleBase.hpp"

class CoreImpl;
class MulNXUINode;

namespace MulNX {
    namespace Core {
        // 核心启动器基类，定义核心启动的基本接口
        class CoreStarterBase :public MulNX::ModuleBase {
        public:
            // 虚析构
            virtual ~CoreStarterBase() = default;
            bool Init() override {
                return true;
            }
            // 初始化Core的所有系统组件
            bool SystemInit(MulNX::Core::Core* pCore);

            // UI系统启动辅助函数
            void StartUIWith(std::string&& EntryName);
            // 注册主绘制函数
            void RegisteMainDrawWith(std::function<void(MulNXUINode*)>&& MainDrawFunc);

            // 核心完全初始化后回调
            std::function<void()>InitEndCall = nullptr;

            // 所有系统全部启动启动启动！！！
            virtual void StartAll() {}
        };
    }
}