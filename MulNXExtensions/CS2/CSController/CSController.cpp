#include "CSController.hpp"

#include "../../CameraSystem/CameraSystemIO/CameraSystemIO.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

std::atomic<bool> IsInCameraSystemOverride = false;
std::atomic<float> OriginX = 0;
std::atomic<float> OriginY = 0;
std::atomic<float> OriginZ = 0;
std::atomic<float> AnglesX = 0;
std::atomic<float> AnglesY = 0;
std::atomic<float> AnglesZ = 0;
std::atomic<float> FOV = 90.0f;

void HandleOverrideView(void* ThisCViewSetup) {

    int* pWidth = (int*)((unsigned char*)ThisCViewSetup + 0x434);
    int* pHeight = (int*)((unsigned char*)ThisCViewSetup + 0x43C);

    float* pFov = (float*)((unsigned char*)ThisCViewSetup + 0x498);
    float* pViewOrigin = (float*)((unsigned char*)ThisCViewSetup + 0x4a0);
    float* pViewAngles = (float*)((unsigned char*)ThisCViewSetup + 0x4b8);

    if (IsInCameraSystemOverride.load()) {
        pViewOrigin[0] = OriginX.load();
        pViewOrigin[1] = OriginY.load();
        pViewOrigin[2] = OriginZ.load();

        pViewAngles[0] = AnglesX.load();
        pViewAngles[1] = AnglesY.load();
        pViewAngles[2] = AnglesZ.load();

        *pFov = FOV.load();
    }
    return;
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

void CSController::Execute(const char* cmd) {
    vmt::CallVirtual<void>(49, this->CmdInterface, 0, cmd, 1);
    return;
}

void CSController::InitInterface() {
    const char* ModuleName = "engine2.dll";
    const char* InterfaceName = "Source2EngineToClient001";
    HMODULE hModule = GetModuleHandleA(ModuleName);
    if (!hModule) return;
    auto pFunc = reinterpret_cast<void* (*)(const char*, int*)>(GetProcAddress(hModule, "CreateInterface"));
    if (!pFunc) return;
    this->CmdInterface = pFunc(InterfaceName, nullptr);
}
bool CSController::Init() {
    this->InitInterface();
    this->GetModules();
    this->Catch();
    this->EntryCreateThread();// 包含线程创建
    this->SetMyThreadDelta(3);
    this->AL3D->SetCmdInterface([this](const char* command) {
        this->Execute(command);
        return true;
        });
    this->AL3D->SetCameraSystemIOOverrideFunc([this](const CameraSystemIO* const IO) {
        return this->CameraSystemIOOverride(IO);
        });
    this->AL3D->SetGetViewMatrixFunc([this]() {
        return this->LocalPlayer.ViewMatrix;
        });
    this->AL3D->SetGetSpatialStateFunc([this]() {
        return this->LocalPlayer.GetSpatialState();
        });

    uintptr_t clientBase = 0;
    size_t clientSize = 0;
    if (MulNX::Memory::GetModuleInfo(L"client.dll", clientBase, clientSize)) {
        // 搜索 .text 段
        uintptr_t textBase = 0;
        size_t textSize = 0;
        if (MulNX::Memory::GetTextSectionRange(clientBase, textBase, textSize)) {
            MulNX::Memory::Region textRegion(textBase, textSize);
            // 搜索特征码
            MulNX::Memory::Pattern pattern("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 48 01 00 00 0f 57 ff 84 c0 74 63 ba ff ff ff ff");
            auto Target = MulNX::Memory::Accessor::FindRegion(textRegion, pattern);
            if (Target.IsValid()) {
                MulNX::Memory::Region::ProtectionGuard Guard = Target.ExchangeProtection(PAGE_EXECUTE_READWRITE);
                unsigned char asmCode[16] = {
                    0x48, 0x89, 0xf1,
                    0x48, 0xb8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                    0xff, 0x10,
                    0x90
                };
                static LPVOID ptr = HandleOverrideView;
                LPVOID ptrPtr = &ptr;

                memcpy(&asmCode[5], &ptrPtr, sizeof(LPVOID));
                memcpy((LPVOID)Target.begin(), asmCode, 16);
            }
        }
    }

    return true;
}

void CSController::GetModules() {
    this->Modules.client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    this->Modules.engine2 = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"engine2.dll"));
    this->Modules.tier0 = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"tier0.dll"));

    const char* ModuleName = "tier0.dll";
    const char* InterfaceName = "VEngineCvar007";
    HMODULE hModule = GetModuleHandleA(ModuleName);
    auto pFunc = reinterpret_cast<void* (*)(const char*, int*)>(GetProcAddress(hModule, "CreateInterface"));
    this->CvarSystem.Address = (uintptr_t)pFunc(InterfaceName, nullptr);

    this->LocalPlayer.pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
}

