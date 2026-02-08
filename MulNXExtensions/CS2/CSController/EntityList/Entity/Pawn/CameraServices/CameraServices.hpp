#pragma once

#include <MulNX/MulNX.hpp>

class C_CameraServices {
public:
    std::ostringstream GetMsg()const;

    uintptr_t Address{};
    uint32_t FOVStart; // uint32
    GameTime_t FOVTime; // GameTime_t
    float FOVRate; // float32
    //constexpr std::ptrdiff_t m_hZoomOwner = 0x298; // CHandle<C_BaseEntity>
    float LastShotFOV; // float32
    uint32_t iFOV{};
};