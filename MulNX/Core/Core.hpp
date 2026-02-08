#pragma once
// 整个系统的核心，负责所有子模块的生命周期管理，初始化，主循环调用等
// 同时提供各子模块的接口访问
#include "../Base/Base.hpp"

// 前向声明实现类
class CoreImpl;

namespace MulNX {
    namespace Core {
        class Core {
            friend class MulNX::ModuleBase;
        private:
            // 数据存储：

            // Impl的指针
            std::unique_ptr<CoreImpl> pImpl;

			// 核心启动器指针
            std::unique_ptr<MulNX::Core::CoreStarterBase> pCoreStarter = nullptr;
        public:
            // 构造函数
            Core();
            // 析构函数
            ~Core();
            // 删除拷贝构造
            Core(const Core&) = delete;
            // 删除拷贝赋值
            Core& operator=(const Core&) = delete;

            // 初始化
            void Init();
            // 主循环
            void VirtualMain();

            // 获取子模块的接口
            MulNX::IMessageManager& IMessageManager();
            MulNX::IUISystem& IUISystem();
            MulNX::IPCer& IPCer();
            MulNX::IHandleSystem& IHandleSystem();

            // 获取实现
            MulNX::Core::CoreStarterBase* GetStarter() { return this->pCoreStarter.get(); }

            // 设置启动器
			bool SetCoreStarter(std::unique_ptr<CoreStarterBase> Starter);

            // 模块相关

            ModuleManager* ModuleManager();
        };
    }
}