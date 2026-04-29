#include "HookManager.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <d3dcompiler.h>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

// ---------- 着色器源码 ----------
// 全屏三角形顶点着色器
static const char* g_GreenVS = R"(
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(uint id : SV_VertexID) {
    VSOut o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0.0f, 1.0f);
    return o;
}
)";

// 像素着色器：深度阈值抠像
static const char* g_GreenPS = R"(
Texture2D<float>  DepthTex   : register(t0);
Texture2D<float4> ColorTex   : register(t1);
SamplerState      PointSampler : register(s0);

cbuffer ParamCB : register(b0) {
    float g_Threshold;
    float3 pad;
};

float4 main(float4 pos : SV_POSITION) : SV_Target {
    float depth = DepthTex.Load(int3(pos.xy, 0));
    float4 color = ColorTex.Load(int3(pos.xy, 0));
    if (depth < g_Threshold) {
        return color;                   // 前景：原色
    } else {
        return float4(0.0, 1.0, 0.0, 1.0); // 背景：纯绿
    }
}
)";

// ---------- 辅助：编译着色器 ----------
static ID3DBlob* CompileShader(const char* src, const char* target, const char* entry) {
    ID3DBlob* blob = nullptr, * errorBlob = nullptr;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
        entry, target, 0, 0, &blob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return nullptr;
    }
    if (errorBlob) errorBlob->Release();
    return blob;
}

// ------------------------------------------------------------------

bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::UISystem>("UISystem");
    return true;
}

void HookManager::ActiveSystem() {
    // 临时 D3D11 设备/交换链
    ID3D11Device* pTempD3DDevice = nullptr;
    IDXGISwapChain* pTempSwapChain = nullptr;
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
    HRESULT hResult = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, level_count, D3D11_SDK_VERSION, &sd,
        &pTempSwapChain, &pTempD3DDevice, nullptr, nullptr);
    if (FAILED(hResult))
        MulNX::ErrorTerminate("无法创建D3D11设备和交换链，错误代码: " + std::to_string(hResult));

    ID3D11DeviceContext* pTempContext = nullptr;
    pTempD3DDevice->GetImmediateContext(&pTempContext);

    // ---- Hook ClearDepthStencilView (vtable index 53) ----
    this->hkClearDepthStencilView = MulNX::Hook::Create(
        (uint8_t*)IVClass::Assume(pTempContext)->GetVFuncPtr(53),
        0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            ID3D11DeviceContext* pCtx = (ID3D11DeviceContext*)ctx->rcx;
            ID3D11DepthStencilView* pDSV = (ID3D11DepthStencilView*)ctx->rdx;
            UINT ClearFlags = (UINT)ctx->r8;

            if (pDSV && (UINT_PTR)pDSV > 0x10000 && (ClearFlags & D3D11_CLEAR_DEPTH)) {
                // 只拷贝，不创建新资源
                if (this->d3dInited && m_pDepthCopyTex) {
                    ID3D11Resource* pSrcRes = nullptr;
                    pDSV->GetResource(&pSrcRes);
                    if (pSrcRes) {
                        pCtx->CopyResource(m_pDepthCopyTex, pSrcRes);
                        pSrcRes->Release();
                    }
                }
            }
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkClearDepthStencilView->Attach();
    this->ISys().LogSucc("ClearDepthStencilView钩子已部署");

    pTempContext->Release();

    // ---- Hook Present (offset +5) ----
    this->hkPresent = MulNX::Hook::Create(
        (uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(8) + 5,
        0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                this->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                this->d3dInit();
                this->BuildNew();

                // 确保颜色/深度副本存在，并拷贝当前帧颜色
                EnsureCopyResources();
                CopyColorBuffer();

                // UI 系统会依次调用 FrameBefore → 处理控件 → FrameBehind
                this->pUISystem->Render();
            }
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPresent->Attach();
    this->ISys().LogSucc("Present钩子已部署");

    // ---- Hook ResizeBuffers (vtable index 13) ----
    this->hkResizeBuffers = MulNX::Hook::Create(
        (uint8_t*)IVClass::Assume(pTempSwapChain)->GetVFuncPtr(13),
        0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            this->ReleaseOld();
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkResizeBuffers->Attach();
    this->ISys().LogSucc("ResizeBuffers钩子已部署");

    pTempD3DDevice->Release();
    pTempSwapChain->Release();

    // ---- 重写 UI 回调，在 ImGui 绘制之前插入绿幕渲染 ----
    this->pUISystem->FrameBefore = [this]() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        };
    this->pUISystem->FrameBehind = [this]() {
        // 1. 绿幕全屏绘制（覆盖原画面，背景变绿）
        RenderGreenScreen();

        // 2. 可选的 ImGui 控制面板
        ImGui::Begin("GreenScreen Settings");
        ImGui::SliderFloat("Threshold", &m_GreenThreshold, 0.0f, 1.0f);
        ImGui::End();

        // 3. 渲染 ImGui (UI 叠在最上层)
        ImGui::EndFrame();
        ImGui::Render();
        this->pd3dContext->OMSetRenderTargets(1, &this->view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        };
}

void HookManager::d3dInit() {
    if (this->d3dInited) return;
    this->d3dInited = true;

    this->pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&this->pd3dDevice);
    this->pd3dDevice->GetImmediateContext(&this->pd3dContext);

    DXGI_SWAP_CHAIN_DESC sd;
    this->pSwapChain->GetDesc(&sd);
    this->CS2hWnd = sd.OutputWindow;

    // 窗口过程钩子
    this->hkWndProc = MulNX::Hook::Create(
        (uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC),
        0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            return this->MyWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
        }).value();
    this->hkWndProc->Attach();
    this->ISys().LogSucc("窗口过程钩子已部署");

    // 交换链 RTV
    ID3D11Texture2D* buf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
    this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
    buf->Release();

    // ImGui 初始化
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(this->CS2hWnd);
    ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dContext);

    // 创建绿幕着色器资源
    CreateGreenScreenAssets();
}

