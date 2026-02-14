#pragma once

#include "../ModuleBase/ModuleBase.hpp"

#include <map>

class ModuleInfo {
public:
	std::vector<std::pair<std::string, MulNXHandle>> Info;
};

namespace MulNX {
    namespace Core {
        // 模块管理器类，负责加载、卸载和管理各个模块
		class ModuleManager final :public MulNX::ModuleBase {
        private:
            // 存储所有的模块
            std::unordered_map<MulNXHandle, std::unique_ptr<MulNX::ModuleBase>> Modules;

            // 存储
            std::map<int, MulNXHandle>PriorityToHandleMap;
			// 存储从字符串到模块句柄的映射，便于按名称查找
			std::unordered_map<std::string, MulNXHandle> NameToHandleMap;
		public:
			bool Init()override;
			void ProcessMsg(MulNX::Message* Msg)override;
			void VirtualMain()override;
			void Windows()override;

			// 注册模块，需要传入模块指针和名称，以及优先级，从1开始，数字越小优先级越高
            bool RegisteModule(std::unique_ptr<MulNX::ModuleBase>&& Module, std::string&& Name, int Priority = 0);
            // 创建模块
            template<typename T>
            requires std::derived_from<T, ModuleBase>
            ModuleManager& CreateModule(std::string&& Name, int Priority = 0) {
                std::unique_ptr<T>Module = std::make_unique<T>();
                this->RegisteModule(std::move(Module), std::move(Name), Priority);
                return *this;
            }
            
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