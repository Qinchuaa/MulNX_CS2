#include "GlobalVars.hpp"
#include <MulNXExtensions/WinExt/WinExt.hpp>

using namespace MulNX::Memory::ReadWrite;

void C_GlobalVars::Update() {
    this->RealTime = MRead<float>(this->Address + 0x00);
    this->CurrentTime = MRead<float>(this->Address + 0x30);
    this->TickCount = MRead<int>(this->Address + 0x44);

    return;
}
uintptr_t C_GlobalVars::GetCurrentTimePointer()const {
    return this->Address + 0x30;
}