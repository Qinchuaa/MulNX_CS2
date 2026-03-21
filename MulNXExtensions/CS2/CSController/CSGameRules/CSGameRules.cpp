#include "CSGameRules.hpp"

#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

void C_CSGameRules::Update() {
    std::unique_lock lock(this->GameRulesMtx);
    if (!this->Address)return;
    this->m_bFreezePeriod = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bFreezePeriod);
    this->m_bWarmupPeriod = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bWarmupPeriod);
    this->m_fWarmupPeriodEnd = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodEnd);
    this->m_fWarmupPeriodStart = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodStart);
    this->m_bTerroristTimeOutActive = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTerroristTimeOutActive);
    this->m_bCTTimeOutActive = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTTimeOutActive);
    this->m_flTerroristTimeOutRemaining = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flTerroristTimeOutRemaining);
    this->m_flCTTimeOutRemaining = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCTTimeOutRemaining);
    this->m_nTerroristTimeOuts = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTerroristTimeOuts);
    this->m_nCTTimeOuts = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nCTTimeOuts);
    this->m_bTechnicalTimeOut = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTechnicalTimeOut);
    this->m_bMatchWaitingForResume = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMatchWaitingForResume);
    this->m_iRoundTime = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundTime);
    this->m_fMatchStartTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fMatchStartTime);
    this->m_fRoundStartTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fRoundStartTime);
    this->m_flRestartRoundTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flRestartRoundTime);
    this->m_bGameRestart = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bGameRestart);
    this->m_flGameStartTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flGameStartTime);
    this->m_timeUntilNextPhaseStarts = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_timeUntilNextPhaseStarts);
    this->m_gamePhase = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_gamePhase);
    this->m_totalRoundsPlayed = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_totalRoundsPlayed);
    this->m_nRoundsPlayedThisPhase = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundsPlayedThisPhase);
    this->m_nOvertimePlaying = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nOvertimePlaying);
    this->m_iHostagesRemaining = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iHostagesRemaining);
    this->m_bAnyHostageReached = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bAnyHostageReached);
    this->m_bMapHasBombTarget = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBombTarget);
    this->m_bMapHasRescueZone = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasRescueZone);
    this->m_bMapHasBuyZone = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBuyZone);
    this->m_bIsQueuedMatchmaking = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQueuedMatchmaking);
    this->m_nQueuedMatchmakingMode = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nQueuedMatchmakingMode);
    this->m_bIsValveDS = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsValveDS);
    this->m_bLogoMap = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bLogoMap);
    this->m_bPlayAllStepSoundsOnServer = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bPlayAllStepSoundsOnServer);
    this->m_iSpectatorSlotCount = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iSpectatorSlotCount);
    this->m_MatchDevice = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_MatchDevice);
    this->m_bHasMatchStarted = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bHasMatchStarted);
    this->m_nNextMapInMapgroup = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nNextMapInMapgroup);
    // 字符串类型需要特殊处理（此处省略具体实现）
    this->m_nTournamentPredictionsPct = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTournamentPredictionsPct);
    this->m_flCMMItemDropRevealStartTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealStartTime);
    this->m_flCMMItemDropRevealEndTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealEndTime);
    this->m_bIsDroppingItems = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsDroppingItems);
    this->m_bIsQuestEligible = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQuestEligible);
    this->m_bIsHltvActive = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsHltvActive);
    // 数组类型需要循环读取（此处省略具体实现）
    this->m_numBestOfMaps = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_numBestOfMaps);
    this->m_nHalloweenMaskListSeed = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nHalloweenMaskListSeed);
    this->m_bBombDropped = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombDropped);
    this->m_bBombPlanted = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombPlanted);
    this->m_iRoundWinStatus = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundWinStatus);
    this->m_eRoundWinReason = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_eRoundWinReason);
    this->m_bTCantBuy = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTCantBuy);
    this->m_bCTCantBuy = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTCantBuy);
    // 数组类型需要循环读取（此处省略具体实现）
    this->m_iRoundEndTimerTime = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndTimerTime);
    // 字符串和特殊类型需要特殊处理（此处省略具体实现）
    this->m_iRoundEndPlayerCount = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndPlayerCount);
    this->m_bRoundEndNoMusic = MulNX::Memory::Read<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bRoundEndNoMusic);
    this->m_iRoundEndLegacy = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndLegacy);
    this->m_nRoundEndCount = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundEndCount);
    this->m_iRoundStartRoundNumber = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundStartRoundNumber);
    this->m_nRoundStartCount = MulNX::Memory::Read<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount);
    this->m_flLastPerfSampleTime = MulNX::Memory::Read<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flLastPerfSampleTime);

    return;
}