void CSController::Catch() {
    // 本地工作
    // 获取本地视图投影矩阵
    this->LocalPlayer.ViewMatrix = reinterpret_cast<float*>(this->Modules.client + cs2_dumper::offsets::client_dll::dwViewMatrix);
    // 获取本地欧拉角
    this->LocalPlayer.ViewAngles = reinterpret_cast<DirectX::XMFLOAT3*>(this->Modules.client + cs2_dumper::offsets::client_dll::dwViewAngles);

    return;
}
int CSController::BasicUpdate() {
    // 获取EntityList
    if (!MulNX::Memory::Read(this->Modules.client + cs2_dumper::offsets::client_dll::dwEntityList, this->EntityList.Address)) {
        return -1;
    }
    // 获取CS2全局变量
    if (!MulNX::Memory::Read(this->Modules.client + cs2_dumper::offsets::client_dll::dwGlobalVars, this->CSGlobalVars.Address)) {
        return -2;
    }
    //获取本地控制器
    if (!MulNX::Memory::Read(this->Modules.client + cs2_dumper::offsets::client_dll::dwLocalPlayerController, this->LocalPlayer.Entity.Controller.Address)) {
        return -1001;
    }
    if (int result = this->LocalPlayer.Update()) {
        return result;
    }

    static int OldRoundStartCount = this->CSGameRules.m_nRoundStartCount;
    if (OldRoundStartCount != this->CSGameRules.m_nRoundStartCount) {
        MulNX::Message Msg(MulNX::MsgType::Game_NewRound);
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
            this->AL3D->UpdatePlayerMsg(std::move(PlayerMsg));
            this->IndexInMap_To_IndexInEntityList_Map[IndexInMap] = Entity.IndexInEntityList;
            ++IndexInMap;
        }
    }
    return 0;
}
int CSController::GameRulesUpdate() {
    MulNX::Memory::Read(this->Modules.client + cs2_dumper::offsets::client_dll::dwGameRules, this->CSGameRules.Address);
    this->CSGameRules.Update();
    return 0;
}

void CSController::ThreadMain() {
    this->GetMsgResult = this->TryGetMsg();
    IsInCameraSystemOverride.store(this->GlobalVars->CampathPlaying);
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
    MulNX::Memory::Read(this->Modules.client + cs2_dumper::offsets::client_dll::dwPlantedC4, ppPlantedC4);
    if (ppPlantedC4) {
        MulNX::Memory::Read(ppPlantedC4, this->PlantedC4.Address);
        this->PlantedC4.Update();
    }

    if (this->CSGameRules.m_bBombPlanted) {
        this->AL3D->SetPhaseStartTime(this->PlantedC4.m_flC4Blow - this->PlantedC4.m_flTimerLength);
        this->AL3D->SetPhaseDuration(this->PlantedC4.m_flTimerLength);
    }
    else {
        this->AL3D->SetPhaseStartTime(this->CSGameRules.m_fRoundStartTime);
        this->AL3D->SetPhaseDuration(this->CSGameRules.m_iRoundTime);
    }

    this->CSGlobalVars.Update();
    uintptr_t CurrentTimePointer = this->AL3D->GetCurrentTimePointer();
    uintptr_t GlobalVarsPointer = this->CSGlobalVars.GetCurrentTimePointer();
    if (CurrentTimePointer != GlobalVarsPointer) {
        this->AL3D->SetCurrentTimePointer(GlobalVarsPointer);
    }

    this->CurrentTime = this->CSGlobalVars.CurrentTime;

    this->GlobalVars->InGamePlaying = true;

    return 0;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    if (this->LocalPlayer.Entity.Pawn.m_pObserverServices.m_iObserverMode == 4) {
        DirectX::XMFLOAT4 PosAndFOV = IO->Frame.GetPositionAndFOV();
        DirectX::XMFLOAT3 RotEuler = IO->Frame.GetRotationEuler();
#ifdef _DEBUG
        static MulNX::Base::Math::Frame thisFrame;
        if (thisFrame != IO->Frame) {
            thisFrame = IO->Frame;
            this->ISys().LogInfo(thisFrame.GetMsg());
        }
#endif // _DEBUG
        //this->LocalPlayer.SetPosition(PosAndFOV);
        OriginX = PosAndFOV.x;
        OriginY = PosAndFOV.y;
        OriginZ = PosAndFOV.z;
        //this->LocalPlayer.SetFOV(PosAndFOV.w);
        FOV = PosAndFOV.w;
        //this->LocalPlayer.SetViewAngle(RotEuler);
        AnglesX = RotEuler.x;
        AnglesY = RotEuler.y;
        AnglesZ = RotEuler.z;
    }
    else {
        this->Execute("spec_mode 4");
    }
}
void CSController::HandleFirstPersonCameraPath(const CameraSystemIO* const IO) {
    static uint8_t LastIndex = 0xFFFF;
    if (LastIndex != IO->Frame.TargetPlayerIndexInMap) {
        this->Execute(("spec_mode 2;spec_player " + std::to_string(IO->Frame.TargetPlayerIndexInMap)).c_str());
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
    MulNX::Base::Math::CSDirToEuler(dir, TargetViewAngles);

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