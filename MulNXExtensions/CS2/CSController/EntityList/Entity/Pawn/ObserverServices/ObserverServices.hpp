#pragma once

#include <MulNX/MulNX.hpp>

typedef uint32_t ObserverMode_t;

class C_ObserverServices {
public:
    uintptr_t Address{};

    uint8_t m_iObserverMode; // uint8
    uintptr_t m_hObserverTarget; // CHandle<C_BaseEntity>
    ObserverMode_t m_iObserverLastMode; // ObserverMode_t
    bool m_bForcedObserverMode; // bool
    float m_flObserverChaseDistance; // float32
    GameTime_t m_flObserverChaseDistanceCalcTime; // GameTime_t
};