void HookManager::ReleaseOld() {
    if (this->view) { this->view->Release(); this->view = nullptr; }
    if (m_pDepthSRV) { m_pDepthSRV->Release(); m_pDepthSRV = nullptr; }
    if (m_pDepthCopyTex) { m_pDepthCopyTex->Release(); m_pDepthCopyTex = nullptr; }
    if (m_pColorCopySRV) { m_pColorCopySRV->Release(); m_pColorCopySRV = nullptr; }
    if (m_pColorCopyTex) { m_pColorCopyTex->Release(); m_pColorCopyTex = nullptr; }
    this->needReBuild.store(true, std::memory_order_release);
}

void HookManager::BuildNew() {
    if (!this->needReBuild.load(std::memory_order_acquire)) return;
    ID3D11Texture2D* buf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
    if (!buf) return;
    this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
    buf->Release();
    this->needReBuild.store(false, std::memory_order_release);
}

MulNX::Hook::Then HookManager::MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE)
        this->ISys().LogWarning(I18n("sys.shutdown_warning"));
    this->pUISystem->winMsgs.enqueue({ hwnd, uMsg, wParam, lParam });
    if (this->pUISystem->WantCaptureMouse.load(std::memory_order_acquire) && MulNX::Win32::IsMouseMessage(uMsg))
        return MulNX::Hook::Then::Return;
    if (this->pUISystem->WantTextInput.load(std::memory_order_acquire) && MulNX::Win32::IsKeyboardMessage(uMsg))
        return MulNX::Hook::Then::Return;
    return MulNX::Hook::Then::Continue;
}

