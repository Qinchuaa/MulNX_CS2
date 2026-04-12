#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

void CSController::HandleCameraSystemPlay(CS2::CViewSetup* viewSetup) {
    // 加载来自摄像机系统的View
    auto view = this->controlView.ViewToGame.load(std::memory_order_acquire);
    if (view) {
        viewSetup->pViewOrigin()->x = view->OriginX;
        viewSetup->pViewOrigin()->y = view->OriginY;
        viewSetup->pViewOrigin()->z = view->OriginZ;

        viewSetup->pViewAngles()->x = view->AnglesX;
        viewSetup->pViewAngles()->y = view->AnglesY;
        viewSetup->pViewAngles()->z = view->AnglesZ;

        if (view->FOV > 0.01f) {
            *viewSetup->pFov() = view->FOV;
        }
    }
}

void CSController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    // 同步窗口尺寸到ControlView
    this->controlView.currentView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.currentView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

    // 执行roll覆盖
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);
    this->pAdvancedViewController->HandleUpdate(viewSetup);
    

    // 根据状态调用不同的视角控制逻辑
    // 自由摄像机优先级最高，其次是高级视角控制，最后是普通摄像机系统控制
    if (this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
        // 如果启用自由摄像机控制，同步角度到InputSystem
        auto& freeCam = this->pInputSystem->GetFreeCamera();
        freeCam.Rotation = *viewSetup->pViewAngles();
        freeCam.Update(this->pInputSystem);
        viewSetup->pViewOrigin()->x = freeCam.Position.x;
        viewSetup->pViewOrigin()->y = freeCam.Position.y;
        viewSetup->pViewOrigin()->z = freeCam.Position.z;
    }
    else if (this->pAdvancedViewController->HandleOverrideView(viewSetup)) {
        
    }
    else {
        this->HandleCameraSystemPlay(viewSetup);
    }

    // 记录视角数据
    this->controlView.currentView.OriginX.store(viewSetup->pViewOrigin()->x, std::memory_order_release);
    this->controlView.currentView.OriginY.store(viewSetup->pViewOrigin()->y, std::memory_order_release);
    this->controlView.currentView.OriginZ.store(viewSetup->pViewOrigin()->z, std::memory_order_release);

    this->controlView.currentView.AnglesX.store(viewSetup->pViewAngles()->x, std::memory_order_release);
    this->controlView.currentView.AnglesY.store(viewSetup->pViewAngles()->y, std::memory_order_release);
    this->controlView.currentView.AnglesZ.store(viewSetup->pViewAngles()->z, std::memory_order_release);

    this->controlView.currentView.FOV.store(*viewSetup->pFov(), std::memory_order_release);
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;
    MulNX::TransInfo info;
    info.pMatrix = this->GetViewMatrix();
    info.windowHeight = this->GetWinHeight();
    info.windowWidth = this->GetWinWidth();

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);
    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }

    if (ImGui::CollapsingHeader("时间控制")) {
        static float gameTimeScale = 1.0f;
        static float virtualTimeScale = 1.0f;
        ImGui::SliderFloat("游戏时间流速", &gameTimeScale, 0.0f, 5.0f);
        ImGui::SliderFloat("虚拟时间流速", &virtualTimeScale, 0.0f, 5.0f);

        if (ImGui::Button("启用时间虚拟化")) {
            this->AL3D->ExecuteCommand(std::format("host_timescale {}", gameTimeScale));
            this->AL3D->Time()->RefreshVirtual(true, virtualTimeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("解除时间虚拟化")) {
            this->AL3D->ExecuteCommand("host_timescale 1");
            this->AL3D->Time()->RefreshVirtual(false, 1.0f);
        }

        MulNX::UI::Checkbox("自动分析tick", this->autoTick);
        MulNX::UI::SliderInt("GameTick与DemoTick差值", this->deltaTick, 0, 10000);
    }
    if (ImGui::CollapsingHeader("烟雾弹控制")) {
        MulNX::UI::Checkbox("启用烟雾弹控制", this->controlSmoke.Enbale);
        MulNX::UI::Checkbox("烟雾显示", this->controlSmoke.Show);
        MulNX::UI::SliderFloat("色彩R", this->controlSmoke.R, 0, 255);
        MulNX::UI::SliderFloat("色彩G", this->controlSmoke.G, 0, 255);
        MulNX::UI::SliderFloat("色彩B", this->controlSmoke.B, 0, 255);
    }
    node->CallUINode("PlayerFlashController");
    node->CallUINode("AdvancedViewController");
    // 自由摄像机控制
    if (ImGui::CollapsingHeader("自由摄像机控制")) {
        bool currentEnable = this->EnableFreeCameraControl.load(std::memory_order_acquire);
        if (ImGui::Checkbox("启用自由摄像机位置控制", &currentEnable)) {
            if (currentEnable && !this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
                // 从未启用到启用：读取当前游戏位置和角度
                DirectX::XMFLOAT3 gamePos{
                    this->controlView.currentView.OriginX.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginY.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginZ.load(std::memory_order_acquire)
                };
                this->pInputSystem->GetFreeCamera().Position = gamePos;
            }
            this->EnableFreeCameraControl.store(currentEnable, std::memory_order_release);
        }

        if (currentEnable) {
            auto& freeCam = this->pInputSystem->GetFreeCamera();

            ImGui::Text("当前位置: X=%.2f, Y=%.2f, Z=%.2f",
                freeCam.Position.x, freeCam.Position.y, freeCam.Position.z);

            float speed = freeCam.MoveSpeed;
            if (ImGui::SliderFloat("移动速度", &speed, 10.0f, 1000.0f)) {
                freeCam.MoveSpeed = speed;
            }
        }
    }

    return true;
}

