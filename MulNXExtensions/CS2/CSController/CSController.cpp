#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>
#include <MulNX/Systems/InputSystem/InputSystem.hpp>

void CSController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    this->controlView.currentView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.currentView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

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

    // 执行roll覆盖
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);
    // 如果启用自由摄像机控制，同步角度到InputSystem
    if (this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
        auto* inputSys = this->pInputSystem;
        auto& freeCam = inputSys->GetFreeCamera();
        freeCam.Rotation = *viewSetup->pViewAngles();
        freeCam.Update(inputSys);
        viewSetup->pViewOrigin()->x = freeCam.Position.x;
        viewSetup->pViewOrigin()->y = freeCam.Position.y;
        viewSetup->pViewOrigin()->z = freeCam.Position.z;
    }

    // 记录视角数据
    this->controlView.currentView.OriginX.store(viewSetup->pViewOrigin()->x, std::memory_order_release);
    this->controlView.currentView.OriginY.store(viewSetup->pViewOrigin()->y, std::memory_order_release);
    this->controlView.currentView.OriginZ.store(viewSetup->pViewOrigin()->z, std::memory_order_release);

    this->controlView.currentView.AnglesX.store(viewSetup->pViewAngles()->x, std::memory_order_release);
    this->controlView.currentView.AnglesY.store(viewSetup->pViewAngles()->y, std::memory_order_release);
    this->controlView.currentView.AnglesZ.store(viewSetup->pViewAngles()->z, std::memory_order_release);

    this->controlView.currentView.FOV.store(*viewSetup->pFov(), std::memory_order_release);

    if (!this->controlAdvancedView.Enable.load(std::memory_order_acquire))return;

    // 通过时间桥判断是否需要更新视角，防止抖动
    static auto lastTime = this->AL3D->Time()->GetReal();
    auto currentTime = this->AL3D->Time()->GetReal();
    if (currentTime > lastTime) {
        auto result = this->HandleSelfViewUpdate();
        if (result != 0) {
            this->ISys().LogWarning("HandleSelfViewUpdate returned error code: " + std::to_string(result));
        }
        lastTime = currentTime;
    }
        
    // 输出平滑后的值，滚转角强制为0（保持水平）
    *viewSetup->pViewOrigin() = this->controlAdvancedView.smoothCameraPos;
    *viewSetup->pViewAngles() = this->controlAdvancedView.smoothCameraAngle;
    viewSetup->pViewAngles()->z = 0;  // 滚转角保持水平
}

