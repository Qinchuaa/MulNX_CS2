#include "HookManager.hpp"

#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <chrono>

// 链接d3d11库
#pragma comment(lib,"d3d11.lib")

using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::IUISystem>("UISystem");
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
    this->hkPresent = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(8),
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                // 这里是Hook Present
                // 我们从这里，偷到了游戏的真正的交换链指针
                this->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                this->d3dInit();
                this->BuildNew();
                this->pUISystem->Render();
            }
            return true;
        }).value();
    this->hkPresent->Attach();
    this->ISys().LogSucc("Present钩子已部署");

    // Hook ResizeBuffers函数
    this->hkResizeBuffers = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(13),
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            this->ReleaseOld();
            return true;
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
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            bool CallRawFunc = this->MyWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
            return CallRawFunc;
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

    // 设置ini文件路径
    ImGuiIO& io = ImGui::GetIO();
    auto IniPath = this->ISys().PathGet("Config") / "MulNXUIConfig.ini";
    // 这里需要进行转换，以适配ImGui的接口
    this->ImguiIniPathString = MulNX::Base::CharUtility::FilePathToString(IniPath);
    io.IniFilename = this->ImguiIniPathString.c_str();

    // 加载中文字体
    io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/msyh.ttc",				// 微软雅黑字体路径
        16.0f,										// 字体大小
        nullptr									// 使用默认配置
    );

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

// ImGui窗口处理函数导入
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool HookManager::MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        this->ISys().LogWarning(I18n("sys.shutdown_warning"));
    }
    std::unique_lock lock(this->pUISystem->UIMtx);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    // 鼠标：当ImGui想要捕获时总是拦截
    if (io.WantCaptureMouse && MulNX::Base::WIN32Msg::IsMouseMessage(uMsg)) {
        return false;
    }
    // 键盘：只在WantTextInput为true时拦截（表示输入框激活）
    else if (io.WantTextInput && MulNX::Base::WIN32Msg::IsKeyboardMessage(uMsg)) {
        return false;
    }
    return true;
}