void CSController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Core/ReHook"_hash: {
        this->ISys().LogSucc("已完成Hook重载！");
        break;
    }
    case "Game/Command"_hash: {
        auto cmd = Msg.asp.get<MulNX::NetExt>()->str1;
        this->ExecuteCommand(cmd);
        break;
    }

    }
}

bool CSController::Init() {
    this->ShowWindow = true;
    this->ISys()
        .SubscribeAsync("Core/ReHook")
        .SubscribeAsync("Game/Command");
        
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->UINodeFunc(node);});

    this->pAdvancedViewController = this->Core->ModuleManager()->FindModule<AdvancedViewController>("AdvancedViewController");

    this->Modules.client = CS2::Module::Client(L"client.dll");
    this->Modules.engine2 = CS2::Module::engine2(L"engine2.dll");
    this->Modules.tier0 = MulNX::Memory::DllModule(L"tier0.dll");

    this->Source2EngineToClient001 =
        this->Modules.engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("Source2EngineToClient001", nullptr);
    this->executor = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(49);

    this->CvarSystem.Address =
        (uintptr_t)this->Modules.tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("VEngineCvar007", nullptr);

    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 0, true, [this](RegContext* ctx, MulNX::Memory::HookEx* hookEx)->bool {
                    this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
                    return true;
                }).value();
                this->MyHook->Attach();
            }
        }
    }

    this->controlView.dofs.pNearBlurry = this->CvarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CvarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CvarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CvarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

    this->SendTask("CS2控制线程", [this]()->bool {
        try {
            this->GetMsgResult = this->TryGetMsg();
            this->EntryProcessMsg();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
        return true;
        });

    return true;
}

