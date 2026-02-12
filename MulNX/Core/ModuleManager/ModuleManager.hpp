#pragma once

#include "../ModuleBase/ModuleBase.hpp"

#include <map>

class ModuleInfo {
public:
	std::vector<std::pair<std::string, MulNXHandle>> Info;
};

namespace MulNX {
    class ModulePack {
        friend class Core::ModuleManager;
        class ModuleBuffer {
        public:
            std::unique_ptr<ModuleBase> Module;
            std::string Name;
            int Priorty;

            ModuleBuffer() = delete;
            ModuleBuffer(std::unique_ptr<ModuleBase> Module, std::string&& Name, int Priorty) {
                this->Module = std::move(Module);
                this->Name = std::move(Name);
                this->Priorty = Priorty;
            }
        };
        std::vector<ModuleBuffer>Modules;
    public:
        template<typename T>
        requires std::derived_from<T, ModuleBase>
        ModulePack& CreateBack(std::string&& Name, int Priority = 0) {
            std::unique_ptr<T>Module = std::make_unique<T>();
            this->Modules.push_back(ModuleBuffer(
                std::move(Module), std::move(Name), Priority
            ));
            return *this;
        }
    };
    namespace Core {
        // 模块管理器类，负责加载、卸载和管理各个模块
		class ModuleManager final :public MulNX::ModuleBase {
		private:
			// 存储初始化优先级与模块句柄的映射
			std::map<int, MulNXHandle>InitPriority;
			
			std::atomic<size_t> Index = 0;
			// 存储已加载模块的映射
			std::unordered_map<MulNXHandle, std::unique_ptr<MulNX::ModuleBase>> Modules;
			// 存储从字符串到模块句柄的映射，便于按名称查找
			std::unordered_map<std::string, MulNXHandle> NameToHandleMap;
		public:
			bool Init()override;
			void ProcessMsg(MulNX::Message* Msg)override;
			void VirtualMain()override;
			void Windows()override;

			// 注册模块，需要传入模块指针和名称，以及优先级，从1开始，数字越小优先级越高
            bool RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module, std::string&& Name, int Priority = 0);
            // 注册模块，通过模块包
            bool RegisteModules(ModulePack&& ModulePack);
            // 根据名称获取模块指针
			MulNX::ModuleBase* FindModule(const std::string& Name);
			// 按类型查找模块
			template<typename T>
			T* FindModule(const std::string& Name) {
				return dynamic_cast<T*>(this->FindModule(Name));
			}

			// 初始化最后部分使用
			bool PackedInit();
			// 主循环调用所有模块的VirtualMain
			void PackedVirtualMain();
			// 对窗口入口的包装
			void PackedWindows();
		};
	}

}