int CSController::HandleSelfViewUpdate() {
    // ==================== 摄像机绑定逻辑（武器枪口视角） ====================
    // 摄像机与骨骼9的偏移距离（单位）
    float CAMERA_OFFSET = this->controlAdvancedView.distance.load(std::memory_order_acquire);
    try {
        // ---------- 获取目标 ----------
        auto* localController = this->Modules.client.dwLocalPlayerController();
        if (!localController) {
            return 1;
        }
        auto hLocalPawn = MulNX::MRead(localController->hPawn());
        auto* localPawn = this->Modules.client.GetBaseEntityFromHandle(hLocalPawn)->As<CS2::C_CSPlayerPawn>();
        if (!localPawn) {
            return 2;
        }
        auto hObserverTarget = localPawn->GetHandleObserverTarget();
        auto* target = this->Modules.client.GetBaseEntityFromHandle(hObserverTarget)->As<CS2::C_CSPlayerPawn>();
        if (!target) {
            return 3;
        }

        // ---------- 获取武器骨骼 ----------
        auto hActiveWeapon = target->GetHandleActiveWeapon();
        auto* pWeapon = this->Modules.client.GetBaseEntityFromHandle(hActiveWeapon)->As<CS2::C_BasePlayerWeapon>();
        if (!pWeapon) {
            return 4;
        }

        // 两个参考点
        DirectX::XMFLOAT3 gunPos = pWeapon->GetBonePos(this->controlAdvancedView.boneIndex1.load(std::memory_order_acquire));
        DirectX::XMFLOAT3 gunDirRef = pWeapon->GetBonePos(this->controlAdvancedView.boneIndex2.load(std::memory_order_acquire));

        // 计算枪口指向单位向量（从枪口指向参考点）
        DirectX::XMFLOAT3 forwardDir = gunDirRef - gunPos;
        float len = sqrtf(forwardDir.x * forwardDir.x + forwardDir.y * forwardDir.y + forwardDir.z * forwardDir.z);
        if (len > 0.0001f) {
            forwardDir.x /= len;
            forwardDir.y /= len;
            forwardDir.z /= len;
        }
        else {
            return 5;  // 方向无效，放弃修改
        }

        // 目标摄像机位置
        DirectX::XMFLOAT3 targetCameraPos = {
            gunDirRef.x + forwardDir.x * CAMERA_OFFSET,
            gunDirRef.y + forwardDir.y * CAMERA_OFFSET,
            gunDirRef.z + forwardDir.z * CAMERA_OFFSET
        };

        // 目标摄像机角度
        DirectX::XMFLOAT3 lookDir = {
            gunPos.x - targetCameraPos.x,
            gunPos.y - targetCameraPos.y,
            gunPos.z - targetCameraPos.z
        };
        len = sqrtf(lookDir.x * lookDir.x + lookDir.y * lookDir.y + lookDir.z * lookDir.z);
        if (len > 0.0001f) {
            lookDir.x /= len;
            lookDir.y /= len;
            lookDir.z /= len;
        }
        else {
            return 6;  // 方向无效，放弃修改
        }

        DirectX::XMFLOAT3 targetCameraAngle;
        MulNX::Math::CSDirToEuler(lookDir, targetCameraAngle);

        // ---------- 平滑处理 ----------
        if (!this->controlAdvancedView.initialized) {
            this->controlAdvancedView.smoothCameraPos = targetCameraPos;
            this->controlAdvancedView.smoothCameraAngle = targetCameraAngle;
            this->controlAdvancedView.initialized = true;
        }

        // 指数平滑位置
        this->controlAdvancedView.smoothCameraPos.x += (targetCameraPos.x - this->controlAdvancedView.smoothCameraPos.x) * this->controlAdvancedView.SMOOTH_FACTOR;
        this->controlAdvancedView.smoothCameraPos.y += (targetCameraPos.y - this->controlAdvancedView.smoothCameraPos.y) * this->controlAdvancedView.SMOOTH_FACTOR;
        this->controlAdvancedView.smoothCameraPos.z += (targetCameraPos.z - this->controlAdvancedView.smoothCameraPos.z) * this->controlAdvancedView.SMOOTH_FACTOR;

        // 指数平滑角度（处理角度环绕）
        auto angleDiff = [](float target, float current) -> float {
            float diff = target - current;
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
            return diff;
            };
        this->controlAdvancedView.smoothCameraAngle.x += angleDiff(targetCameraAngle.x, this->controlAdvancedView.smoothCameraAngle.x) * this->controlAdvancedView.SMOOTH_FACTOR;
        this->controlAdvancedView.smoothCameraAngle.y += angleDiff(targetCameraAngle.y, this->controlAdvancedView.smoothCameraAngle.y) * this->controlAdvancedView.SMOOTH_FACTOR;
        this->controlAdvancedView.smoothCameraAngle.z += angleDiff(targetCameraAngle.z, this->controlAdvancedView.smoothCameraAngle.z) * this->controlAdvancedView.SMOOTH_FACTOR;
    }
    catch (...) {
        // 发生异常时，重置平滑状态，避免使用无效数据
        this->controlAdvancedView.initialized = false;
        // 不清空位置/角度，让游戏继续使用原有视角（或后续帧重新初始化）
        return 0xffff;  // 返回特定错误码，表示发生异常
    }
    return 0;
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

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

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);

    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    //MulNX::UI::Checkbox("摄像机模式", this->controlView.CameraMode);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }
    if (ImGui::CollapsingHeader("烟雾弹控制")) {
        MulNX::UI::Checkbox("启用烟雾弹控制", this->controlSmoke.Enbale);
        MulNX::UI::Checkbox("烟雾显示", this->controlSmoke.Show);
        MulNX::UI::SliderFloat("色彩R", this->controlSmoke.R, 0, 255);
        MulNX::UI::SliderFloat("色彩G", this->controlSmoke.G, 0, 255);
        MulNX::UI::SliderFloat("色彩B", this->controlSmoke.B, 0, 255);
    }
    if (ImGui::CollapsingHeader("高级视角控制")) {
        MulNX::UI::Checkbox("启用高级视角控制", this->controlAdvancedView.Enable);

        MulNX::UI::SliderInt("骨骼起点", this->controlAdvancedView.boneIndex1, 0, 127);
        MulNX::UI::SliderInt("骨骼终点", this->controlAdvancedView.boneIndex2, 0, 127);

        MulNX::UI::SliderFloat("距离控制", this->controlAdvancedView.distance, 0.0f, 200.0f);
    }

    // 自由摄像机控制
    if (ImGui::CollapsingHeader("自由摄像机控制")) {
        bool currentEnable = this->EnableFreeCameraControl.load(std::memory_order_acquire);
        if (ImGui::Checkbox("启用自由摄像机位置控制", &currentEnable)) {
            if (currentEnable && !this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
                // 从未启用到启用：读取当前游戏位置和角度
                DirectX::XMFLOAT3 gamePos{ this->controlView.currentView.OriginX.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginY.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginZ.load(std::memory_order_acquire) };
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
#ifdef _DEBUG

    // try {
    //     for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
    //         auto* pEntity = this->Modules.client.GetBaseEntity(i);
    //         if (!pEntity)continue;
    //         auto hPawn = MulNX::MRead(pEntity->As<CS2::CBasePlayerController>()->hPawn());
    //         if (!hPawn.Valid())continue;
    //         auto* pPawn = this->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
    //         if (!pPawn)continue;
    //         auto* pGameSceneNode = MulNX::MRead(pPawn->pGameSceneNode());
    //         if (!pGameSceneNode)continue;
    //         auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
    //         if (!bones)continue;

    //         auto* pWeaponServices = MulNX::MRead(pPawn->pWeaponServices());
    //         auto hAc = MulNX::MRead(pWeaponServices->hActiveWeapon());
    //         auto* pWeapon = this->Modules.client.GetBaseEntityFromHandle(hAc)->As<CS2::C_BasePlayerWeapon>();
    //         auto* pWeaponsGameSceneNode = MulNX::MRead(pWeapon->pGameSceneNode());
    //         auto* bones2 = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pWeaponsGameSceneNode)->unkBoneArray());

    //         auto* pGlow = pPawn->Glow();
    //         auto* pGlowColor = pGlow->fGlowColor();;

    //         MulNX::TransInfo info;
    //         info.pMatrix = this->GetViewMatrix();
    //         info.windowHeight = this->GetWinHeight();
    //         info.windowWidth = this->GetWinWidth();

    //         for (int i = 0;i < 34;++i) {
    //             DirectX::XMFLOAT3 pos = MulNX::MRead(bones->at(i));
    //             DirectX::XMFLOAT3 pos2 = MulNX::MRead(bones2->at(i));
    //             MulNX::UI::DrawWorldPoint(pos, info, std::to_string(i).c_str());
    //             MulNX::UI::DrawWorldPoint(pos2, info, std::to_string(i).c_str());
    //         }
    //     }
    // }
    // catch (const std::runtime_error& e) {
    //     this->ISys().LogWarning("捕获到在绘制时发生的异常");
    // }


#endif
    return true;
}

void CSController::VirtualMain() {
    this->EntryProcessMsg();
    return;
}
void CSController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Core/ReHook"_hash: {
        this->ISys().LogSucc("已完成Hook重载！");
        break;
    }

    }
}

