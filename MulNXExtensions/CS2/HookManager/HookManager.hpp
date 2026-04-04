#pragma once

#include <MulNX/MulNX.hpp>
#include <d3d11.h>
#include <functional>

#include <MulNXExtensions/WinExt/WinExt.hpp>

class HookManager final :public MulNX::Core::CoreStarterBase {
    friend MulNX::Core::Core;
    friend class MulNXiCoreImpl;
private:
    MulNX::IUISystem* pUISystem = nullptr;
    std::atomic<bool>GuardPleaseAction = false;

    std::unique_ptr<MulNX::Memory::HookEx> hkPresent = nullptr;
    
    // 窗口处理函数
    std::unique_ptr<MulNX::Memory::HookEx> hkWndProc = nullptr;
    LRESULT __stdcall MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    std::atomic<bool>ReHook = false;
    std::atomic<bool>NeedReHook = false;

    // D3D11指针组
public:
    ID3D11Device* pd3dDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pd3dContext = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool d3dInited = false;
    HWND CS2hWnd = nullptr;//CS2窗口句柄

    bool UIInited = false;
    bool ImGuiInited = false;
    std::filesystem::path imguiIniPath;
    std::string imguiIniPathString;
    bool UIStyleInited()const { return this->UIInited; }
private:
    void d3dInit(IDXGISwapChain* _this);
public:
    bool Init()override;
    void StartAll()override;
    void CheckHook();
    void ThreadMain()override;
    void ProcessMsg(MulNX::Message& Msg)override;

    DWORD CreateHook();//创建Hook   
};