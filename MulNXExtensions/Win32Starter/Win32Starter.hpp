#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

#include <d3d11.h>

class Win32Starter final :public MulNX::Core::CoreStarterBase {
    friend MulNX::Core::Core;
public:
    inline static Win32Starter* pInstance = nullptr;
private:
    // D3D11指针组
public:
    ID3D11Device* pd3dDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pd3dContext = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool SwapChainOccluded = false;
    UINT ResizeWidth = 0, ResizeHeight = 0;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;

    HWND hWnd;
    // Main loop
    bool done = false;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Forward declarations of helper functions
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool UIInited = false;
    bool ImGuiInited = false;
    std::filesystem::path imguiIniPath;
    std::string imguiIniPathString;
    bool InitUIStyle() { return true; }
    bool UIStyleInited()const { return this->UIInited; }
private:
    void d3dInit(IDXGISwapChain* _this) {};
public:
    bool Init()override;
    void StartAll()override {};
    void ThreadMain()override {};
    void ProcessMsg(MulNX::Message* Msg)override {};
};