// ------------------------------------------------------------------
// 创建/重建颜色 + 深度副本（在 Present 安全点调用）
// ------------------------------------------------------------------
void HookManager::EnsureCopyResources() {
    if (!this->pd3dDevice || !this->pd3dContext) return;

    // ---------- 颜色副本 ----------
    ID3D11Texture2D* backBuffer = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (backBuffer) {
        D3D11_TEXTURE2D_DESC bbDesc;
        backBuffer->GetDesc(&bbDesc);

        bool colorNeedCreate = (!m_pColorCopyTex || !m_pColorCopySRV ||
            m_ColorWidth != bbDesc.Width ||
            m_ColorHeight != bbDesc.Height ||
            m_ColorCopyFormat != bbDesc.Format);

        if (colorNeedCreate) {
            if (m_pColorCopySRV) { m_pColorCopySRV->Release(); m_pColorCopySRV = nullptr; }
            if (m_pColorCopyTex) { m_pColorCopyTex->Release(); m_pColorCopyTex = nullptr; }

            D3D11_TEXTURE2D_DESC copyDesc = {};
            copyDesc.Width = bbDesc.Width;
            copyDesc.Height = bbDesc.Height;
            copyDesc.MipLevels = 1;
            copyDesc.ArraySize = 1;
            copyDesc.Format = bbDesc.Format;
            copyDesc.SampleDesc.Count = 1;
            copyDesc.Usage = D3D11_USAGE_DEFAULT;
            copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (bbDesc.SampleDesc.Count > 1)
                copyDesc.BindFlags |= D3D11_BIND_RENDER_TARGET; // 支持 MSAA Resolve

            HRESULT hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &m_pColorCopyTex);
            if (SUCCEEDED(hr)) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = bbDesc.Format;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                hr = pd3dDevice->CreateShaderResourceView(m_pColorCopyTex, &srvDesc, &m_pColorCopySRV);
                if (SUCCEEDED(hr)) {
                    m_ColorWidth = bbDesc.Width;
                    m_ColorHeight = bbDesc.Height;
                    m_ColorCopyFormat = bbDesc.Format;
                    this->ISys().LogSucc("颜色副本创建成功");
                }
            }
        }
        backBuffer->Release();
    }

    // ---------- 深度副本 ----------
    ID3D11DepthStencilView* curDSV = nullptr;
    this->pd3dContext->OMGetRenderTargets(0, nullptr, &curDSV);
    if (curDSV) {
        ID3D11Texture2D* depthTex = nullptr;
        curDSV->GetResource(reinterpret_cast<ID3D11Resource**>(&depthTex));
        if (depthTex) {
            D3D11_TEXTURE2D_DESC ddesc;
            depthTex->GetDesc(&ddesc);
            depthTex->Release();

            bool depthNeedCreate = (!m_pDepthCopyTex || !m_pDepthSRV ||
                m_DepthWidth != ddesc.Width ||
                m_DepthHeight != ddesc.Height ||
                m_DepthCopyFormat != ddesc.Format);
            if (depthNeedCreate) {
                if (m_pDepthSRV) { m_pDepthSRV->Release(); m_pDepthSRV = nullptr; }
                if (m_pDepthCopyTex) { m_pDepthCopyTex->Release(); m_pDepthCopyTex = nullptr; }

                D3D11_TEXTURE2D_DESC copyDesc = {};
                copyDesc.Width = ddesc.Width;
                copyDesc.Height = ddesc.Height;
                copyDesc.MipLevels = 1;
                copyDesc.ArraySize = 1;
                copyDesc.Format = ddesc.Format;           // typeless
                copyDesc.SampleDesc.Count = 1;
                copyDesc.Usage = D3D11_USAGE_DEFAULT;
                copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                HRESULT hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &m_pDepthCopyTex);
                if (SUCCEEDED(hr)) {
                    DXGI_FORMAT srvFmt = DXGI_FORMAT_UNKNOWN;
                    switch (ddesc.Format) {
                    case DXGI_FORMAT_R24G8_TYPELESS: srvFmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
                    case DXGI_FORMAT_R32_TYPELESS:   srvFmt = DXGI_FORMAT_R32_FLOAT;              break;
                    case DXGI_FORMAT_R16_TYPELESS:   srvFmt = DXGI_FORMAT_R16_UNORM;              break;
                    default: srvFmt = ddesc.Format; break;
                    }
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = srvFmt;
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MipLevels = 1;
                    hr = pd3dDevice->CreateShaderResourceView(m_pDepthCopyTex, &srvDesc, &m_pDepthSRV);
                    if (SUCCEEDED(hr)) {
                        m_DepthWidth = ddesc.Width;
                        m_DepthHeight = ddesc.Height;
                        m_DepthCopyFormat = ddesc.Format;
                        this->ISys().LogSucc("深度副本创建成功");
                    }
                }
            }
        }
        curDSV->Release();
    }
}

// ------------------------------------------------------------------
// 拷贝后备缓冲区 → 颜色副本（非 MSAA 直接 CopyResource）
// ------------------------------------------------------------------
void HookManager::CopyColorBuffer() {
    if (!m_pColorCopyTex || !this->pd3dContext) return;

    ID3D11Texture2D* backBuf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    if (!backBuf) return;

    D3D11_TEXTURE2D_DESC bbDesc;
    backBuf->GetDesc(&bbDesc);

    if (bbDesc.SampleDesc.Count > 1) {
        this->pd3dContext->ResolveSubresource(m_pColorCopyTex, 0, backBuf, 0, bbDesc.Format);
    }
    else {
        this->pd3dContext->CopyResource(m_pColorCopyTex, backBuf);
    }
    backBuf->Release();
}

