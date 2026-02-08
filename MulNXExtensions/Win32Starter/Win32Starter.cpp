#include "Win32Starter.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>

#include <d3d11.h>
#include <tchar.h>
#pragma comment(lib,"d3d11.lib")

bool Win32Starter::Init() {
    this->pInstance = this;
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, this->WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    this->hWnd = ::CreateWindowW(wc.lpszClassName, L"齐鲁工业大学RM自定义客户端", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(this->hWnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(this->hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(this->hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    //style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(this->hWnd);
    ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    // 加载字体
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\msyh.ttc",         // 字体文件路径
        18.0f,                                  // 字体大小
        nullptr,                                // 字体配置（可选）
        io.Fonts->GetGlyphRangesChineseFull()   // 支持中文
    );
    if (myFont == nullptr) {
        throw 1;
    }

    // 设置为默认字体
    io.FontDefault = myFont;

    // 重建字体纹理（必须在加载字体后调用）
    io.Fonts->Build();
    //IM_ASSERT(font != nullptr);

    this->done = false;

    this->Core->IUISystem().SetFrameBefore([this]() {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                this->done = true;
        }
        if (this->done)
            return;

        // Handle window being minimized or screen locked
        if (this->SwapChainOccluded && this->pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            return;
        }
        this->SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (this->ResizeWidth != 0 && this->ResizeHeight != 0) {
            CleanupRenderTarget();
            this->pSwapChain->ResizeBuffers(0, this->ResizeWidth, this->ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            this->ResizeWidth = this->ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        });

    this->Core->IUISystem().SetFrameBehind([this]() {
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            this->clear_color.x * this->clear_color.w,
            this->clear_color.y * this->clear_color.w,
            this->clear_color.z * this->clear_color.w,
            this->clear_color.w
        };
        this->pd3dContext->OMSetRenderTargets(1, &this->mainRenderTargetView, nullptr);
        this->pd3dContext->ClearRenderTargetView(this->mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        //HRESULT hr = this->pSwapChain->Present(1, 0);   // Present with vsync
        HRESULT hr = this->pSwapChain->Present(0, 0); // Present without vsync
        this->SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        });

    this->IDebugger->SetShowFunc([](MulNX::Debugger* This)->void {
        ImGui::Begin("调试器");
        // 在标签页内创建一个子窗口
        ImVec2 childSize = ImGui::GetContentRegionAvail();
        childSize.y -= ImGui::GetStyle().ItemSpacing.y; // 留出一点空间

        // 开始子窗口，占据标签页的剩余空间
        ImGui::BeginChild("信息", childSize, true, ImGuiWindowFlags_HorizontalScrollbar);

        // 使用虚拟列表优化性能
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(This->DebugMsg.size()));
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto& msg = This->DebugMsg[i];

                // 根据消息类型着色
                if (msg.find(This->Info) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 50, 255, 255));
                }
                else if (msg.find(This->Succ) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 100, 0, 255));
                }
                else if (msg.find(This->Warning) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 0, 255));
                }
                else if (msg.find(This->Error) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
                }

                ImGui::TextUnformatted(msg.c_str());

                //弹出
                ImGui::PopStyleColor();
            }
        }

        // 自动滚动到最新消息
        if (This->NeedAutoScroll) {
            ImGui::SetScrollHereY(1.0f);
            This->NeedAutoScroll = false;
        }

        // 结束子窗口
        ImGui::EndChild();
        ImGui::End();
        });

    return true;
}

// Helper functions

bool Win32Starter::CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &this->pSwapChain, &this->pd3dDevice, &featureLevel, &this->pd3dContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &this->pSwapChain, &this->pd3dDevice, &featureLevel, &this->pd3dContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void Win32Starter::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (this->pSwapChain) { this->pSwapChain->Release(); this->pSwapChain = nullptr; }
    if (this->pd3dContext) { this->pd3dContext->Release(); this->pd3dContext = nullptr; }
    if (this->pd3dDevice) { this->pd3dDevice->Release(); this->pd3dDevice = nullptr; }
}

void Win32Starter::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    this->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    this->pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &this->mainRenderTargetView);
    pBackBuffer->Release();
}

void Win32Starter::CleanupRenderTarget() {
    if (this->mainRenderTargetView) { this->mainRenderTargetView->Release(); this->mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI Win32Starter::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Win32Starter* pThis = Win32Starter::pInstance;
    std::unique_lock lock(pThis->Core->IUISystem().UIMtx);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        pThis->ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        pThis->ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
