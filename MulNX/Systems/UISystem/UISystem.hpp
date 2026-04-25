#pragma once

#include "IUISystem.hpp"

namespace MulNX {
    class UISystem final :public IUISystem {
    private:
        std::shared_mutex UIMutex{};
        MulNX::UIContext UIContext{};
        bool UISystemRunning = false;
        std::function<void(void)>FrameBefore = nullptr;
        std::function<void(void)>FrameBehind = nullptr;
        std::string strImguiIniPath;
    public:
        bool Init()override;

        void ProcessMsg(MulNX::Message& Msg)override;

        std::shared_mutex& GetMutex()override { return this->UIMutex; }

        int Render()override;

        MulNX::UIContext* GetUIContext() { return &this->UIContext; }

        void SetFrameBefore(std::function<void(void)>Before)override;
        void SetFrameBehind(std::function<void(void)>Behind)override;
    };
}