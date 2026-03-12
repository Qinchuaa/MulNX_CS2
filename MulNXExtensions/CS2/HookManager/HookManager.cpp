#include "HookManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/All_ImGui.hpp>
#pragma comment(lib,"d3d11.lib")
#include <chrono>

static bool AllowReHook = false;// 允许重hook
bool HookManager::Init() {
    this->pInstance = this;
    MH_Initialize();
    return true;
}
void HookManager::StartAll() {
	this->MainMsgChannel = this->ICreateAndGetMessageChannel();
	this->ReHook = true;
    this->NeedThread(250);
    this->ThreadMain();// 手动执行一次Hook
}

void HookManager::ThreadMain() {
	this->EntryProcessMsg();

	if (this->GuardPleaseAction) {
		AllowReHook = true;
		this->GuardPleaseAction = false;
		this->ISys().LogInfo("检测到D3D11波动，等待用户手动ReHook");
	}
	// 检查是否超时：正在等待CheckBack且超过2秒未收到回复
	if (AllowReHook) {
		if (this->KT->CheckComboClick(VK_INSERT, 2)) {
			ReHook = true;
			AllowReHook = false;
		}
	}
	if (ReHook) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		// 执行重新Hook逻辑
		// 先清理Hook
		this->hkPresent.Clear();
		this->hkRelease.Clear();
		

		// 重置状态
		this->d3dInited = false;
		this->NeedReHook = true;

		// 延迟重新创建Hook
		std::this_thread::sleep_for(std::chrono::seconds(1));
		this->CreateHook();

		//发送重新Hook消息
		MulNX::Message Msg("Core_ReHook"_hash);
		this->ISys().PublishAsync(std::move(Msg));

		ReHook = false;

		// UI系统主界面初始化
		this->StartUIWith("MainDraw");
	}
}

void HookManager::ProcessMsg(MulNX::Message* Msg) {
	switch (Msg->Type) {

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
			auto pVtable = (void***)(this->pSwapChain);
			auto Vtable = *pVtable;

			this->hkRelease.SetTarget(Vtable[2]);
			this->hkRelease.SetMyFunction([this](auto&&... args) {
				return this->MyRelease(std::forward<decltype(args)>(args)...); });
			this->hkRelease.CreateAndEnable();

			this->hkPresent.SetTarget(Vtable[8]);
			this->hkPresent.SetMyFunction([this](auto&&... args) {
				return this->MyPresent(std::forward<decltype(args)>(args)...); });
			this->hkPresent.CreateAndEnable();

			this->pd3dDevice->Release();
			this->pSwapChain->Release();

			this->Core->IUISystem().SetFrameBefore([this]()->void {

				this->d3dInit(this->pSwapChain);
				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				return;
				});
			this->Core->IUISystem().SetFrameBehind([this]()->void {

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

LRESULT __stdcall HookManager::EntryMyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (HookManager::pInstance->MyWndProc(hwnd, uMsg, wParam, lParam)) {
		return true;
	}	
	return CallWindowProcW(HookManager::pInstance->OriginWndProc, hwnd, uMsg, wParam, lParam);
}



// ImGui窗口处理函数导入
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall HookManager::MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	std::unique_lock lock(this->Core->IUISystem().UIMtx);
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
		return true;
	}

	ImGuiIO& io = ImGui::GetIO();
	// 鼠标：当ImGui想要捕获时总是拦截
	if (io.WantCaptureMouse && MulNX::Base::WIN32Msg::IsMouseMessage(uMsg)) {
		return true;
	}
	// 键盘：只在WantTextInput为true时拦截（表示输入框激活）
	else if (io.WantTextInput && MulNX::Base::WIN32Msg::IsKeyboardMessage(uMsg)) {
		return true;
	}

	return false;
}
HRESULT __stdcall HookManager::MyPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
	if(!this->OriginWndProc)
		this->OriginWndProc = (WNDPROC)SetWindowLongPtrW(this->CS2hWnd, GWLP_WNDPROC, (LONG_PTR)HookManager::pInstance->EntryMyWndProc);
	if (this->GlobalVars->SystemReady) {
		this->pSwapChain = swapChain;
		this->Core->IUISystem().Render();
	}
	return 0;
}
ULONG __stdcall HookManager::MyRelease(IUnknown* pThis) {
	//MessageBoxW(NULL, L"交换链被释放", L"D3D11SwapChain", NULL);
	this->NeedReHook = true;
	this->GuardPleaseAction = true;

	return 0;
}

