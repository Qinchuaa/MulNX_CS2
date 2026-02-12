#pragma once

#include <MulNX/MulNX.hpp>
#include <d3d11.h>
#include <functional>

#include <MulNXExtensions/WinExt/WinExt.hpp>

class HookManager final :public MulNX::Core::CoreStarterBase {
    friend MulNX::Core::Core;
    friend class MulNXiCoreImpl;
public:
    inline static HookManager* pInstance = nullptr;
private:
    std::atomic<bool>GuardPleaseAction = false;

    // Hook Present
    using Present_t = HRESULT __stdcall(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    MulNX::Base::HookUtility::HookUtility<Present_t>& hkPresent = MulNX::Base::HookUtility::HookUtility<Present_t>::GetInstance();
    HRESULT __stdcall MyPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    // Hook Release
    using Release_t = ULONG __stdcall(IUnknown* pThis);
    MulNX::Base::HookUtility::HookUtility<Release_t>& hkRelease = MulNX::Base::HookUtility::HookUtility<Release_t>::GetInstance();
    ULONG __stdcall MyRelease(IUnknown* pThis);
    // 窗口处理函数处理，这里不适用Hook
    WNDPROC OriginWndProc = nullptr;
    static LRESULT __stdcall EntryMyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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
    void ThreadMain()override;
    void ProcessMsg(MulNX::Message* Msg)override;

    DWORD CreateHook();//创建Hook   
};