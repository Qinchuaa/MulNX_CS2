#include "LocalPlayer.hpp"

#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

using namespace MulNX::Memory::ReadWrite;

std::ostringstream C_LocalPlayer::GetMsg() {
    std::shared_lock lock(this->LocalPlayerMutex);
    std::ostringstream oss;
    oss << "   本地玩家信息:" << this->Entity.GetMsg().str()
        << "   HUD信息： " << this->Entity.Pawn.m_iHideHUD;
    return oss;
}

MulNX::Math::View C_LocalPlayer::GetView()const {
    std::shared_lock lock(this->LocalPlayerMutex);

    MulNX::Math::View view;
    view.position = this->Entity.Pawn.GetEyePos();
    view.rotation = *this->ViewAngles;

    return view;
}

DirectX::XMFLOAT3 C_LocalPlayer::GetPosition() {
    std::shared_lock lock(this->LocalPlayerMutex);
    return *this->PositionA;
}
DirectX::XMFLOAT3 C_LocalPlayer::GetRotationEuler() {
    std::shared_lock lock(this->LocalPlayerMutex);
    return *this->ViewAngles;
}
float C_LocalPlayer::GetFov() {
    std::shared_lock lock(this->LocalPlayerMutex);
    return static_cast<float>(this->Entity.Pawn.CameraServices.FOVStart);
}
float* C_LocalPlayer::GetMatrix() {
    return this->ViewMatrix;
}


void C_LocalPlayer::SetViewAngle(const DirectX::XMFLOAT3& Angles) {
    __try {
        DirectX::XMFLOAT3* pRotation = this->ViewAngles;
        pRotation->x = Angles.x;
        pRotation->y = Angles.y;
        pRotation->z = Angles.z;

        return;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }
    return;
}
bool C_LocalPlayer::SetPosition(const DirectX::XMFLOAT4& PosAndFOV) {
    __try {
        DirectX::XMFLOAT3* pPositionA = this->PositionA;
        DirectX::XMFLOAT3* pPositionB = this->PositionB;

        // pPositionB->x = PosAndFOV.x;
        // pPositionB->y = PosAndFOV.y;
        // pPositionB->z = PosAndFOV.z;

        pPositionA->x = PosAndFOV.x;
        pPositionA->y = PosAndFOV.y;
        pPositionA->z = PosAndFOV.z;

        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int C_LocalPlayer::Update() {
    std::unique_lock lock(this->LocalPlayerMutex);
    // 判断本地控制器是否可用
    if (!this->Entity.Controller.Address) {
        return 1003;
    }
    {
        MulNX::Memory::ReadString(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName,
            this->Entity.Controller.m_iszPlayerName,
            sizeof(this->Entity.Controller.m_iszPlayerName));
        // 获取本地控制器的期望FOV
        this->Entity.Controller.m_iDesiredFOV = MRead<uint32_t>(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iDesiredFOV);
    }
    // 获取本地实体句柄
    this->Entity.Controller.hPawn = MRead<uint32_t>(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn);
    if (this->Entity.Controller.hPawn == 0xFFFFFFFF)return 2001;
    // 获取本地实体
    this->Entity.Pawn.Address = C_EntityList::GetEntityPawnFromHandle(this->Entity.Controller.hPawn);
    if (!this->Entity.Pawn.Address)return 3001;
    // 摄像机模块
    {
        // 获取本地实体的摄像机模块
        this->Entity.Pawn.CameraServices.Address = MRead<uintptr_t>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pCameraServices);
        // 从摄像机服务获取fov
        this->Entity.Pawn.CameraServices.iFOV = MRead<uint32_t>(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV);
        this->Entity.Pawn.CameraServices.LastShotFOV = MRead<float>(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flLastShotFOV);
        this->Entity.Pawn.CameraServices.FOVStart = MRead<uint32_t>(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOVStart);
        this->Entity.Pawn.CameraServices.FOVRate = MRead<float>(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flFOVRate);
        this->Entity.Pawn.CameraServices.FOVTime = MRead<GameTime_t>(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flFOVTime);
    }
    // 其它
    {
        this->Entity.Pawn.m_iTeamNum = -10;
        this->Entity.Pawn.m_iTeamNum = MRead<int>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (this->Entity.Pawn.m_iTeamNum < 0)return 4001;
    }


    // 从本地实体获取GameSecneNode
    this->Entity.Pawn.GameSceneNode.Address = MRead<uintptr_t>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    // 获取本地坐标
    this->Entity.Pawn.m_vOldOrigin = MRead<DirectX::XMFLOAT3>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
    this->Entity.Pawn.m_iHideHUD = MRead<uint32_t>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_iHideHUD);
    this->PositionA = reinterpret_cast<DirectX::XMFLOAT3*>(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);
    this->PositionB = reinterpret_cast<DirectX::XMFLOAT3*>(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecWrappedLocalOrigin);
    this->Entity.Pawn.GameSceneNode.Position = MRead<DirectX::XMFLOAT3>(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);

    this->Entity.Pawn.m_pObserverServices.Address = MRead<uintptr_t>(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pObserverServices);
    if (this->Entity.Pawn.m_pObserverServices.Address) {
        this->Entity.Pawn.m_pObserverServices.m_iObserverMode = MRead<uint8_t>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode);
        this->Entity.Pawn.m_pObserverServices.m_hObserverTarget = MRead<uint32_t>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_hObserverTarget);
        this->Entity.Pawn.m_pObserverServices.m_iObserverLastMode = MRead<ObserverMode_t>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverLastMode);
        this->Entity.Pawn.m_pObserverServices.m_bForcedObserverMode = MRead<bool>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_bForcedObserverMode);
        this->Entity.Pawn.m_pObserverServices.m_flObserverChaseDistance = MRead<float>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistance);
        this->Entity.Pawn.m_pObserverServices.m_flObserverChaseDistanceCalcTime = MRead<float>(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistanceCalcTime);
    }
    return 0;
}