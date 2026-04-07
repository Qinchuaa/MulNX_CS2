#pragma once
// 整个系统的核心，负责所有子模块的生命周期管理，初始化，主循环调用等
// 同时提供各子模块的接口访问
#include <MulNX/Config/Config.hpp>
#include <Memory>

namespace MulNX {
    namespace Core {
        class Core {
            friend class MulNX::ModuleBase;
        private:
            // 数据存储：

            // 自身指针
            std::unique_ptr<Core> pMyself = nullptr;
            // 核心名称（文件系统路径管理需要用）
            std::string CoreName;
            // 模块管理器指针
            std::unique_ptr<ModuleManager> pModuleManager;
			// 核心启动器指针
            std::unique_ptr<MulNX::Core::CoreStarterBase> pCoreStarter = nullptr;

            Core() = delete;
            // 删除拷贝构造
            Core(const Core&) = delete;
            // 删除拷贝赋值
            Core& operator=(const Core&) = delete;
        public:
            // 构造函数
            Core(std::string&& CoreName);
            // 析构函数
            ~Core() = default;
            
            // 只允许被调用一次
            static Core* Create(std::string&& CoreName);

            // 初始化
            void Init();
            // 主循环
            void VirtualMain();

            // 获取模块的接口
            ModuleManager* ModuleManager();

            // 获取启动器
            MulNX::Core::CoreStarterBase* GetStarter() { return this->pCoreStarter.get(); }

            // 设置启动器
			bool SetCoreStarter(std::unique_ptr<CoreStarterBase> Starter);

            // 获取核心名
            std::string GetName();
        };
    }
}