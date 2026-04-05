#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/List/C_BaseEntity.hpp>

class CSController;
class PlayerHub final :public MulNX::ModuleBase {
private:
    CSController* CS = nullptr;
    std::array<char[128], 64>nameReplace{};
    std::map<uint64_t, int>nameReplaceInfo{};
    std::unique_ptr<MulNX::Memory::HookEx>hkGetDecoratedPlayerName = nullptr;
    std::unique_ptr<MulNX::Memory::HookEx>hkGetPlayerName = nullptr;
    bool bGetPlayerNameHooked = false;
    void HandleVHook(CS2::CCSPlayerController* pPlayerController);
public:
    bool Init()override;
    bool Window(MulNXUINode* node);
};