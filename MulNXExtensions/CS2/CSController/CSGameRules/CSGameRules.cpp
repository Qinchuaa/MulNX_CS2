#include "CSGameRules.hpp"

#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

void C_CSGameRules::Update() {
    std::unique_lock lock(this->GameRulesMtx);
    if (!this->Address)return;
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bFreezePeriod, this->m_bFreezePeriod);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bWarmupPeriod, this->m_bWarmupPeriod);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodEnd, this->m_fWarmupPeriodEnd);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodStart, this->m_fWarmupPeriodStart);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTerroristTimeOutActive, this->m_bTerroristTimeOutActive);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTTimeOutActive, this->m_bCTTimeOutActive);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flTerroristTimeOutRemaining, this->m_flTerroristTimeOutRemaining);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCTTimeOutRemaining, this->m_flCTTimeOutRemaining);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTerroristTimeOuts, this->m_nTerroristTimeOuts);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nCTTimeOuts, this->m_nCTTimeOuts);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTechnicalTimeOut, this->m_bTechnicalTimeOut);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMatchWaitingForResume, this->m_bMatchWaitingForResume);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundTime, this->m_iRoundTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fMatchStartTime, this->m_fMatchStartTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fRoundStartTime, this->m_fRoundStartTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flRestartRoundTime, this->m_flRestartRoundTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bGameRestart, this->m_bGameRestart);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flGameStartTime, this->m_flGameStartTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_timeUntilNextPhaseStarts, this->m_timeUntilNextPhaseStarts);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_gamePhase, this->m_gamePhase);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_totalRoundsPlayed, this->m_totalRoundsPlayed);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundsPlayedThisPhase, this->m_nRoundsPlayedThisPhase);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nOvertimePlaying, this->m_nOvertimePlaying);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iHostagesRemaining, this->m_iHostagesRemaining);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bAnyHostageReached, this->m_bAnyHostageReached);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBombTarget, this->m_bMapHasBombTarget);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasRescueZone, this->m_bMapHasRescueZone);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBuyZone, this->m_bMapHasBuyZone);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQueuedMatchmaking, this->m_bIsQueuedMatchmaking);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nQueuedMatchmakingMode, this->m_nQueuedMatchmakingMode);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsValveDS, this->m_bIsValveDS);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bLogoMap, this->m_bLogoMap);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bPlayAllStepSoundsOnServer, this->m_bPlayAllStepSoundsOnServer);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iSpectatorSlotCount, this->m_iSpectatorSlotCount);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_MatchDevice, this->m_MatchDevice);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bHasMatchStarted, this->m_bHasMatchStarted);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nNextMapInMapgroup, this->m_nNextMapInMapgroup);
    // 字符串类型需要特殊处理（此处省略具体实现）
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTournamentPredictionsPct, this->m_nTournamentPredictionsPct);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealStartTime, this->m_flCMMItemDropRevealStartTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealEndTime, this->m_flCMMItemDropRevealEndTime);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsDroppingItems, this->m_bIsDroppingItems);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQuestEligible, this->m_bIsQuestEligible);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsHltvActive, this->m_bIsHltvActive);
    // 数组类型需要循环读取（此处省略具体实现）
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_numBestOfMaps, this->m_numBestOfMaps);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nHalloweenMaskListSeed, this->m_nHalloweenMaskListSeed);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombDropped, this->m_bBombDropped);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombPlanted, this->m_bBombPlanted);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundWinStatus, this->m_iRoundWinStatus);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_eRoundWinReason, this->m_eRoundWinReason);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTCantBuy, this->m_bTCantBuy);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTCantBuy, this->m_bCTCantBuy);
    // 数组类型需要循环读取（此处省略具体实现）
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndTimerTime, this->m_iRoundEndTimerTime);
    // 字符串和特殊类型需要特殊处理（此处省略具体实现）
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndPlayerCount, this->m_iRoundEndPlayerCount);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bRoundEndNoMusic, this->m_bRoundEndNoMusic);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndLegacy, this->m_iRoundEndLegacy);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundEndCount, this->m_nRoundEndCount);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundStartRoundNumber, this->m_iRoundStartRoundNumber);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount, this->m_nRoundStartCount);
    MulNX::Base::Memory::Read(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flLastPerfSampleTime, this->m_flLastPerfSampleTime);

    return;
}