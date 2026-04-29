#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>

class HookManager final : public MulNX::Core::CoreStarterBase {
private:
    MulNX::UISystem* pUISystem = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    // Present 钩子
    std::unique_ptr<MulNX::Hook> hkPresent = nullptr;
    // ResizeBuffers 钩子
    std::unique_ptr<MulNX::Hook> hkResizeBuffers = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    MulNX::Hook::Then MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // ClearDepthStencilView 钩子（清空前偷深度）
    std::unique_ptr<MulNX::Hook> hkClearDepthStencilView = nullptr;

    
    bool d3dInited = false;
    HWND CS2hWnd = nullptr;

    void d3dInit();
public:
    bool Init() override;
    void ActiveSystem() override;
};