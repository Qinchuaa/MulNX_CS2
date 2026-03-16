#pragma once

#include "MulNXUIContext/MulNXUIContext.hpp"
#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

namespace MulNX {
	class IUISystem :public ModuleBase {
	public:
		std::recursive_mutex UIMtx;

		virtual std::shared_mutex& GetMutex() = 0;

		virtual int Render() = 0;

        virtual MulNXUIContext* GetUIContext() = 0;

		virtual void SetFrameBefore(std::function<void(void)>Before) = 0;
		virtual void SetFrameBehind(std::function<void(void)>Behind) = 0;
	};
}