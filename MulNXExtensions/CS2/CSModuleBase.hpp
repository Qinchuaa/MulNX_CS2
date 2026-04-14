#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class CSController;
class PlayerHub;

using Steam64UID = uint64_t;

class CSModuleBase :public MulNX::ModuleBase {
public:
    CSController* CS2() { return reinterpret_cast<CSController*>(this->AL3D); }
    PlayerHub* Hub();
};