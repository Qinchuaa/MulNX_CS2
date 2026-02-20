#include "GlobalVars.hpp"
#include <MulNXExtensions/WinExt/WinExt.hpp>

void C_GlobalVars::Update() {
    MulNX::Memory::Read(this->Address + 0x00, this->RealTime);
    MulNX::Memory::Read(this->Address + 0x30, this->CurrentTime);
    MulNX::Memory::Read(this->Address + 0x44, this->TickCount);

    return;
}
uintptr_t C_GlobalVars::GetCurrentTimePointer() {
    return this->Address + 0x30;
}