// ------------------------------------------------------------------
// 创建绿幕着色器、常量缓冲区、采样器、混合状态
// ------------------------------------------------------------------
void HookManager::CreateGreenScreenAssets() {
    auto device = this->pd3dDevice;
    if (!device) return;

    ID3DBlob* vsBlob = CompileShader(g_GreenVS, "vs_4_0", "main");
    ID3DBlob* psBlob = CompileShader(g_GreenPS, "ps_4_0", "main");
    if (!vsBlob || !psBlob) {
        this->ISys().LogError("CreateGreenScreenAssets: 着色器编译失败");
        return;
    }

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pGreenVS);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pGreenPS);

    D3D11_INPUT_ELEMENT_DESC dummyDesc[] = {
        {"DUMMY", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    device->CreateInputLayout(dummyDesc, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pGreenLayout);

    vsBlob->Release();
    psBlob->Release();

    // 常量缓冲区（存储阈值）
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = 16;  // float4
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &m_pGreenCB);

    // 点采样器
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sampDesc, &m_pPointSampler);

    // 混合状态（可选，保留原写法）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;       // 此处不启用混合，直接覆盖
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, &m_pBlendState);

    this->ISys().LogSucc("绿幕着色器资源创建成功");
}

// ------------------------------------------------------------------
// 全屏三角形渲染：深度 < 阈值保留原色，否则绿色
// ------------------------------------------------------------------
void HookManager::RenderGreenScreen() {
    if (!this->pd3dContext || !this->view ||
        !m_pDepthSRV || !m_pColorCopySRV ||
        !m_pGreenVS || !m_pGreenPS)
        return;

    auto ctx = this->pd3dContext;

    // 保存状态
    ID3D11RenderTargetView* savedRTV = nullptr;
    ID3D11DepthStencilView* savedDSV = nullptr;
    ctx->OMGetRenderTargets(1, &savedRTV, &savedDSV);

    UINT numVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT savedVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ctx->RSGetViewports(&numVP, savedVP);

    ID3D11DepthStencilState* savedDSS = nullptr;
    UINT savedStencilRef = 0;
    ctx->OMGetDepthStencilState(&savedDSS, &savedStencilRef);

    ID3D11BlendState* savedBlend = nullptr;
    FLOAT savedBlendFactor[4] = { 1,1,1,1 };
    UINT savedMask = 0;
    ctx->OMGetBlendState(&savedBlend, savedBlendFactor, &savedMask);

    // 设置管线
    ctx->IASetInputLayout(m_pGreenLayout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

    ctx->VSSetShader(m_pGreenVS, nullptr, 0);
    ctx->PSSetShader(m_pGreenPS, nullptr, 0);

    ID3D11ShaderResourceView* srvs[2] = { m_pDepthSRV, m_pColorCopySRV };
    ctx->PSSetShaderResources(0, 2, srvs);
    ctx->PSSetSamplers(0, 1, &m_pPointSampler);
    ctx->PSSetConstantBuffers(0, 1, &m_pGreenCB);

    // 更新阈值常量缓冲
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_pGreenCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &m_GreenThreshold, sizeof(float));
        ctx->Unmap(m_pGreenCB, 0);
    }

    ctx->OMSetRenderTargets(1, &this->view, nullptr);
    D3D11_VIEWPORT vp = { 0.f, 0.f, (FLOAT)m_DepthWidth, (FLOAT)m_DepthHeight, 0.f, 1.f };
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(nullptr, 0);
    ctx->OMSetBlendState(m_pBlendState, nullptr, 0xffffffff);

    ctx->Draw(3, 0);   // 全屏三角形

    // 恢复状态
    ctx->OMSetRenderTargets(1, &savedRTV, savedDSV);
    ctx->RSSetViewports(numVP, savedVP);
    ctx->OMSetDepthStencilState(savedDSS, savedStencilRef);
    ctx->OMSetBlendState(savedBlend, savedBlendFactor, savedMask);

    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSRV);
    ctx->VSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);

    if (savedRTV) savedRTV->Release();
    if (savedDSV) savedDSV->Release();
    if (savedDSS) savedDSS->Release();
    if (savedBlend) savedBlend->Release();
}