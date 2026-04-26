#pragma once

#include <MulNX/MulNX.hpp>
#include <d3d11.h>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class HookManager final :public MulNX::Core::CoreStarterBase {
private:
    MulNX::UISystem* pUISystem = nullptr;

    // Present函数Hook
    std::unique_ptr<MulNX::Hook> hkPresent = nullptr;
    // ResizeBuffers函数Hook
    std::unique_ptr<MulNX::Hook> hkResizeBuffers = nullptr;
    // 窗口过程Hook
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    MulNX::Hook::Then MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // D3D11指针组
    ID3D11Device* pd3dDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pd3dContext = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool d3dInited = false;
    HWND CS2hWnd = nullptr;//CS2窗口句柄    
public:
    bool Init()override;
    void ActiveSystem()override;
private:
    void CreateHook();
    void d3dInit();
    void ReleaseOld();
    std::atomic<bool> needReBuild = false;
    void BuildNew();
};