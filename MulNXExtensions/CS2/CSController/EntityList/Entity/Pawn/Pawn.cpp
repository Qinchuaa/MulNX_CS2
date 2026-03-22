#include"Pawn.hpp"
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

using namespace MulNX::Memory::ReadWrite;

std::ostringstream C_Pawn::GetMsg()const {
    std::ostringstream oss;
    //oss << this->GameSceneNode.GetMsg().str() << " 阵营:" << this->m_iTeamNum;
    oss << "   坐标: X:" << this->m_vOldOrigin.x << "   Y:" << this->m_vOldOrigin.y << "   Z:" << this->m_vOldOrigin.z
        << "   角度: X:" << this->m_angEyeAngles.x << "   Y:" << this->m_angEyeAngles.y << "   Z:" << this->m_angEyeAngles.z
        << "   阵营:" << this->m_iTeamNum << "   血量：" << this->m_iHealth
        << "   HUD: " << this->m_iHideHUD;
    return oss;
}
DirectX::XMFLOAT3 C_Pawn::GetOriginPos()const {
    return this->GameSceneNode.Position;
}
DirectX::XMFLOAT3 C_Pawn::GetEyePos()const {
    auto ViewOffset = MRead<DirectX::XMFLOAT3>(this->Address + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);
    return this->GetOriginPos() + ViewOffset;
}