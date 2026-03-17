#pragma once

#include "IMulNXUISystem.hpp"

namespace MulNX {
	class UISystem final :public IUISystem {
	private:
		std::shared_mutex UIMutex{};
		MulNXUIContext UIContext{};
		bool UISystemRunning = false;
		std::function<void(void)>FrameBefore = nullptr;
		std::function<void(void)>FrameBehind = nullptr;
	public:
		bool Init()override;

		void ProcessMsg(MulNX::Message& Msg)override;

		std::shared_mutex& GetMutex()override { return this->UIMutex; }
		
		int Render()override;

		MulNXUIContext* GetUIContext() { return &this->UIContext; }

		void SetFrameBefore(std::function<void(void)>Before)override;
		void SetFrameBehind(std::function<void(void)>Behind)override;
	};
}