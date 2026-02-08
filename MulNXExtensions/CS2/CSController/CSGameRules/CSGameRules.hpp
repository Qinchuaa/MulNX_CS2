#pragma once

#include <MulNX/MulNX.hpp>
#include <cstdint>
#include <shared_mutex>

class C_CSGameRules {
public:
    uintptr_t Address;
    inline static std::shared_mutex GameRulesMtx;

    bool m_bFreezePeriod; // bool
    bool m_bWarmupPeriod; // bool
    GameTime_t m_fWarmupPeriodEnd; // GameTime_t
    GameTime_t m_fWarmupPeriodStart; // GameTime_t
    bool m_bTerroristTimeOutActive; // bool
    bool m_bCTTimeOutActive; // bool
    float m_flTerroristTimeOutRemaining; // float32
    float m_flCTTimeOutRemaining; // float32
    int32_t m_nTerroristTimeOuts; // int32
    int32_t m_nCTTimeOuts; // int32
    bool m_bTechnicalTimeOut; // bool
    bool m_bMatchWaitingForResume; // bool
    int32_t m_iRoundTime; // int32
    float m_fMatchStartTime; // float32
    GameTime_t m_fRoundStartTime; // GameTime_t
    GameTime_t m_flRestartRoundTime; // GameTime_t
    bool m_bGameRestart; // bool
    float m_flGameStartTime; // float32
    float m_timeUntilNextPhaseStarts; // float32
    int32_t m_gamePhase; // int32
    int32_t m_totalRoundsPlayed; // int32
    int32_t m_nRoundsPlayedThisPhase; // int32
    int32_t m_nOvertimePlaying; // int32
    int32_t m_iHostagesRemaining; // int32
    bool m_bAnyHostageReached; // bool
    bool m_bMapHasBombTarget; // bool
    bool m_bMapHasRescueZone; // bool
    bool m_bMapHasBuyZone; // bool
    bool m_bIsQueuedMatchmaking; // bool
    int32_t m_nQueuedMatchmakingMode; // int32
    bool m_bIsValveDS; // bool
    bool m_bLogoMap; // bool
    bool m_bPlayAllStepSoundsOnServer; // bool
    int32_t m_iSpectatorSlotCount; // int32
    int32_t m_MatchDevice; // int32
    bool m_bHasMatchStarted; // bool
    int32_t m_nNextMapInMapgroup; // int32
    //char m_szTournamentEventName[512]; // char[512]
    //char m_szTournamentEventStage[512]; // char[512]
    //char m_szMatchStatTxt[512]; // char[512]
    //char m_szTournamentPredictionsTxt[512]; // char[512]
    int32_t m_nTournamentPredictionsPct; // int32
    GameTime_t m_flCMMItemDropRevealStartTime; // GameTime_t
    GameTime_t m_flCMMItemDropRevealEndTime; // GameTime_t
    bool m_bIsDroppingItems; // bool
    bool m_bIsQuestEligible; // bool
    bool m_bIsHltvActive; // bool
    //uint16_t m_arrProhibitedItemIndices[100]; // uint16[100]
    //uint32_t m_arrTournamentActiveCasterAccounts[4]; // uint32[4]
    int32_t m_numBestOfMaps; // int32
    int32_t m_nHalloweenMaskListSeed; // int32
    bool m_bBombDropped; // bool
    bool m_bBombPlanted; // bool
    int32_t m_iRoundWinStatus; // int32
    int32_t m_eRoundWinReason; // int32
    bool m_bTCantBuy; // bool
    bool m_bCTCantBuy; // bool
    //int32_t m_iMatchStats_RoundResults[30]; // int32[30]
    //int32_t m_iMatchStats_PlayersAlive_CT[30]; // int32[30]
    //int32_t m_iMatchStats_PlayersAlive_T[30]; // int32[30]
    //float m_TeamRespawnWaveTimes[32]; // float32[32]
    //GameTime_t m_flNextRespawnWave[32]; // GameTime_t[32]
    //Vector m_vMinimapMins; // Vector
    //Vector m_vMinimapMaxs; // Vector
    //float m_MinimapVerticalSectionHeights[8]; // float32[8]
    uint64_t m_ullLocalMatchID; // uint64
    //int32_t m_nEndMatchMapGroupVoteTypes[10]; // int32[10]
    //int32_t m_nEndMatchMapGroupVoteOptions[10]; // int32[10]
    int32_t m_nEndMatchMapVoteWinner; // int32
    int32_t m_iNumConsecutiveCTLoses; // int32
    int32_t m_iNumConsecutiveTerroristLoses; // int32
    int32_t m_nMatchAbortedEarlyReason; // int32
    bool m_bHasTriggeredRoundStartMusic; // bool
    bool m_bSwitchingTeamsAtRoundReset; // bool
    //class CCSGameModeRules* m_pGameModeRules; // CCSGameModeRules*
    //class C_RetakeGameRules m_RetakeRules; // C_RetakeGameRules
    uint8_t m_nMatchEndCount; // uint8
    int32_t m_nTTeamIntroVariant; // int32
    int32_t m_nCTTeamIntroVariant; // int32
    bool m_bTeamIntroPeriod; // bool
    int32_t m_iRoundEndWinnerTeam; // int32
    int32_t m_eRoundEndReason; // int32
    bool m_bRoundEndShowTimerDefend; // bool
    int32_t m_iRoundEndTimerTime; // int32
    //class CUtlString m_sRoundEndFunFactToken; // CUtlString
    //CPlayerSlot m_iRoundEndFunFactPlayerSlot; // CPlayerSlot
    int32_t m_iRoundEndFunFactData1; // int32
    int32_t m_iRoundEndFunFactData2; // int32
    int32_t m_iRoundEndFunFactData3; // int32
    //class CUtlString m_sRoundEndMessage; // CUtlString
    int32_t m_iRoundEndPlayerCount; // int32
    bool m_bRoundEndNoMusic; // bool
    int32_t m_iRoundEndLegacy; // int32
    uint8_t m_nRoundEndCount; // uint8
    int32_t m_iRoundStartRoundNumber; // int32
    uint8_t m_nRoundStartCount; // uint8
    double m_flLastPerfSampleTime; // float64

    void Update();
};