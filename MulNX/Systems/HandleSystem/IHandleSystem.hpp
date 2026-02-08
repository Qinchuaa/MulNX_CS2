#pragma once

#include "../../Core/ModuleBase/ModuleBase.hpp"

namespace MulNX {
	class IHandleSystem :public ModuleBase {
	public:
		IHandleSystem() : ModuleBase() {
			//this->Type = ModuleType::HandleSystem;
		}
		// 注册句柄，传入任意类型的资源，返回对应的句柄
		virtual MulNXHandle RegisteUnique(MulNX::Base::any_unique_ptr Resource) = 0;
		// 释放句柄，返回对应的资源所有权
		virtual MulNX::Base::any_unique_ptr ReleaseUnique(MulNXHandle Handle) = 0;
		// 注册共享句柄，传入任意类型的资源，返回对应的句柄
		virtual MulNXHandle RegisteShared(MulNX::Base::any_shared_ptr Resource) = 0;
		// 获取共享句柄，返回对应的资源共享指针
		virtual MulNX::Base::any_shared_ptr GetShared(MulNXHandle Handle) = 0;
	};
}