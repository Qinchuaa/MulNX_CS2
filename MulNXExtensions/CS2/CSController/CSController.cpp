#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>

void CSController::HandleOverrideView(void* ThisCViewSetup) {
    // 定位关键数据
    int* pWidth = (int*)((char*)ThisCViewSetup + 0x434);
    int* pHeight = (int*)((char*)ThisCViewSetup + 0x43C);
    this->controlView.currentView.WindowWidth.store(*pWidth, std::memory_order_relaxed);
    this->controlView.currentView.WindowHeight.store(*pHeight, std::memory_order_relaxed);

    float* pFov = (float*)((char*)ThisCViewSetup + 0x498);
    float* pViewOrigin = (float*)((char*)ThisCViewSetup + 0x4a0);
    float* pViewAngles = (float*)((char*)ThisCViewSetup + 0x4b8);

    // 加载来自摄像机系统的View
    auto view = this->controlView.ViewToGame.load(std::memory_order_acquire);
    if (view != nullptr) {
        pViewOrigin[0] = view->OriginX;
        pViewOrigin[1] = view->OriginY;
        pViewOrigin[2] = view->OriginZ;

        pViewAngles[0] = view->AnglesX;
        pViewAngles[1] = view->AnglesY;
        pViewAngles[2] = view->AnglesZ;

        if (view->FOV > 0.01f) {
            *pFov = view->FOV;
        }
    }
    // 执行roll覆盖
    pViewAngles[2] = this->controlView.InputRoll.load(std::memory_order_acquire);
    // 记录视角数据
    this->controlView.currentView.AnglesX.store(pViewAngles[0], std::memory_order_release);
    this->controlView.currentView.AnglesY.store(pViewAngles[1], std::memory_order_release);
    this->controlView.currentView.AnglesZ.store(pViewAngles[2], std::memory_order_release);
    this->controlView.currentView.OriginX.store(pViewOrigin[0], std::memory_order_release);
    this->controlView.currentView.OriginY.store(pViewOrigin[1], std::memory_order_release);
    this->controlView.currentView.OriginZ.store(pViewOrigin[2], std::memory_order_release);
    if (*pFov < 0.01f) {
        this->controlView.currentView.FOV.store(90.0f, std::memory_order_release);
    }
    else {
        this->controlView.currentView.FOV.store(*pFov, std::memory_order_release);
    }
    // try {
    //     for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
    //         auto entity = this->Modules.client.GetBaseEntity(i);
    //         if (entity == 0) {
    //             continue;
    //         }
    //         auto hPawn = MulNX::MRead(entity->As<CS2::CBasePlayerController>()->hPawn());
    //         if (!hPawn.Valid()) {
    //             continue;
    //         }
    //         auto* pawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
    //         if (!pawn) {
    //             continue;
    //         }
    //         auto* pGameSceneNode = MulNX::MRead(pawn->pGameSceneNode());
    //         if (!pGameSceneNode)continue;
    //         auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
    //         if (!bones)continue;

    //         MulNX::TransInfo info;
    //         info.pMatrix = this->GetViewMatrix();
    //         info.windowHeight = 1080;
    //         info.windowWidth = 1920;

    //         DirectX::XMFLOAT3 pos15 = MulNX::MRead(bones->at(15));
    //         DirectX::XMFLOAT3 pos16 = MulNX::MRead(bones->at(16));

    //         DirectX::XMFLOAT3 euler;
    //         MulNX::Math::CSDirToEuler(pos16 - pos15, euler);

    //         pViewOrigin[0] = pos15.x;
    //         pViewOrigin[1] = pos15.y;
    //         pViewOrigin[2] = pos15.z;

    //         pViewAngles[0] = euler.x;
    //         pViewAngles[1] = euler.y;
    //         break;
    //     }
    // }
    // catch (const std::runtime_error& e) {
    //     this->ISys().LogWarning("捕获到在应用自拍杆时发生的异常");
    // }

    return;
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179, 179);

    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    if (ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179));
    MulNX::UI::Checkbox("摄像机模式", this->controlView.CameraMode);
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
#ifdef _DEBUG
    try {
        for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
            auto* pEntity = this->Modules.client.GetBaseEntity(i);
            if (pEntity == nullptr) {
                continue;
            }
            auto hPawn = MulNX::MRead(pEntity->As<CS2::CBasePlayerController>()->hPawn());
            if (!hPawn.Valid()) {
                continue;
            }
            auto* pPawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
            if (!pPawn) {
                continue;
            }
            auto* pGameSceneNode = MulNX::MRead(pPawn->pGameSceneNode());
            if (!pGameSceneNode)continue;
            auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
            if (!bones)continue;

            MulNX::TransInfo info;
            info.pMatrix = this->GetViewMatrix();
            info.windowHeight = this->GetWinHeight();
            info.windowWidth = this->GetWinWidth();

            for (int i = 0;i < 34;++i) {
                DirectX::XMFLOAT3 pos = MulNX::MRead(bones->at(i));
                MulNX::UI::DrawWorldPoint(pos, info, std::to_string(i).c_str());
            }
        }
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogWarning("捕获到在绘制时发生的异常");
    }


