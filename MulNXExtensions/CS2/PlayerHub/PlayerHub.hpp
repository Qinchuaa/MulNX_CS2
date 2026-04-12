#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
public:
    std::atomic<Steam64UID> currentSteamId;
    
    bool Init()override;
    bool Window(MulNXUINode* node);
};