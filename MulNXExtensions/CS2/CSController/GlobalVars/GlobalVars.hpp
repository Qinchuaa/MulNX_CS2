#pragma once

#include <cstdint>
#include <MulNX/MulNX.hpp>

class C_GlobalVars {
public:
    // uintptr_t Address;

    // float RealTime{};
    // int FrameCount{};
    // int MaxClients{};
    // float IntervalPerTick{};
    // float CurrentTime{};
    // int TickCount{};
    // float IntervalPerTick2{};
    // char* CurrentMapName = nullptr;

    float* fRealTime() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x00); }
    float* fCurrentTime() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x30); }
    int* iTickCount() {return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x44); }
};