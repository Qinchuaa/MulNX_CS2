#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/List/C_BaseEntity.hpp>

class CSController;
class PlayerFlashController final :public MulNX::ModuleBase {
private:
    CSController* CS = nullptr;
    std::atomic<bool>bForceNoFlash = false;
public:
    bool Init()override;
    bool Menu(MulNXUINode* node);
    bool HandleForceFlash(CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
};