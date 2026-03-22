#include "PlantedC4.hpp"

#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

using namespace MulNX::Memory::ReadWrite;

void C_PlantedC4::Update() {
    if (!this->Address)return;
    this->m_bBombTicking = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBombTicking);
    this->m_nBombSite = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_nBombSite);
    this->m_flC4Blow = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flC4Blow);
    this->m_bHasExploded = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bHasExploded);
    this->m_flTimerLength = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flTimerLength);
    this->m_bBeingDefused = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBeingDefused);
    this->m_flDefuseLength = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flDefuseLength);
    this->m_flDefuseCountDown = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_flDefuseCountDown);
    this->m_bBombDefused = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_bBombDefused);
    this->m_hBombDefuser = MRead<uint32_t>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_hBombDefuser);
    this->m_fLastDefuseTime = MRead<GameTime_t>(this->Address + cs2_dumper::schemas::client_dll::C_PlantedC4::m_fLastDefuseTime);

    return;
}