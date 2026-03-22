#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>

using namespace MulNX::Memory::ReadWrite;

DirectX::XMFLOAT3 BonePos(uintptr_t addr, int32_t index) {
    int32_t d = 32 * index;
    uintptr_t pGameSceneNode = *reinterpret_cast<uintptr_t*>(addr + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    if (!pGameSceneNode)return {};
    auto BoneArraySchema = cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80;
    uintptr_t BoneArray = *reinterpret_cast<uintptr_t*>(pGameSceneNode + BoneArraySchema);
    if (!pGameSceneNode)return {};
    return *reinterpret_cast<DirectX::XMFLOAT3*>(BoneArray + d);
}

void CSController::HandleOverrideView(void* ThisCViewSetup) {
    // 定位关键数据
    int* pWidth = (int*)((unsigned char*)ThisCViewSetup + 0x434);
    int* pHeight = (int*)((unsigned char*)ThisCViewSetup + 0x43C);

    float* pFov = (float*)((unsigned char*)ThisCViewSetup + 0x498);
    float* pViewOrigin = (float*)((unsigned char*)ThisCViewSetup + 0x4a0);
    float* pViewAngles = (float*)((unsigned char*)ThisCViewSetup + 0x4b8);

    // 加载来自摄像机系统的View
    auto view = this->ViewToGame.load(std::memory_order_acquire);
    // 如果处于摄像机轨道播放中
    if (this->GlobalVars->CampathPlaying.load(std::memory_order_acquire)) {
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

            this->atoRoll.store(view->AnglesZ, std::memory_order_release);
        }
        return;
    }
    // 记录关键数据
    if (*pFov < 0.01f) {
        this->outFOV.store(90.0f, std::memory_order_release);
    }
    else {
        this->outFOV.store(*pFov, std::memory_order_release);
    }
    pViewAngles[2] = this->atoRoll.load(std::memory_order_acquire);
    return;
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

    auto roll = this->atoRoll.load(std::memory_order_acquire);
    if (ImGui::SliderFloat("roll调整", &roll, -179, 179)) {
        this->atoRoll.store(roll, std::memory_order_release);
    }

    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    if (ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179));

    if (ImGui::Button("一键归正")) {
        this->atoRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }
#ifdef _DEBUG
    int highestEntityIndex = *this->Modules.client.dwGameEntitySystem_highestEntityIndex();
    for (int i = 0;i <= highestEntityIndex;i++) {
        auto entity = this->Modules.client.GetBaseEntity(i);
        if (entity == 0) {
            continue;
        }
        auto hPawn = *entity->As<CS2::CBasePlayerController>()->hPawn();
        if (!hPawn.Valid()) {
            continue;
        }
        auto* pawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
        if (!pawn) {
            continue;
        }
        // DirectX::XMFLOAT3 pos5 = BonePos(reinterpret_cast<uintptr_t>(pawn), 5);
        // DirectX::XMFLOAT3 pos6 = BonePos(reinterpret_cast<uintptr_t>(pawn), 6);
        // DirectX::XMFLOAT3 pos7 = BonePos(reinterpret_cast<uintptr_t>(pawn), 7);
        // DirectX::XMFLOAT3 pos8 = BonePos(reinterpret_cast<uintptr_t>(pawn), 8);

        // DirectX::XMFLOAT2 D5, D6, D7, D8;

        // MulNX::Math::XMWorldToScreen(pos5, D5, this->GetViewMatrix(), 1920, 1080);
        // MulNX::Math::XMWorldToScreen(pos6, D6, this->GetViewMatrix(), 1920, 1080);
        // MulNX::Math::XMWorldToScreen(pos7, D7, this->GetViewMatrix(), 1920, 1080);
        // MulNX::Math::XMWorldToScreen(pos8, D8, this->GetViewMatrix(), 1920, 1080);

        // ImGui::GetBackgroundDrawList()->AddLine({ D5.x,D5.y }, { D6.x,D6.y }, IM_COL32(255, 0, 0, 255));
        // ImGui::GetBackgroundDrawList()->AddLine({ D6.x,D6.y }, { D7.x,D7.y }, IM_COL32(255, 0, 0, 255));
        // ImGui::GetBackgroundDrawList()->AddLine({ D7.x,D7.y }, { D8.x,D8.y }, IM_COL32(255, 0, 0, 255));
    }


#endif
    return true;
}

