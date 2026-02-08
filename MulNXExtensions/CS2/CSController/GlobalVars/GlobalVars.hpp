#pragma once

#include <MulNX/MulNX.hpp>

class C_GlobalVars {
public:
    uintptr_t Address;

    float RealTime{};
    int FrameCount{};
    int MaxClients{};
    float IntervalPerTick{};
    float CurrentTime{};
    int TickCount{};
    float IntervalPerTick2{};
    char* CurrentMapName = nullptr;

    void Update();

    uintptr_t GetCurrentTimePointer();
};