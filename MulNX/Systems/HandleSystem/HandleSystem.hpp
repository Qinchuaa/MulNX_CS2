#pragma once

#include"IHandleSystem.hpp"

namespace MulNX {
	//句柄系统用于全局提供句柄标识，中转any_unique_ptr资源
	class HandleSystem final :public IHandleSystem {
	private:
		std::shared_mutex MapMutex{};
		std::unordered_map<MulNXHandle, MulNX::Base::any_unique_ptr>UniqueResources{};
		std::unordered_map<MulNXHandle, MulNX::Base::any_shared_ptr>SharedResources{};
	public:
		HandleSystem() :IHandleSystem() {
			//this->Type = ModuleType::HandleSystem;
		}

		bool Init()override;

		// 注册句柄，传入任意类型的资源，返回对应的句柄
		MulNXHandle RegisteUnique(MulNX::Base::any_unique_ptr Resource)override;
		// 释放句柄，返回对应的资源所有权
		MulNX::Base::any_unique_ptr ReleaseUnique(MulNXHandle Handle)override;
		// 注册共享句柄，传入任意类型的资源，返回对应的句柄
		MulNXHandle RegisteShared(MulNX::Base::any_shared_ptr Resource)override;
		// 获取共享句柄，返回对应的资源共享指针
		MulNX::Base::any_shared_ptr GetShared(MulNXHandle Handle)override;
	};
}