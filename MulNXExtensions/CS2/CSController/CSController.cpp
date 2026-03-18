#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>

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
    ImGui::Begin("CS摄像机控制");

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

    ImGui::End();

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
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();
    this->ISys()
        .SubscribeAsync("Core/ReHook");
    this->NeedThread(3);
    this->NeedUINode = true;

    this->GetModules();
    this->Catch();
    
    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 14, [this](RegContext* ctx)->void {
                    return this->HandleOverrideView((void*)ctx->rsi);
                    });
                this->MyHook->Attach();
            }
        }
    }

    return true;
}

void CSController::GetModules() {
    this->Modules.client = MulNX::Memory::DllModule(L"client.dll");
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
}

void CSController::Catch() {
    // 本地工作
    // 获取本地视图投影矩阵
    this->LocalPlayer.ViewMatrix = reinterpret_cast<float*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewMatrix);
    // 获取本地欧拉角
    this->LocalPlayer.ViewAngles = reinterpret_cast<DirectX::XMFLOAT3*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewAngles);

    return;
}
int CSController::BasicUpdate() {
    // 获取EntityList
    if (!MulNX::Memory::Read(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList, this->EntityList.Address)) {
        return -1;
    }
    // 获取CS2全局变量
    if (!MulNX::Memory::Read(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars, this->CSGlobalVars.Address)) {
        return -2;
    }
    //获取本地控制器
    if (!MulNX::Memory::Read(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerController, this->LocalPlayer.Entity.Controller.Address)) {
        return -1001;
    }
    if (int result = this->LocalPlayer.Update()) {
        return result;
    }

    static int OldRoundStartCount = this->CSGameRules.m_nRoundStartCount;
    if (OldRoundStartCount != this->CSGameRules.m_nRoundStartCount) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = this->CSGameRules.m_nRoundStartCount;
    }

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
int CSController::GameRulesUpdate() {
    MulNX::Memory::Read(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameRules, this->CSGameRules.Address);
    this->CSGameRules.Update();
    return 0;
}

void CSController::ThreadMain() {
    this->GetMsgResult = this->TryGetMsg();
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
    Result = this->GameRulesUpdate();
    if (Result) {
        return Result;
    }

    uintptr_t ppPlantedC4;
    MulNX::Memory::Read(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwPlantedC4, ppPlantedC4);
    if (ppPlantedC4) {
        MulNX::Memory::Read(ppPlantedC4, this->PlantedC4.Address);
        this->PlantedC4.Update();
    }


    this->CSGlobalVars.Update();

    this->CurrentTime = this->CSGlobalVars.CurrentTime;

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
    //this->LocalPlayer.SetPosition(PosAndFOV);
    view->OriginX = PosAndFOV.x;
    view->OriginY = PosAndFOV.y;
    view->OriginZ = PosAndFOV.z;
    //this->LocalPlayer.SetFOV(PosAndFOV.w);
    view->FOV = PosAndFOV.w;
    //this->LocalPlayer.SetViewAngle(RotEuler);
    view->AnglesX = RotEuler.x;
    view->AnglesY = RotEuler.y;
    view->AnglesZ = RotEuler.z;
    this->ViewToGame.store(view);
    // if (this->LocalPlayer.Entity.Pawn.m_pObserverServices.m_iObserverMode == 4) {
        
    // }
    // else {
    //     this->Execute("spec_mode 4");
    // }
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