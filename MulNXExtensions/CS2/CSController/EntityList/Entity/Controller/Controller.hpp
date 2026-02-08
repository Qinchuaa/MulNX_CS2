#pragma once

#include <MulNX/MulNX.hpp>

class C_PlayerController {
public:
    std::ostringstream GetMsg()const;

    uintptr_t Address;
    uint32_t hPawn;
    char m_iszPlayerName[128];
    uint32_t m_ipDesiredFOV;
};