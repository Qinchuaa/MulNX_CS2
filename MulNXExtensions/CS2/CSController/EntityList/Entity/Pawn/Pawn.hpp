#pragma once

#include "CameraServices/CameraServices.hpp"
#include "GameSceneNode/GameSceneNode.hpp"
#include "ObserverServices/ObserverServices.hpp"

class C_Pawn {
public:
    std::ostringstream GetMsg()const;

    uintptr_t Address{};
    C_CameraServices CameraServices{};
    C_GameSceneNode GameSceneNode{};
    C_ObserverServices m_pObserverServices{};
    int m_iTeamNum;
    int m_iHealth;
    uint32_t m_iHideHUD; // uint32

    DirectX::XMFLOAT3 m_vOldOrigin;
    DirectX::XMFLOAT3 m_angEyeAngles;

    DirectX::XMFLOAT3 GetOriginPos()const;
    DirectX::XMFLOAT3 GetEyePos()const;
};