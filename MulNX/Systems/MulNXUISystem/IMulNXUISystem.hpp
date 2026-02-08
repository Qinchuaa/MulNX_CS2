#pragma once

#include "MulNXUIContext/MulNXUIContext.hpp"

#include "../../Core/ModuleBase/ModuleBase.hpp"

#include <d3d11.h>

namespace MulNX {
	class IUISystem :public ModuleBase {
	public:
		std::recursive_mutex UIMtx;

		IUISystem() : ModuleBase() {
			//this->Type = ModuleType::MulNXUISystem;
		}

		virtual std::shared_mutex& GetMutex() = 0;

		virtual int Render() = 0;

		virtual MulNXSingleUIContext* GetSingleContext(const MulNXHandle& hContext) = 0;

		virtual void SetFrameBefore(std::function<void(void)>Before) = 0;
		virtual void SetFrameBehind(std::function<void(void)>Behind) = 0;
	};
}