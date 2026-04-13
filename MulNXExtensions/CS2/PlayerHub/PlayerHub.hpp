#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
    std::atomic<bool> ShowCompanionWindow = false;
public:
    enum class View :uint8_t {
        Player,
        Team
    };
    std::atomic<View> showView;
    std::atomic<Steam64UID> currentSteamId;
    std::atomic<CS2::ui8TeamNum> currentTeam;

    bool Init()override;
    bool Window(MulNXUINode* node);
};