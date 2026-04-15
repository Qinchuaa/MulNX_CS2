#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class ProjectileTracker final : public CSModuleBase {
    class ProjectileInfo {
    public:
        float createTime = 0;
        CS2::CCSPlayerController* controller = nullptr;
    };
    std::map<CS2::C_BaseCSGrenadeProjectile*, ProjectileInfo> trackedProjectiles;
public:
    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
    void Menu(MulNXUINode* node);
    void HandleFindProjectile(CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn);
    void HandleUpdate();
    void CheckAndDelete();
};