//if (!this->GlobalVars->DebugMode) {
//	ImGui::Text("调试模式未启用，无法使用该功能，请前往控制台打开");
//	return;
//}
//// 显示当前位置和旋转
//ImGui::Text("本地位置: X: %.6f, Y: %.6f, Z: %.6f",
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetPosition().x,
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetPosition().y,
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetPosition().z);
//ImGui::Text("本地欧拉角: Pitch: %.6f, Yaw: %.6f, Roll: %.6f",
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetRotationEuler().x,
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetRotationEuler().y,
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetRotationEuler().z);
//
//int FOV = this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.CameraServices.iFOV;
//ImGui::Text("FOV:%d", FOV);
//int FOVStart = this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.CameraServices.FOVStart;
//ImGui::Text("FOVStart:%d", FOVStart);
//float FOVRate = this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.CameraServices.FOVRate;
//ImGui::Text("FOVRate:%.4f", FOVRate);
//float FOVTime = this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.CameraServices.FOVTime;
//ImGui::Text("FOVTime:%.4f", FOVTime);
//static bool F1 = false, F2 = false;
//ImGui::Checkbox("F1", &F1);
//ImGui::Checkbox("F2", &F2);
//
//ImGui::Separator();
//ImGui::Separator();
//
//
//if (this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.pGlobalFOV) {
//	int GlobalFOV = *this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.pGlobalFOV;
//	static bool GlobalFOVControl = false;
//	ImGui::Checkbox("GlobalFOVControl", &GlobalFOVControl);
//
//	static float inputGFOV = 0;
//	ImGui::SliderFloat("全局FOV", &inputGFOV, 0, 360);
//
//	if (GlobalFOVControl) {
//		*this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.pGlobalFOV = inputGFOV;
//	}
//}
//
//ImGui::Separator();
//ImGui::Separator();
//
//static DirectX::XMFLOAT3 TestPosition;
//static DirectX::XMFLOAT3 TestRotationEuler;
//static DirectX::XMFLOAT4 TestRotation4;
//
//// 检测K键按下
//if (ImGui::IsKeyPressed(ImGuiKey_K)) {
//	TestPosition = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetPosition();
//	TestRotationEuler = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetRotationEuler();
//	MulNX::Base::Math::CSEulerToQuat(TestRotationEuler, TestRotation4);
//}
//
//// 显示存储的测试值
//ImGui::Separator();
//ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "按下K存储数值:");
//ImGui::Text("新位置: X: %.6f, Y: %.6f, Z: %.6f",
//	TestPosition.x, TestPosition.y, TestPosition.z);
//ImGui::Text("新欧拉角: X: %.6f, Y: %.6f, Z: %.6f",
//	TestRotationEuler.x, TestRotationEuler.y, TestRotationEuler.z);
//ImGui::Text("新四元数: X: %.6f, Y: %.6f, Z: %.6f, W: %.6f",
//	TestRotation4.x, TestRotation4.y, TestRotation4.z, TestRotation4.w);
//
//DirectX::XMFLOAT3 back{};
//MulNX::Base::Math::CSQuatToEuler(TestRotation4, back);
//ImGui::Text("回转欧拉角：X: %.6f, Y: %.6f, Z: %.6f",
//	back.x, back.y, back.z);
//
//
//
//ImGui::Separator();
//ImGui::Separator();
//ImGui::Separator();
//
//static float CameraHigh = 20.0;
//static float CameraX = 30.0;
//static float CameraY = 15.0;
//static float AxisLenth = 10.0;
//
//ImGui::SliderFloat("摄像机高度", &CameraHigh, 1.0, 500.0);
//ImGui::SliderFloat("摄像机长度", &CameraX, 1.0, 500.0);
//ImGui::SliderFloat("摄像机宽度", &CameraY, 1.0, 500.0);
//ImGui::SliderFloat("摄像机坐标轴长度", &AxisLenth, 1.0, 100.0);
//
//ImGui::SliderFloat3("位置", &TestPosition.x, -2000.0, 2000, 0);
//ImGui::SliderFloat("俯仰角", &TestRotationEuler.x, -89.0, 89.0);
//ImGui::SliderFloat("偏航角", &TestRotationEuler.y, -179.0, 179.0);
//ImGui::SliderFloat("滚转角", &TestRotationEuler.z, -179.0, 179.0);
//
//static ImVec4 cameraColourVec = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // RGBA，每个分量0-1
//ImGui::ColorEdit4("相机颜色", (float*)&cameraColourVec);
//
//
//
//// 将ImVec4转换为ImU32
//ImU32 cameraColour = IM_COL32(
//	(int)(cameraColourVec.x * 255),
//	(int)(cameraColourVec.y * 255),
//	(int)(cameraColourVec.z * 255),
//	(int)(cameraColourVec.w * 255)
//);
//
////this->Core->CameraSystem().CameraDrawer.Init(CameraHigh, CameraX, CameraY, AxisLenth, cameraColour);
////this->Core->CameraSystem().CameraDrawer.Update(this->Core->CS().CSGetMatrix(), this->Core->CS().CSGetScreenWide(), this->Core->CS().CSGetScreenHigh());
////this->Core->CameraSystem().CameraDrawer.DrawCamera(TestPosition, TestRotationEuler, "旧测试");
//this->Core->ModuleManager()->FindModule<ICameraSystem>("CameraSystem")->ResetCameraModule(CameraHigh, CameraX, CameraY, AxisLenth, cameraColour);
//this->Core->ModuleManager()->FindModule<ICameraSystem>("CameraSystem")->DrawCameraByPAR(TestPosition, TestRotationEuler, "测试摄像机");
//
//ImGui::Separator();
//ImGui::Separator();
//ImGui::Separator();
//
//
//ImGui::Text(("OB模式：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_iObserverMode)).c_str());
//ImGui::Text(("m_hObserverTarget：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_hObserverTarget)).c_str());
//ImGui::Text(("m_iObserverLastMode：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_iObserverLastMode)).c_str());
//ImGui::Text(("m_bForcedObserverMode：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_bForcedObserverMode)).c_str());
//ImGui::Text(("m_flObserverChaseDistance：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_flObserverChaseDistance)).c_str());
//ImGui::Text(("m_flObserverChaseDistanceCalcTime：" + std::to_string(this->Core->ModuleManager()->FindModule<CSController>("CSController")->LocalPlayer.Entity.Pawn.m_pObserverServices.m_flObserverChaseDistanceCalcTime)).c_str());
//















