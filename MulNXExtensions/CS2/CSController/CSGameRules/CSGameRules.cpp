#include "CSGameRules.hpp"

#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

using namespace MulNX::Memory::ReadWrite;

void C_CSGameRules::Update() {
    std::unique_lock lock(this->GameRulesMtx);
    if (!this->Address)return;
    this->m_bFreezePeriod = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bFreezePeriod);
    this->m_bWarmupPeriod = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bWarmupPeriod);
    this->m_fWarmupPeriodEnd = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodEnd);
    this->m_fWarmupPeriodStart = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodStart);
    this->m_bTerroristTimeOutActive = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTerroristTimeOutActive);
    this->m_bCTTimeOutActive = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTTimeOutActive);
    this->m_flTerroristTimeOutRemaining = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flTerroristTimeOutRemaining);
    this->m_flCTTimeOutRemaining = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCTTimeOutRemaining);
    this->m_nTerroristTimeOuts = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTerroristTimeOuts);
    this->m_nCTTimeOuts = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nCTTimeOuts);
    this->m_bTechnicalTimeOut = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTechnicalTimeOut);
    this->m_bMatchWaitingForResume = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMatchWaitingForResume);
    this->m_iRoundTime = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundTime);
    this->m_fMatchStartTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fMatchStartTime);
    this->m_fRoundStartTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fRoundStartTime);
    this->m_flRestartRoundTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flRestartRoundTime);
    this->m_bGameRestart = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bGameRestart);
    this->m_flGameStartTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flGameStartTime);
    this->m_timeUntilNextPhaseStarts = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_timeUntilNextPhaseStarts);
    this->m_gamePhase = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_gamePhase);
    this->m_totalRoundsPlayed = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_totalRoundsPlayed);
    this->m_nRoundsPlayedThisPhase = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundsPlayedThisPhase);
    this->m_nOvertimePlaying = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nOvertimePlaying);
    this->m_iHostagesRemaining = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iHostagesRemaining);
    this->m_bAnyHostageReached = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bAnyHostageReached);
    this->m_bMapHasBombTarget = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBombTarget);
    this->m_bMapHasRescueZone = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasRescueZone);
    this->m_bMapHasBuyZone = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bMapHasBuyZone);
    this->m_bIsQueuedMatchmaking = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQueuedMatchmaking);
    this->m_nQueuedMatchmakingMode = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nQueuedMatchmakingMode);
    this->m_bIsValveDS = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsValveDS);
    this->m_bLogoMap = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bLogoMap);
    this->m_bPlayAllStepSoundsOnServer = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bPlayAllStepSoundsOnServer);
    this->m_iSpectatorSlotCount = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iSpectatorSlotCount);
    this->m_MatchDevice = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_MatchDevice);
    this->m_bHasMatchStarted = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bHasMatchStarted);
    this->m_nNextMapInMapgroup = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nNextMapInMapgroup);
    // 字符串类型需要特殊处理（此处省略具体实现）
    this->m_nTournamentPredictionsPct = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nTournamentPredictionsPct);
    this->m_flCMMItemDropRevealStartTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealStartTime);
    this->m_flCMMItemDropRevealEndTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealEndTime);
    this->m_bIsDroppingItems = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsDroppingItems);
    this->m_bIsQuestEligible = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsQuestEligible);
    this->m_bIsHltvActive = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bIsHltvActive);
    // 数组类型需要循环读取（此处省略具体实现）
    this->m_numBestOfMaps = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_numBestOfMaps);
    this->m_nHalloweenMaskListSeed = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nHalloweenMaskListSeed);
    this->m_bBombDropped = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombDropped);
    this->m_bBombPlanted = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bBombPlanted);
    this->m_iRoundWinStatus = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundWinStatus);
    this->m_eRoundWinReason = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_eRoundWinReason);
    this->m_bTCantBuy = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bTCantBuy);
    this->m_bCTCantBuy = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bCTCantBuy);
    // 数组类型需要循环读取（此处省略具体实现）
    this->m_iRoundEndTimerTime = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndTimerTime);
    // 字符串和特殊类型需要特殊处理（此处省略具体实现）
    this->m_iRoundEndPlayerCount = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndPlayerCount);
    this->m_bRoundEndNoMusic = MRead<bool>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_bRoundEndNoMusic);
    this->m_iRoundEndLegacy = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundEndLegacy);
    this->m_nRoundEndCount = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundEndCount);
    this->m_iRoundStartRoundNumber = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundStartRoundNumber);
    this->m_nRoundStartCount = MRead<int>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount);
    this->m_flLastPerfSampleTime = MRead<float>(this->Address + cs2_dumper::schemas::client_dll::C_CSGameRules::m_flLastPerfSampleTime);

    return;
}