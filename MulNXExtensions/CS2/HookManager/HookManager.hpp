#pragma once

#include <MulNX/MulNX.hpp>
#include <d3d11.h>
#include <functional>

#include <MulNXExtensions/WinExt/WinExt.hpp>

class HookManager final :public MulNX::Core::CoreStarterBase {
private:
    MulNX::IUISystem* pUISystem = nullptr;

    // Present函数Hook
    std::unique_ptr<MulNX::Memory::HookEx> hkPresent = nullptr;
    // ResizeBuffers函数Hook
    std::unique_ptr<MulNX::Memory::HookEx> hkResizeBuffers = nullptr;
    // 窗口过程Hook
    std::unique_ptr<MulNX::Memory::HookEx> hkWndProc = nullptr;
    LRESULT __stdcall MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // D3D11指针组
public:
    ID3D11Device* pd3dDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pd3dContext = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool d3dInited = false;
    HWND CS2hWnd = nullptr;//CS2窗口句柄

    bool ImGuiInited = false;
    std::filesystem::path imguiIniPath;
    std::string imguiIniPathString;
private:
    void d3dInit(IDXGISwapChain* _this);
public:
    bool Init()override;
    void StartAll()override;
    void CheckHook();

    void ReleaseOld();
    std::atomic<bool> needReBuild = false;
    void BuildNew();

    DWORD CreateHook();//创建Hook   
};