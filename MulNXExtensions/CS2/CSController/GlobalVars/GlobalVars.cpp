#include "GlobalVars.hpp"
#include <MulNXExtensions/WinExt/WinExt.hpp>

void C_GlobalVars::Update() {
    this->RealTime = MulNX::MRead<float>(this->Address + 0x00);
    this->CurrentTime = MulNX::MRead<float>(this->Address + 0x30);
    this->TickCount = MulNX::MRead<int>(this->Address + 0x44);

    return;
}
uintptr_t C_GlobalVars::GetCurrentTimePointer()const {
    return this->Address + 0x30;
}