int CSController::GetIndexInEntityListFromIndexInMap(int IndexInMap) {
    std::shared_lock lock(this->IndexMapMtx);
    auto it = this->IndexInMap_To_IndexInEntityList_Map.find(IndexInMap);
    if (it != this->IndexInMap_To_IndexInEntityList_Map.end()) {
        return it->second;
    }
    //未找到对应关系
    return -1;
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
    this->LocalPlayer.pGlobalFOV = &this->outFOV;

    // 获取本地视图投影矩阵
    this->LocalPlayer.ViewMatrix = reinterpret_cast<float*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewMatrix);
    // 获取本地欧拉角
    this->LocalPlayer.ViewAngles = reinterpret_cast<DirectX::XMFLOAT3*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewAngles);

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
    this->EntityList.Address = MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList);
    // 获取CS2全局变量
    this->CSGlobalVars.Address = MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);
    //获取本地控制器
    this->LocalPlayer.Entity.Controller.Address = MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerController);
    if (int result = this->LocalPlayer.Update()) {
        return result;
    }

    static int OldRoundStartCount = this->CSGameRules.m_nRoundStartCount;
    if (OldRoundStartCount != this->CSGameRules.m_nRoundStartCount) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = this->CSGameRules.m_nRoundStartCount;
    }
#ifdef _DEBUG
    for (int i = 0;i <= *this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
        auto entity = this->Modules.client.GetBaseEntity(i);
        if (entity == 0) {
            continue;
        }

        auto pClassInfo = entity->pClassInfo();
        if (pClassInfo == 0) {
            continue;
        }


        auto pName = *((*pClassInfo)->pName());
        if (pName == 0) {
            continue;
        }
        auto zname = std::string(pName);

        if (zname.find("smokegrenade") != std::string::npos && zname.find("weapon") == std::string::npos) {
            // 找到了
            MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->bDidSmokeEffect(), false);
            //*entity->As<CS2::C_SmokeGrenadeProjectile>()->bSmokeEffectSpawned() = false;
            //*entity->As<CS2::C_SmokeGrenadeProjectile>()->nSmokeEffectTickBegin() = 0;
            auto color = entity->As<CS2::C_SmokeGrenadeProjectile>()->vSmokeColor();
            static float x = 255;
            static float y = 1;
            static float z = 255;

            color->x = x;
            color->y = y;
            color->z = z;

            auto pOrigin = (*entity->pGameSceneNode())->vecAbsOrigin();

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

    //处理映射关系，更新3D层数据
    std::unique_lock lock(this->IndexMapMtx);
    this->IndexInMap_To_IndexInEntityList_Map.clear();
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

            this->IndexInMap_To_IndexInEntityList_Map[IndexInMap] = Entity.IndexInEntityList;
            ++IndexInMap;
        }
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
    this->CSGameRules.Address = MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameRules);
    this->CSGameRules.Update();

    uintptr_t ppPlantedC4;
    ppPlantedC4 = MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwPlantedC4);
    if (ppPlantedC4) {
        this->PlantedC4.Address = MRead<uintptr_t>(ppPlantedC4);
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
    this->ViewToGame.store(view);
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

    switch (IO->CurrentElementType) {
    case ElementType::FreeCameraPath:this->HandleFreeCameraPath(IO); break;
    case ElementType::FirstPersonCameraPath:this->HandleFirstPersonCameraPath(IO); break;
    }

    return true;
}

void CSController::HandleAimAtEntity(int AimTargetIndexInMap) {
    DirectX::XMFLOAT3 LocalEyePos = this->LocalPlayer.Entity.Pawn.GetEyePos();
    DirectX::XMFLOAT3 TargetEyePos;
    int TargetIndexInEntityList = this->GetIndexInEntityListFromIndexInMap(AimTargetIndexInMap);
    if (TargetIndexInEntityList == -1)return;
    TargetEyePos = this->EntityList.GetEntity(TargetIndexInEntityList).Pawn.GetEyePos();

    DirectX::XMFLOAT3 dir = TargetEyePos - LocalEyePos;
    DirectX::XMFLOAT3 TargetViewAngles{};
    MulNX::Math::CSDirToEuler(dir, TargetViewAngles);

    this->LocalPlayer.SetViewAngle(TargetViewAngles);
}

C_CSGameRules CSController::GetCSGameRules() {
    std::shared_lock lock(this->CSGameRules.GameRulesMtx);
    return this->CSGameRules;
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