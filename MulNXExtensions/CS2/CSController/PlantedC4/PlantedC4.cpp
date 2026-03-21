#include "PlantedC4.hpp"

#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

void C_PlantedC4::Update() {
    if (!this->Address)return;
    this->m_bBombTicking = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBombTicking);
    this->m_nBombSite = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_nBombSite);
    this->m_flC4Blow = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flC4Blow);
    this->m_bHasExploded = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bHasExploded);
    this->m_flTimerLength = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flTimerLength);
    this->m_bBeingDefused = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBeingDefused);
    this->m_flDefuseLength = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flDefuseLength);
    this->m_flDefuseCountDown = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flDefuseCountDown);
    this->m_bBombDefused = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBombDefused);
    this->m_hBombDefuser = MulNX::Memory::Read<uint32_t>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_hBombDefuser);
    this->m_fLastDefuseTime = MulNX::Memory::Read<GameTime_t>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_fLastDefuseTime);

    return;
}