#endif
    return true;
}

void CSController::VirtualMain() {
    this->EntryProcessMsg();
    //RECT rect;
    //GetClientRect(GetActiveWindow(), &rect);
    //int width = rect.right - rect.left;
    //int height = rect.bottom - rect.top;
    
    //this->AL3DCurrentWindowHeight = height;
    //this->AL3DCurrentWindowWidth = width;

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
    this->NeedUINode = true;

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

    //this->LocalPlayer.pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();

    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 16, false, [this](RegContext* ctx)->void {
                    return this->HandleOverrideView((void*)ctx->rsi);
                    });
                this->MyHook->Attach();
            }
        }
    }

    return true;
}

int CSController::BasicUpdate() {
    // 获取EntityList
    this->EntityList.Address = this->Modules.client.dwEntityList();
    // 获取CS2全局变量
    this->CSGlobalVars.Address = MulNX::MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);
    
    static int OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
    if (OldRoundStartCount != MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount())) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
    }
    for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
        auto entity = this->Modules.client.GetBaseEntity(i);
        if (entity == nullptr) {
            continue;
        }

        auto pClassInfo = MulNX::MRead(entity->pClassInfo());
        if (pClassInfo == nullptr) {
            continue;
        }


        auto pName = MulNX::MRead(((pClassInfo))->pName());
        if (pName == nullptr) {
            continue;
        }
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

            //auto pOrigin = (*entity->pGameSceneNode())->vecAbsOrigin();

            // auto view = std::make_shared<Views>();
            // view->OriginX = pOrigin->x;
            // view->OriginY = pOrigin->y;
            // view->OriginZ = pOrigin->z;
            // view->FOV = 90;
            // view->AnglesX = 0;
            // view->AnglesY = 0;
            // view->AnglesZ = 0;
            // this->ViewToGame.store(view);
        }
    }
#ifdef _DEBUG

    // int highestEntityIndex = *this->Modules.client.dwGameEntitySystem_highestEntityIndex();
    // for (int i = 0;i <= highestEntityIndex;i++) {
    //     auto entity = this->Modules.client.GetBaseEntity(i);
    //     if (entity == 0) {
    //         continue;
    //     }
    //     auto hPawn = *entity->As<CS2::CBasePlayerController>()->hPawn();
    //     if (!hPawn.Valid()) {
    //         continue;
    //     }
    //     auto* pawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
    //     if (!pawn) {
    //         continue;
    //     }
    //     auto* pRot = pawn->As<CS2::C_CSPlayerPawn>()->angEyeAngles();
    //     auto t = std::string(*(*pawn->pClassInfo())->pName());
    //     if (t.find("layer") == std::string::npos) {
    //         continue;
    //     }
    //     auto* pPlayerRot = (*pawn->pGameSceneNode())->angAbsRotation();
    //     auto* pPlayerRot2 = (*pawn->pGameSceneNode())->angRotation();
    //     auto* pPlayerRot3 = (*pawn->pGameSceneNode())->angWrappedLocalRotation();

    //     auto** ppWeaponServices = pawn->As<CS2::C_BasePlayerPawn>()->pWeaponServices();
    //     if (!ppWeaponServices) {
    //         continue;
    //     }
    //     auto pWeaponServices = *ppWeaponServices;
    //     if (!pWeaponServices) {
    //         continue;
    //     }
    //     auto phWeapon = pWeaponServices->hActiveWeapon();
    //     if (!phWeapon || reinterpret_cast<uintptr_t>(phWeapon) == 0xFFFFFFFFFFFFFFFF) {
    //         continue;
    //     }
    //     CS2::CHandle<CS2::C_BasePlayerWeapon> hWeapon = *phWeapon;
    //     if (!hWeapon.Valid()) {
    //         continue;
    //     }
    //     auto weapon = this->Modules.client.GetBaseEntity(hWeapon.GetIndexInEntityList());
    //     if (!weapon) {
    //         continue;
    //     }

    //     auto pGameSceneNode = *weapon->pGameSceneNode();
    //     if (!pGameSceneNode) {
    //         continue;
    //     }
    //     char* pName = *(*weapon->pClassInfo())->pName();
    //     auto* pWeaponPos = pGameSceneNode->vecAbsOrigin();

    //     if (!pWeaponPos) {
    //         continue;
    //     }
    //     DirectX::XMFLOAT3 pos = *pWeaponPos;
    //     DirectX::XMFLOAT3 rot = *pRot;

    //     auto zipaiView = std::make_shared<Views>();
    //     zipaiView->AnglesX = 89.0f;
    //     zipaiView->AnglesY = rot.y;
    //     zipaiView->AnglesZ = rot.z;

    //     zipaiView->OriginX = pos.x;
    //     zipaiView->OriginY = pos.y;
    //     zipaiView->OriginZ = pos.z + 120.0f;

    //     zipaiView->FOV = 90.0f;

    //     //this->ViewToGame.store(zipaiView);
    //     //this->GlobalVars->CampathPlaying = true;
    // }
