#pragma once

#include <MulNX/MulNX.hpp>
#include <d3d11.h>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class HookManager final : public MulNX::Core::CoreStarterBase {
private:
    MulNX::UISystem* pUISystem = nullptr;

    // Present 钩子
    std::unique_ptr<MulNX::Hook> hkPresent = nullptr;
    // ResizeBuffers 钩子
    std::unique_ptr<MulNX::Hook> hkResizeBuffers = nullptr;
    // 窗口过程钩子
    std::unique_ptr<MulNX::Hook> hkWndProc = nullptr;
    MulNX::Hook::Then MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // ClearDepthStencilView 钩子（清空前偷深度）
    std::unique_ptr<MulNX::Hook> hkClearDepthStencilView = nullptr;
    void CopyDepthBeforeClear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* pDSV);

    // D3D11 核心指针
    ID3D11Device* pd3dDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pd3dContext = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool d3dInited = false;
    HWND CS2hWnd = nullptr;

    void d3dInit();
    void ReleaseOld();
    std::atomic<bool> needReBuild = false;
    void BuildNew();

    // 深度副本
    ID3D11Texture2D* m_pDepthCopyTex = nullptr;
    ID3D11ShaderResourceView* m_pDepthSRV = nullptr;
    UINT m_DepthWidth = 0;
    UINT m_DepthHeight = 0;
    DXGI_FORMAT m_DepthCopyFormat = DXGI_FORMAT_UNKNOWN;

    // 颜色副本（后备缓冲区拷贝）
    ID3D11Texture2D* m_pColorCopyTex = nullptr;
    ID3D11ShaderResourceView* m_pColorCopySRV = nullptr;
    DXGI_FORMAT m_ColorCopyFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_ColorWidth = 0;
    UINT m_ColorHeight = 0;

    void EnsureCopyResources();   // 创建/重建颜色和深度副本
    void CopyColorBuffer();       // 从后备缓冲区拷贝到颜色副本

    // 绿幕着色器资源
    ID3D11VertexShader* m_pGreenVS = nullptr;
    ID3D11PixelShader* m_pGreenPS = nullptr;
    ID3D11InputLayout* m_pGreenLayout = nullptr;
    ID3D11Buffer* m_pGreenCB = nullptr;
    ID3D11SamplerState* m_pPointSampler = nullptr;
    ID3D11BlendState* m_pBlendState = nullptr;   // 可选，用于半透明混合

    void CreateGreenScreenAssets();
    void RenderGreenScreen();

    // 绿幕阈值
    float m_GreenThreshold = 0.5f;

public:
    bool Init() override;
    void ActiveSystem() override;
};