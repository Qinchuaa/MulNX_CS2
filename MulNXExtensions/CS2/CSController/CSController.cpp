#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXExtensions/CS2/PlayerHub/ProjectileTracker/ProjectileTracker.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

using AddEntity_t = void* (*)(void* This, CS2::C_BaseEntity* p, CS2::CHandleBase handle);
using RemoveEntity_t = void* (*)(void* This, CS2::C_BaseEntity* p, CS2::CHandleBase handle);

void CSController::HandleCameraSystemPlay(CS2::CViewSetup* viewSetup) {
    // 加载来自摄像机系统的View
    if (this->controlView.hasViewToGame.load(std::memory_order_acquire)) {
        auto view = this->controlView.ViewToGame.Read();
        *viewSetup->pViewOrigin() = view->position;
        *viewSetup->pViewAngles() = view->rotation;

        if (view->FOV > 0.01f) {
            *viewSetup->pFov() = view->FOV;
        }
    }
}

void CSController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
        this->Core->ModuleManager()->VirtualMain();
    }
    

    static auto* pProjectileTracker = this->Core->ModuleManager()->FindModule<ProjectileTracker>("ProjectileTracker");
    auto trckerView = pProjectileTracker->GetView();
    if (trckerView.has_value()) {
        *viewSetup->pViewOrigin() = trckerView.value().first;
        *viewSetup->pViewAngles() = trckerView.value().second;
    }

    // 同步窗口尺寸到ControlView
    this->controlView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

    // 执行roll覆盖，这是优先级最低的覆盖，保证运镜至少优先于此，且不影响于此
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);
    this->pAdvancedViewController->HandleUpdate(viewSetup);

    // 根据状态调用不同的视角控制逻辑
    // 自由摄像机优先级最高，其次是高级视角控制，最后是普通摄像机系统控制
    if (this->pFreeCameraController->HandleUpdate(viewSetup)) {
        this->pFreeCameraController->HandleOverrideView(viewSetup);
    }
    else if (this->pAdvancedViewController->HandleOverrideView(viewSetup)) {

    }
    else {
        this->HandleCameraSystemPlay(viewSetup);
    }

    // 记录视角数据
    {
        auto currentView = this->controlView.currentView.Write();

        currentView->position = *viewSetup->pViewOrigin();
        currentView->rotation = *viewSetup->pViewAngles();

        currentView->FOV = *viewSetup->pFov();
    }
}

bool CSController::UINodeFunc(MulNX::UINode* node) {
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
    node->CallUINode("PlayerFlashController");
    node->CallUINode("AdvancedViewController");
    node->CallUINode("FreeCameraController");

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

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});

    this->pAdvancedViewController = this->Core->ModuleManager()->FindModule<AdvancedViewController>("AdvancedViewController");
    this->pFreeCameraController = this->Core->ModuleManager()->FindModule<FreeCameraController>("FreeCameraController");

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

    static auto vtable = (uint8_t**)IVClass::Assume(this->Modules.client.dwGameEntitySystem())->GetVTablePtr();
    auto pAddEntity = vtable[15];

    static auto hkAddEntity = MulNX::Hook::Create(pAddEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            auto pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            MulNX::Message msg("Game/Entity/Added"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            this->ISys().PublishAsync(std::move(msg));
            return true;
        }).value();
    hkAddEntity->Attach();

    auto pRemoveEntity = vtable[16];
    static auto hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            auto pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            MulNX::Message msg("Game/Entity/Removed"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            this->ISys().PublishAsync(std::move(msg));
            return true;
        }).value();
    hkRemoveEntity->Attach();

    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->hkPosCallIsPlayingDemo = MulNX::Hook::Create(target.Data(), 0, true, [this](RegContext* ctx, MulNX::Hook* Hook)->bool {
                    this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
                    return true;
                    }).value();
                this->hkPosCallIsPlayingDemo->Attach();
            }
        }
    }

    this->controlView.dofs.pNearBlurry = this->CvarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CvarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CvarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CvarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

    this->SendTask("CS2控制线程", [this]()->bool {
        try {
            this->Update();
            this->EntryProcessMsg();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
        });

    return true;
}

void CSController::Update() {
    // 获取CS2全局变量
    this->CSGlobalVars = MulNX::MRead<C_GlobalVars*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);

    auto pGameRules = this->Modules.client.dwGameRules();
    if (!pGameRules) return;

    static int OldRoundStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    if (OldRoundStartCount != MulNX::MRead(pGameRules->m_nRoundStartCount())) {
        this->ISys().PublishAsync("Game/NewRound"_hash);
        OldRoundStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    }

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
        if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)continue;
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
            AL3DEntity.Team = static_cast<int>(team);
            AL3DEntity.Alive = AL3DEntity.HP;
            AL3DEntity.IndexInMap = playerNum;
        }
    }
    return;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    const auto& pos = IO->Frame.view.position;
    const auto& fov = IO->Frame.view.FOV;
    const auto& rot = IO->Frame.view.rotation;
    const auto& dof = IO->Frame.view.dof;

    {
        auto view = this->controlView.ViewToGame.Write();
        view->position = pos;
        view->FOV = fov;
        view->rotation = rot;
    }
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;

    this->controlView.hasViewToGame.store(true, std::memory_order_release);
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