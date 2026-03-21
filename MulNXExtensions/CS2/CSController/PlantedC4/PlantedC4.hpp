#pragma once

#include <MulNX/MulNX.hpp>

class C_PlantedC4 {
public:
    uintptr_t Address;

    bool m_bBombTicking; // bool
    int m_nBombSite; // int32
    //constexpr std::ptrdiff_t m_nSourceSoundscapeHash = 0x1168; // int32
    //constexpr std::ptrdiff_t m_entitySpottedState = 0x1170; // EntitySpottedState_t
    //constexpr std::ptrdiff_t m_flNextGlow = 0x1188; // GameTime_t
    //constexpr std::ptrdiff_t m_flNextBeep = 0x118C; // GameTime_t
    std::atomic<GameTime_t> m_flC4Blow; // GameTime_t
    //constexpr std::ptrdiff_t m_bCannotBeDefused = 0x1194; // bool
    bool m_bHasExploded; // bool
    float m_flTimerLength; // float32
    bool m_bBeingDefused; // bool
    //constexpr std::ptrdiff_t m_bTriggerWarning = 0x11A0; // float32
    //constexpr std::ptrdiff_t m_bExplodeWarning = 0x11A4; // float32
    //constexpr std::ptrdiff_t m_bC4Activated = 0x11A8; // bool
    //constexpr std::ptrdiff_t m_bTenSecWarning = 0x11A9; // bool
    float m_flDefuseLength; // float32
    GameTime_t m_flDefuseCountDown; // GameTime_t
    bool m_bBombDefused; // bool
    uint32_t m_hBombDefuser; // CHandle<C_CSPlayerPawn>
    //constexpr std::ptrdiff_t m_AttributeManager = 0x11C0; // C_AttributeContainer
    //constexpr std::ptrdiff_t m_hDefuserMultimeter = 0x1698; // CHandle<C_Multimeter>
    //constexpr std::ptrdiff_t m_flNextRadarFlashTime = 0x169C; // GameTime_t
    //constexpr std::ptrdiff_t m_bRadarFlash = 0x16A0; // bool
    //constexpr std::ptrdiff_t m_pBombDefuser = 0x16A4; // CHandle<C_CSPlayerPawn>
    GameTime_t m_fLastDefuseTime; // GameTime_t
    //constexpr std::ptrdiff_t m_pPredictionOwner = 0x16B0; // CBasePlayerController*
    //constexpr std::ptrdiff_t m_vecC4ExplodeSpectatePos = 0x16B8; // Vector
    //constexpr std::ptrdiff_t m_vecC4ExplodeSpectateAng = 0x16C4; // QAngle
    //constexpr std::ptrdiff_t m_flC4ExplodeSpectateDuration = 0x16D0; // float32

    void Update();
};