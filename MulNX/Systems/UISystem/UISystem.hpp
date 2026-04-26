#pragma once

#include "UIContext/UIContext.hpp"
#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXExtensions/WinExt/WIN32Msg/WIN32Msg.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class UISystem final :public MulNX::ModuleBase {
    private:
        std::shared_mutex UIMutex{};
        MulNX::UIContext UIContext{};
        bool UISystemRunning = false;
        std::function<void(void)>FrameBefore = nullptr;
        std::function<void(void)>FrameBehind = nullptr;
        std::string strImguiIniPath;
        
    public:
        std::atomic<bool>WantCaptureMouse{ false };
        std::atomic<bool>WantTextInput{ false };
        moodycamel::ConcurrentQueue<MulNX::Win32::Msg4>winMsgs{};
        bool Init()override;

        void ProcessMsg(MulNX::Message& Msg)override;

        std::shared_mutex& GetMutex() { return this->UIMutex; }

        int Render();

        MulNX::UIContext* GetUIContext() { return &this->UIContext; }

        void SetFrameBefore(std::function<void(void)>Before);
        void SetFrameBehind(std::function<void(void)>Behind);
    };
}