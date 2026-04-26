#include "HookManager.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <chrono>

// 链接d3d11库
#pragma comment(lib,"d3d11.lib")

using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");
    return true;
}
void HookManager::ActiveSystem() {
    this->CreateHook();
}
void HookManager::CreateHook() {
    // 准备临时的D3D11设备和交换链，以获取函数地址
    ID3D11Device* pTempD3DDevice = nullptr;
    IDXGISwapChain* pTempSwapChain = nullptr;
    // 尝试创建D3D11设备和交换链
    const unsigned level_count = 2;
    D3D_FEATURE_LEVEL levels[level_count] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    HRESULT hRusult = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        levels,
        level_count,
        D3D11_SDK_VERSION,
        &sd,
        &pTempSwapChain,
        &pTempD3DDevice,
        nullptr,
        nullptr);
    // 检查是否成功创建设备和交换链
    if (FAILED(hRusult)) {
        MulNX::ErrorTerminate("无法创建D3D11设备和交换链，错误代码: " + std::to_string(hRusult));
    }
    // Hook Present函数
    // 函数开头：
    // 0~4：Steam钩子（OBS游戏捕获钩子会与其交互，进行画面捕获）
    // 5~9：在这里部署MulNX的钩子，注意此时OBS捕获已经完成，可以做到启动顺序无关的渲染分离
    // 10+：其它汇编指令，我们的MulNX钩子最终跳转继续执行
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(8) + 5,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk) {
            if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                // 从这里偷取游戏真正的交换链指针
                this->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                this->d3dInit();
                this->BuildNew();
                this->pUISystem->Render();
            }
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPresent->Attach();
    this->ISys().LogSucc("Present钩子已部署");

    // Hook ResizeBuffers函数
    this->hkResizeBuffers = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(13),
        0, false, [this](RegContext* ctx, MulNX::Hook* hk) {
            this->ReleaseOld();
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkResizeBuffers->Attach();
    this->ISys().LogSucc("ResizeBuffers钩子已部署");

    // 释放临时设备和交换链
    pTempD3DDevice->Release();
    pTempSwapChain->Release();

    this->pUISystem->SetFrameBefore([this]()->void {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        return;
        });
    this->pUISystem->SetFrameBehind([this]()->void {
        ImGui::EndFrame();
        ImGui::Render();
        this->pd3dContext->OMSetRenderTargets(1, &this->view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        return;
        });
}

void HookManager::d3dInit() {
    if (this->d3dInited)return;
    this->d3dInited = true;
    // 定位到真正的交换链指针后，获取设备和上下文
    this->pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&this->pd3dDevice);
    this->pd3dDevice->GetImmediateContext(&this->pd3dContext);
    // 通过设备描述获取窗口句柄
    DXGI_SWAP_CHAIN_DESC sd;
    this->pSwapChain->GetDesc(&sd);
    this->CS2hWnd = sd.OutputWindow;
    // 以此为基础，Hook窗口过程，以便处理输入等消息
    this->hkWndProc = MulNX::Hook::Create((uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC),
        0, false, [this](RegContext* ctx, MulNX::Hook* hk) {
            return this->MyWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
        }).value();
    this->hkWndProc->Attach();
    this->ISys().LogSucc("窗口过程钩子已部署");
    // 创建渲染目标视图
    ID3D11Texture2D* buf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
    this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
    buf->Release();
    // 初始化ImGui上下文
    ImGui::CreateContext();
    // 初始化ImGui平台和渲染器绑定
    ImGui_ImplWin32_Init(this->CS2hWnd);
    ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dContext);
}

void HookManager::ReleaseOld() {
    if (this->view) {
        this->view->Release();
        this->view = nullptr;
    }
    this->needReBuild.store(true, std::memory_order_release);
}
void HookManager::BuildNew() {
    if (this->needReBuild.load(std::memory_order_acquire)) {
        ID3D11Texture2D* buf = nullptr;
        this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
        if (buf == nullptr)return;
        this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
        buf->Release();
        this->needReBuild.store(false, std::memory_order_release);
    }
}
MulNX::Hook::Then HookManager::MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        this->ISys().LogWarning(I18n("sys.shutdown_warning"));
    }
    this->pUISystem->winMsgs.enqueue({ hwnd,uMsg,wParam,lParam });
    if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire) && MulNX::Win32::IsMouseMessage(uMsg)) {
        return MulNX::Hook::Then::Return;
    }
    else if (this->pUISystem->WantTextInput.load(std::memory_order_acquire) && MulNX::Win32::IsKeyboardMessage(uMsg)) {
        return MulNX::Hook::Then::Return;
    }
    return MulNX::Hook::Then::Continue;
}