#pragma once

#include"IMulNXUISystem.hpp"

namespace MulNX {
	class UISystem final :public IUISystem {
	private:
		std::shared_mutex UIMutex{};
		MulNXUIContext UIContext{};
		bool UISystemRunning = false;
		std::function<void(void)>FrameBefore = nullptr;
		std::function<void(void)>FrameBehind = nullptr;
	public:
		UISystem() :IUISystem() {
			//this->Type = ModuleType::MulNXUISystem;
		}
		bool Init()override;

		void ProcessMsg(MulNX::Message* Msg)override;

		std::shared_mutex& GetMutex()override { return this->UIMutex; }
		
		int Render()override;

		MulNXSingleUIContext* GetSingleContext(const MulNXHandle& hContext)override {
			return this->UIContext.GetSingleContext(hContext);
		}

		void SetFrameBefore(std::function<void(void)>Before)override;
		void SetFrameBehind(std::function<void(void)>Behind)override;
	};
}