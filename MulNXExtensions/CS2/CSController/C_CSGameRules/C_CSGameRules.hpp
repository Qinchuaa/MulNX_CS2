#pragma once

#include <MulNXThirdParty/All_cs2_dumper.hpp>

namespace CS2 {
    class C_CSGameRules {
    public:
        GameTime_t* fWarmupPeriodEnd() { return reinterpret_cast<GameTime_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodEnd); }
        uint8_t* nRoundStartCount() { return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount); }
    };
}