//ImGui::Text("调试监控");
//static int Result = 0;
//auto* pCS = this->Core->ModuleManager()->FindModule<CSController>("CSController");
//if (!pCS)return;
//Result = pCS->GetMsgResult;
//ImGui::Text("CSController信息获取返回值: %d", Result);
//if (Result < 0) {
//	this->AddError("成功捕获一次内核错误！");
//}
//if (!this->GlobalVars->DebugMode) {
//	ImGui::Text("调试模式未启用，无法使用该功能，请前往控制台打开");
//	return;
//}
//
//ImGui::Separator();
//
//ImGui::Text("本地测试");
//std::ostringstream Msg = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetLocalPlayer().GetMsg();
//ImGui::Text(Msg.str().c_str());
//
//ImGui::Separator();
//ImGui::Separator();
//
//ImGui::Text("当前时间： %f", this->Core->ModuleManager()->FindModule<CSController>("CSController")->CSGetCurrentTime());
//
//ImGui::Separator();
//
//float C4BlowTime = this->Core->ModuleManager()->FindModule<CSController>("CSController")->PlantedC4.m_flC4Blow;
//ImGui::Text("炸弹爆炸时间： %f", C4BlowTime);
//ImGui::Text("倒计时: %f", this->AL3D->GetPhaseRemainingTime());
//
//ImGui::Separator();
//ImGui::Separator();
//// 基本游戏状态
//C_CSGameRules CSGameRules = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCSGameRules();
//
//ImGui::Text("冻结期： %s", CSGameRules.m_bFreezePeriod ? "是" : "否");
//ImGui::Text("热身期： %s", CSGameRules.m_bWarmupPeriod ? "是" : "否");
//ImGui::Separator();
//ImGui::Text("热身开始时间： %f", CSGameRules.m_fWarmupPeriodStart);
//ImGui::Text("热身结束时间： %f", CSGameRules.m_fWarmupPeriodEnd);
//static int TargetSkipTickForRenderTick = 0;
//ImGui::InputInt("TargetSkipTickForRenderTick", &TargetSkipTickForRenderTick);
//if (ImGui::Button("应用TargetSkipTickForRenderTick")) {
//	static int TickSchemaed = 0;
//	TickSchemaed = TargetSkipTickForRenderTick - CSGameRules.m_fWarmupPeriodEnd * 64;
//	this->Core->ModuleManager()->FindModule<CSController>("CSController")->Execute(("demo_gototick " + std::to_string(TickSchemaed)).c_str());
//}
//ImGui::Separator();
//
//ImGui::Text("T方暂停： %s", CSGameRules.m_bTerroristTimeOutActive ? "是" : "否");
//ImGui::Text("CT方暂停： %s", CSGameRules.m_bCTTimeOutActive ? "是" : "否");
//ImGui::Text("T方暂停剩余： %f", CSGameRules.m_flTerroristTimeOutRemaining);
//ImGui::Text("CT方暂停剩余： %f", CSGameRules.m_flCTTimeOutRemaining);
//ImGui::Text("T方暂停次数： %d", CSGameRules.m_nTerroristTimeOuts);
//ImGui::Text("CT方暂停次数： %d", CSGameRules.m_nCTTimeOuts);
//ImGui::Text("技术暂停： %s", CSGameRules.m_bTechnicalTimeOut ? "是" : "否");
//ImGui::Text("比赛等待恢复： %s", CSGameRules.m_bMatchWaitingForResume ? "是" : "否");
//ImGui::Separator();
//
//
//// 回合时间信息
//ImGui::Text("回合在未安装炸弹的情况下最长储蓄秒数： %d", CSGameRules.m_iRoundTime);
//ImGui::Text("比赛开始时间： %f", CSGameRules.m_fMatchStartTime);
//ImGui::Text("回合开始时间： %f", CSGameRules.m_fRoundStartTime);
//ImGui::Text("重启回合时间： %f", CSGameRules.m_flRestartRoundTime);
//ImGui::Separator();
//
//
//ImGui::Text("游戏重启： %s", CSGameRules.m_bGameRestart ? "是" : "否");
//ImGui::Text("游戏开始时间： %f", CSGameRules.m_flGameStartTime);
//ImGui::Separator();
//
//
//ImGui::Text("下一阶段开始时间： %f", CSGameRules.m_timeUntilNextPhaseStarts);
//ImGui::Text("游戏阶段： %d", CSGameRules.m_gamePhase);
//ImGui::Separator();
//
//
//// 回合计数信息
//ImGui::Text("总已完成回合数： %d", CSGameRules.m_totalRoundsPlayed);
//ImGui::Text("当前阶段已完成回合数： %d", CSGameRules.m_nRoundsPlayedThisPhase);
//ImGui::Text("加时回合： %d", CSGameRules.m_nOvertimePlaying);
//ImGui::Separator();
//
//
//ImGui::Text("剩余人质： %d", CSGameRules.m_iHostagesRemaining);
//ImGui::Text("有人质到达： %s", CSGameRules.m_bAnyHostageReached ? "是" : "否");
//ImGui::Separator();
//
//
//// 地图信息
//ImGui::Text("有爆破区： %s", CSGameRules.m_bMapHasBombTarget ? "是" : "否");
//ImGui::Text("有救援区： %s", CSGameRules.m_bMapHasRescueZone ? "是" : "否");
//ImGui::Text("有购买区： %s", CSGameRules.m_bMapHasBuyZone ? "是" : "否");
//ImGui::Separator();
//
//
//// 比赛设置
//ImGui::Text("m_bIsQueuedMatchmaking： %s", CSGameRules.m_bIsQueuedMatchmaking ? "是" : "否");
//ImGui::Text("m_nQueuedMatchmakingMode： %d", CSGameRules.m_nQueuedMatchmakingMode);
//ImGui::Text("Valve服务器： %s", CSGameRules.m_bIsValveDS ? "是" : "否");
//ImGui::Text("Logo地图： %s", CSGameRules.m_bLogoMap ? "是" : "否");
//ImGui::Text("m_bPlayAllStepSoundsOnServer： %s", CSGameRules.m_bPlayAllStepSoundsOnServer ? "是" : "否");
//ImGui::Text("m_iSpectatorSlotCount： %d", CSGameRules.m_iSpectatorSlotCount);
//ImGui::Text("m_MatchDevice： %d", CSGameRules.m_MatchDevice);
//ImGui::Text("比赛已开始： %s", CSGameRules.m_bHasMatchStarted ? "是" : "否");
//ImGui::Text("m_nNextMapInMapgroup： %d", CSGameRules.m_nNextMapInMapgroup);
//ImGui::Separator();
//
//
//// 炸弹状态
//ImGui::Text("炸弹掉落： %s", CSGameRules.m_bBombDropped ? "是" : "否");
//ImGui::Text("炸弹已安置： %s", CSGameRules.m_bBombPlanted ? "是" : "否");
//ImGui::Separator();
//
//
//// 回合结束信息
//ImGui::Text("上回合总时间： %d", CSGameRules.m_iRoundEndTimerTime);
//ImGui::Text("上回合结束玩家数： %d", CSGameRules.m_iRoundEndPlayerCount);
//ImGui::Text("上回合结束无音乐： %s", CSGameRules.m_bRoundEndNoMusic ? "是" : "否");
//ImGui::Separator();
//
//
//
//ImGui::Text("回合开始编号： %d", CSGameRules.m_iRoundStartRoundNumber);
//ImGui::Text("回合开始计数： %d", CSGameRules.m_nRoundStartCount);
//ImGui::Text("回合结束计数： %d", CSGameRules.m_nRoundEndCount);
//ImGui::Separator();
//
//
//// 其他信息
//ImGui::Text("最后性能采样时间： %f", CSGameRules.m_flLastPerfSampleTime);
//
//ImGui::Separator();
//ImGui::Separator();
//
////return;
//
//
//for (size_t i = 0; i < 64; ++i) {
//	try {
//		C_Entity Entity = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetEntityList().GetEntity(i);
//		ImGui::Text("编号(位于游戏EntityList)：%d", Entity.IndexInEntityList);
//		ImGui::SameLine();
//		ImGui::Text(Entity.GetMsg().str().c_str());
//	}
//	catch (...) {
//		continue;
//	}
//
//}