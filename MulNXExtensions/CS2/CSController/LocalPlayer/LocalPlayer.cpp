#include "LocalPlayer.hpp"

#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNX/ThirdParty/All_cs2_dumper.hpp>

std::ostringstream C_LocalPlayer::GetMsg() {
    std::shared_lock lock(this->LocalPlayerMutex);
    std::ostringstream oss;
    oss << "   本地玩家信息:" << this->Entity.GetMsg().str()
        << "   HUD信息： " << this->Entity.Pawn.m_iHideHUD;
    return oss;
}

MulNX::Base::Math::SpatialState C_LocalPlayer::GetSpatialState() {
    std::shared_lock lock(this->LocalPlayerMutex);

    DirectX::XMFLOAT3 pos = this->Entity.Pawn.GetEyePos();
    MulNX::Base::Math::SpatialState temp;
    temp.PositionAndFOV = DirectX::XMVectorSet(
        pos.x,
        pos.y,
        pos.z,
        *this->pGlobalFOV
    );

    DirectX::XMVECTOR quatVec = MulNX::Base::Math::CSEulerToQuatVec(*this->ViewAngles);
    temp.RotationQuat = quatVec;

    return temp;
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

void C_LocalPlayer::SetFOV(float FOV) {
    *this->pGlobalFOV = FOV;
}

int C_LocalPlayer::Update() {
    std::unique_lock lock(this->LocalPlayerMutex);
    //判断本地控制器是否可用
    if (!this->Entity.Controller.Address) {
        return 1003;
    }
    {
        MulNX::Base::Memory::ReadString(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName,
            this->Entity.Controller.m_iszPlayerName,
            sizeof(this->Entity.Controller.m_iszPlayerName));
        //获取本地控制器的期望FOV
        MulNX::Base::Memory::Read(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iDesiredFOV,
            this->Entity.Controller.m_ipDesiredFOV);
    }
    //获取本地实体句柄
    MulNX::Base::Memory::Read(this->Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn, this->Entity.Controller.hPawn);
    if (this->Entity.Controller.hPawn == 0xFFFFFFFF)return 2001;
    //获取本地实体
    this->Entity.Pawn.Address = C_EntityList::GetEntityPawnFromHandle(this->Entity.Controller.hPawn);
    if (!this->Entity.Pawn.Address)return 3001;
    //摄像机模块
    {
        //获取本地实体的摄像机模块
        MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pCameraServices, this->Entity.Pawn.CameraServices.Address);
        //从摄像机服务获取fov
        MulNX::Base::Memory::Read(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV, this->Entity.Pawn.CameraServices.iFOV);
        MulNX::Base::Memory::Read(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flLastShotFOV, this->Entity.Pawn.CameraServices.LastShotFOV);
        MulNX::Base::Memory::Read(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOVStart, this->Entity.Pawn.CameraServices.FOVStart);
        MulNX::Base::Memory::Read(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flFOVRate, this->Entity.Pawn.CameraServices.FOVRate);
        MulNX::Base::Memory::Read(this->Entity.Pawn.CameraServices.Address + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flFOVTime, this->Entity.Pawn.CameraServices.FOVTime);
    }
    //其它
    {
        this->Entity.Pawn.m_iTeamNum = -10;
        MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum, this->Entity.Pawn.m_iTeamNum);
        if (this->Entity.Pawn.m_iTeamNum < 0)return 4001;
    }


    //从本地实体获取GameSecneNode
    MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode, this->Entity.Pawn.GameSceneNode.Address);
    //获取本地坐标
    MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin, this->Entity.Pawn.m_vOldOrigin);
    MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_iHideHUD, this->Entity.Pawn.m_iHideHUD);
    this->PositionA = reinterpret_cast<DirectX::XMFLOAT3*>(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);
    this->PositionB = reinterpret_cast<DirectX::XMFLOAT3*>(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecWrappedLocalOrigin);
    MulNX::Base::Memory::Read(this->Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin, this->Entity.Pawn.GameSceneNode.Position);

    MulNX::Base::Memory::Read(this->Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pObserverServices, this->Entity.Pawn.m_pObserverServices.Address);
    if (this->Entity.Pawn.m_pObserverServices.Address) {
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode, this->Entity.Pawn.m_pObserverServices.m_iObserverMode);
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_hObserverTarget, this->Entity.Pawn.m_pObserverServices.m_hObserverTarget);
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverLastMode, this->Entity.Pawn.m_pObserverServices.m_iObserverLastMode);
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_bForcedObserverMode, this->Entity.Pawn.m_pObserverServices.m_bForcedObserverMode);
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistance, this->Entity.Pawn.m_pObserverServices.m_flObserverChaseDistance);
        MulNX::Base::Memory::Read(this->Entity.Pawn.m_pObserverServices.Address + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistanceCalcTime, this->Entity.Pawn.m_pObserverServices.m_flObserverChaseDistanceCalcTime);
    }
    return 0;
}