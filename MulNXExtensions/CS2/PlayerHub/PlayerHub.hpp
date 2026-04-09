#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
private:
    std::array<char[128], 64>nameReplace{};
    std::map<uint64_t, int>nameReplaceInfo{};
    std::unique_ptr<MulNX::Memory::HookEx>hkGetDecoratedPlayerName = nullptr;
    std::unique_ptr<MulNX::Memory::HookEx>hkGetPlayerName = nullptr;
    bool bGetPlayerNameHooked = false;
    void HandleVHook(CS2::CCSPlayerController* pPlayerController);
public:
    std::vector<CSModuleBase*> ModulesAboutPlayer{};
    
    bool Init()override;
    bool Window(MulNXUINode* node);
};