#endif 
    return 0;
}
int CSController::EntityListUpdate() {
    //更新实体列表
    this->EntityList.Update();

    int IndexInMap = 1;
    for (int i = 0; i < 64; ++i) {
        if (IndexInMap == 11) {
            break;
        }
        C_Entity& Entity = this->EntityList.at(i);
        int team = Entity.GetPawn().m_iTeamNum;
        if (team == 2 || team == 3) {
            D_Player PlayerMsg{
            .Position = Entity.GetPawn().m_vOldOrigin,
            .EyePosition = Entity.GetPawn().GetEyePos(),
            .Rotation = Entity.GetPawn().m_angEyeAngles,
            .HP = Entity.GetPawn().m_iHealth,
            .Team = team,
            .Alive = (Entity.GetPawn().m_iHealth > 0),
            .IndexInEntityList = Entity.IndexInEntityList,
            .IndexInMap = IndexInMap,
            };

            std::unique_lock lock(this->GetMutex());
            this->AL3DGameData.Players[PlayerMsg.IndexInMap] = std::move(PlayerMsg);
            lock.unlock();
        }
        ++IndexInMap;
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
    Result = this->EntityListUpdate();
    if (Result) {
        return Result;
    }

    uintptr_t ppPlantedC4 = ppPlantedC4 = MulNX::MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwPlantedC4);
    if (ppPlantedC4) {
        this->PlantedC4.Address = MulNX::MRead<uintptr_t>(ppPlantedC4);
        this->PlantedC4.Update();
    }


    this->CSGlobalVars.Update();
    this->GlobalVars->InGamePlaying = true;

    return 0;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    DirectX::XMFLOAT4 PosAndFOV = IO->Frame.GetPositionAndFOV();
    DirectX::XMFLOAT3 RotEuler = IO->Frame.GetRotationEuler();
#ifdef _DEBUG
    static MulNX::Math::Frame thisFrame;
    if (thisFrame != IO->Frame) {
        thisFrame = IO->Frame;
        this->ISys().LogInfo(thisFrame.GetMsg());
    }
#endif // _DEBUG
    auto view = std::make_shared<Views>();
    view->OriginX = PosAndFOV.x;
    view->OriginY = PosAndFOV.y;
    view->OriginZ = PosAndFOV.z;
    view->FOV = PosAndFOV.w;
    view->AnglesX = RotEuler.x;
    view->AnglesY = RotEuler.y;
    view->AnglesZ = RotEuler.z;
    this->controlView.InputRoll.store(view->AnglesZ, std::memory_order_release);
    this->controlView.ViewToGame.store(view);
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

// void CSController::HandleAimAtEntity(int AimTargetIndexInMap) {
//     DirectX::XMFLOAT3 LocalEyePos = this->LocalPlayer.Entity.Pawn.GetEyePos();
//     DirectX::XMFLOAT3 TargetEyePos;
//     int TargetIndexInEntityList = this->GetIndexInEntityListFromIndexInMap(AimTargetIndexInMap);
//     if (TargetIndexInEntityList == -1)return;
//     TargetEyePos = this->EntityList.GetEntity(TargetIndexInEntityList).Pawn.GetEyePos();

//     DirectX::XMFLOAT3 dir = TargetEyePos - LocalEyePos;
//     DirectX::XMFLOAT3 TargetViewAngles{};
//     MulNX::Math::CSDirToEuler(dir, TargetViewAngles);

//     this->LocalPlayer.SetViewAngle(TargetViewAngles);
// }

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