bool CSController::Init() {
    this->ShowWindow = true;
    this->ISys()
        .SubscribeAsync("Core/ReHook");
    this->NeedThread(3);
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->UINodeFunc(node);});

    this->Modules.client = CS2::Module::Client(L"client.dll");
    this->Modules.engine2 = MulNX::Memory::DllModule(L"engine2.dll");
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
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 16, false, [this](RegContext* ctx)->void {
                    return this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
                    });
                this->MyHook->Attach();
            }
        }
    }

    this->controlView.dofs.pNearBlurry = this->CvarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CvarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CvarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CvarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

    return true;
}

int CSController::BasicUpdate() {
    // 获取CS2全局变量
    this->CSGlobalVars = MulNX::MRead<C_GlobalVars*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);

    static int OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
    if (OldRoundStartCount != MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount())) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
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
    std::unique_lock lock(this->GetMutex());
    // 玩家控制器，地图上从1到10，位于索引1到10处
    for (int i = 1; i <= 10; ++i) {
        auto* controller = this->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!controller)continue;
        auto hPawn = MulNX::MRead(controller->hPawn());
        auto* pawn = this->Modules.client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != 2 && team != 3)continue;

        auto& AL3DEntity = this->AL3DGameData.Players[i];
        AL3DEntity.Position = MulNX::MRead(pawn->vOldOrigin());
        AL3DEntity.EyePosition = MulNX::MRead(pawn->vOldOrigin()) + MulNX::MRead(pawn->vecViewOffset());
        AL3DEntity.Rotation = MulNX::MRead(pawn->angEyeAngles());
        AL3DEntity.HP = MulNX::MRead(pawn->iHealth());
        AL3DEntity.Team = team;
        AL3DEntity.Alive = AL3DEntity.HP;
        AL3DEntity.IndexInMap = i;
    }

    return 0;
}

void CSController::ThreadMain() {
    while (this->MyThreadRunning) {
        try {
            this->GetMsgResult = this->TryGetMsg();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
    }
    return;
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