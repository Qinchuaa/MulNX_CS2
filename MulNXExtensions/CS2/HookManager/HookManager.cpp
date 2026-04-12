#include "HookManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#include <chrono>
#pragma comment(lib,"d3d11.lib")

using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::IUISystem>("UISystem");
    return true;
}
void HookManager::StartAll() {
    this->CheckHook();// 手动执行一次Hook
}

void HookManager::CheckHook() {
    // 重置状态
    this->d3dInited = false;

    this->CreateHook();

    //发送重新Hook消息
    MulNX::Message Msg("Core/ReHook"_hash);
    this->ISys().PublishAsync(std::move(Msg));
}

DWORD HookManager::CreateHook() {
	this->pSwapChain = nullptr;
	this->pd3dDevice = nullptr;
	while (true) {
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
			&this->pSwapChain,
			&this->pd3dDevice,
			nullptr,
			nullptr);

        if (this->pSwapChain) {
            this->hkPresent = MulNX::Memory::HookEx::Create((uint8_t*)IVClass::Assume(this->pSwapChain)->GetVFuncPtr(8), 0, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
                if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                    this->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                    this->pUISystem->Render();
                }
                return true;
                }).value();
            this->hkPresent->Attach();

            this->hkResizeBuffers = MulNX::Memory::HookEx::Create((uint8_t*)IVClass::Assume(this->pSwapChain)->GetVFuncPtr(13), 0, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
                this->ReleaseOld();
                return true;
                }).value();
            this->hkResizeBuffers->Attach();

			this->pd3dDevice->Release();
			this->pSwapChain->Release();

			this->pUISystem->SetFrameBefore([this]()->void {

                this->d3dInit(this->pSwapChain);
                this->BuildNew();
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

            break;
        }
	}
	return 0;
}
void HookManager::d3dInit(IDXGISwapChain* _this) {
	if (!this->d3dInited) {
		_this->GetDevice(__uuidof(ID3D11Device), (void**)&this->pd3dDevice);
		this->pd3dDevice->GetImmediateContext(&this->pd3dContext);

		DXGI_SWAP_CHAIN_DESC sd;
		_this->GetDesc(&sd);
        this->CS2hWnd = sd.OutputWindow;
        // hook窗口过程
        this->hkWndProc = MulNX::Memory::HookEx::Create((uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC), 0, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
            ctx->rax = this->MyWndProc((HWND)ctx->rcx, ctx->rdx, ctx->r8, ctx->r9);
            return ctx->rax;
            }).value();
        this->hkWndProc->Attach();

		ID3D11Texture2D* buf = nullptr;
		_this->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
		if (buf == nullptr)return;
		this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
		buf->Release();

		if (!this->ImGuiInited) {
			ImGui::CreateContext();
			//设置ini文件路径
			ImGuiIO& io = ImGui::GetIO();

			ImGui_ImplWin32_Init(this->CS2hWnd);
			ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dContext);

			ImFont* font = io.Fonts->AddFontFromFileTTF(
				"C:/Windows/Fonts/msyh.ttc",				// 微软雅黑字体路径
				16.0f,										// 字体大小
				nullptr,									// 使用默认配置
				io.Fonts->GetGlyphRangesChineseFull()		// 加载所有中文字符
			);
			ImGui_ImplDX11_CreateDeviceObjects();

			// 转换为GBK（供ImGui使用）

			this->imguiIniPath = this->ISys().PathGet("Config") / "MulNXUIConfig.ini";
			this->imguiIniPathString = MulNX::Base::CharUtility::FilePathToString(this->imguiIniPath);
			io.IniFilename = this->imguiIniPathString.c_str();

			this->ImGuiInited = true;
		}


		this->d3dInited = true;
	}
}

void HookManager::ReleaseOld() {
    ImGui_ImplDX11_InvalidateDeviceObjects();
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
LRESULT __stdcall HookManager::MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	std::unique_lock lock(this->pUISystem->UIMtx);
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
		return 0;
	}

	ImGuiIO& io = ImGui::GetIO();
	// 鼠标：当ImGui想要捕获时总是拦截
	if (io.WantCaptureMouse && MulNX::Base::WIN32Msg::IsMouseMessage(uMsg)) {
		return 0;
	}
	// 键盘：只在WantTextInput为true时拦截（表示输入框激活）
	else if (io.WantTextInput && MulNX::Base::WIN32Msg::IsKeyboardMessage(uMsg)) {
		return 0;
	}
	return 1;
}