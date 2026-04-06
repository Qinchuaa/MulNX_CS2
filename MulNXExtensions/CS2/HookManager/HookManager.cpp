#include "HookManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/imgui_d11/imgui_impl_dx11.h>
#include <MulNXThirdParty/imgui_d11/imgui_impl_win32.h>
#pragma comment(lib,"d3d11.lib")
#include <chrono>

static bool AllowReHook = false;// 允许重hook
bool HookManager::Init() {
    this->pUISystem = this->Core->ModuleManager()->FindModule<MulNX::IUISystem>("UISystem");
    return true;
}
void HookManager::StartAll() {
	this->ReHook = true;
    this->NeedThread(250);
    this->CheckHook();// 手动执行一次Hook
}

void HookManager::CheckHook() {
    this->EntryProcessMsg();

    if (this->GuardPleaseAction) {
        AllowReHook = true;
        this->GuardPleaseAction = false;
        this->ISys().LogInfo("检测到D3D11波动，等待用户手动ReHook");
    }
    if (AllowReHook) {
        if (this->pInputSystem->CheckComboClick(VK_INSERT, 3)) {
            ReHook = true;
            AllowReHook = false;
        }
    }
    if (ReHook) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // 执行重新Hook逻辑
        // 先清理Hook

        // 重置状态
        this->d3dInited = false;
        this->NeedReHook = true;

        // 延迟重新创建Hook
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->CreateHook();

        //发送重新Hook消息
        MulNX::Message Msg("Core/ReHook"_hash);
        this->ISys().PublishAsync(std::move(Msg));

        ReHook = false;

        // UI系统主界面初始化
        this->StartUIWith("MainDraw");
    }
}

void HookManager::ThreadMain() {
    while (this->MyThreadRunning) {
        this->CheckHook();
        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
    }
}

void HookManager::ProcessMsg(MulNX::Message& Msg) {
	switch (Msg.type) {

	}
	return;
}

DWORD HookManager::CreateHook() {
	this->pSwapChain = nullptr;
	this->pd3dDevice = nullptr;
	while (this->NeedReHook) {
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
            this->hkPresent = MulNX::Memory::HookEx::Create((uint8_t*)IVClass::Assume(this->pSwapChain)->GetVFuncPtr(8), 5, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
                if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                    this->pSwapChain = (IDXGISwapChain*)ctx->rcx;
                    this->pUISystem->Render();
                }
                return true;
                }).value();
            this->hkPresent->Attach();

			this->pd3dDevice->Release();
			this->pSwapChain->Release();

			this->pUISystem->SetFrameBefore([this]()->void {

				this->d3dInit(this->pSwapChain);
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

			this->GuardPleaseAction = false;
            this->NeedReHook = false;
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
        this->hkWndProc = MulNX::Memory::HookEx::Create((uint8_t*)GetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC), 9, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
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