int CSController::BasicUpdate() {
    // 获取CS2全局变量
    this->CSGlobalVars = MulNX::MRead<C_GlobalVars*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);

    auto pGameRules = this->Modules.client.dwGameRules();
    if (!pGameRules) return 1;

    static int OldRoundStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    if (OldRoundStartCount != MulNX::MRead(pGameRules->m_nRoundStartCount())) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    }
    
    for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
        auto entity = this->Modules.client.GetBaseEntity(i);
        if (!entity)continue;

        auto pClassInfo = MulNX::MRead(entity->pClassInfo());
        if (!pClassInfo)continue;

        auto pName = MulNX::MRead(((pClassInfo))->pName());
        if (!pName)continue;

        auto zname = std::string(pName);

        if (zname.find("smokegrenade") != std::string::npos && zname.find("weapon") == std::string::npos) {
            if (this->controlSmoke.Enbale.load(std::memory_order_acquire)) {
                if (this->controlSmoke.Show.load(std::memory_order_acquire) == true) {
                    auto* color = entity->As<CS2::C_SmokeGrenadeProjectile>()->vSmokeColor();
                    DirectX::XMFLOAT3 pushIn{
                        this->controlSmoke.R.load(std::memory_order_acquire) ,
                        this->controlSmoke.G.load(std::memory_order_acquire) ,
                        this->controlSmoke.B.load(std::memory_order_acquire) };
                    MulNX::MWrite(color, pushIn);
                }
                else {
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->bDidSmokeEffect(), false);
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->bSmokeEffectSpawned(), false);
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->nSmokeEffectTickBegin(), 0);
                }
            }
        }
    }
    return 0;
}
int CSController::EntityListUpdate() {
    std::unique_lock lock(this->smutex);
    // 玩家控制器，地图上从1到10
    int playerNum = 0;
    for (int i = 0; i < this->Modules.client.dwGameEntitySystem_highestEntityIndex(); ++i) {
        auto* controller = this->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!controller)continue;
        auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());
        auto* pawn = this->Modules.client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != 2 && team != 3)continue;
        ++playerNum;

        for (const auto& handle : this->handlesControlPlayer) {
            handle(controller, pawn);
        }

        if (playerNum <= 10) {
            auto& AL3DEntity = this->AL3DGameData.Players[playerNum];
            AL3DEntity.Position = MulNX::MRead(pawn->vOldOrigin());
            AL3DEntity.EyePosition = MulNX::MRead(pawn->vOldOrigin()) + MulNX::MRead(pawn->vecViewOffset());
            AL3DEntity.Rotation = MulNX::MRead(pawn->angEyeAngles());
            AL3DEntity.HP = MulNX::MRead(pawn->iHealth());
            AL3DEntity.Team = team;
            AL3DEntity.Alive = AL3DEntity.HP;
            AL3DEntity.IndexInMap = playerNum;
        }
    }

    return 0;
}
int CSController::TryGetMsg() {
    int Result = 0;
    Result = this->BasicUpdate();
    if (Result) {
        this->GlobalVars->InGamePlaying = false;
        return Result;
    }
    this->EntityListUpdate();

    this->GlobalVars->InGamePlaying = true;

    return 0;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    const auto& pos = IO->Frame.view.position;
    const auto& fov = IO->Frame.view.FOV;
    const auto& rot = IO->Frame.view.rotation;
    const auto& dof = IO->Frame.view.dof;
#ifdef _DEBUG
    // static MulNX::Math::Frame thisFrame;
    // if (thisFrame != IO->Frame) {
    //     thisFrame = IO->Frame;
    //     this->ISys().LogInfo(thisFrame.GetMsg());
    // }
#endif // _DEBUG
    auto view = std::make_shared<Views>();
    view->OriginX = pos.x;
    view->OriginY = pos.y;
    view->OriginZ = pos.z;
    view->FOV = fov;
    view->AnglesX = rot.x;
    view->AnglesY = rot.y;
    view->AnglesZ = rot.z;
    this->controlView.InputRoll.store(view->AnglesZ, std::memory_order_release);
    this->controlView.ViewToGame.store(view);

    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}
void CSController::HandleFirstPersonCameraPath(const CameraSystemIO* const IO) {
    static uint8_t LastIndex = 0xFFFF;
    if (LastIndex != IO->Frame.TargetPlayerIndexInMap) {
        this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(IO->Frame.TargetPlayerIndexInMap));
        LastIndex = IO->Frame.TargetPlayerIndexInMap;
    }
}
bool CSController::CameraSystemIOOverride(const CameraSystemIO* const IO) {
    static float LastCallTime = IO->FrameGameTime;
    if (LastCallTime == IO->FrameGameTime) {
        return true;
    }
    LastCallTime = IO->FrameGameTime;

    switch (IO->Frame.TargetOBMode) {
    case 4:this->HandleFreeCameraPath(IO); break;
    case 2:this->HandleFirstPersonCameraPath(IO); break;
    }

    return true;
}

// C_ConVar* m_yawPtr = nullptr;
// m_yawPtr = this->CvarSystem.GetCvar("m_yaw");
// std::vector<int> schemas;
// for (int schema = 0;schema < 0x100;++schema) {
//     const float targetValue = 0.0165f;
//     bool valueIsRight = false;
//     auto checkValue = [&]()->bool {
//         float currentValue = *(float*)((uintptr_t)m_yawPtr + schema);
//         if (fabs(currentValue - targetValue) < 0.001f) {
//             valueIsRight = true;
//         }
//         return true;// 无崩溃
//         };
//     MulNX::Base::UnsafeFunc(checkValue);
//     if (valueIsRight) {
//         schemas.push_back(schema);
//     }
// }