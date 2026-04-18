#pragma once

#include <shared_mutex>
#include <memory>
#include <atomic>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

class ProjectileTracker final : public CSModuleBase {
    std::map<CS2::C_BaseCSGrenade*, CS2::CCSPlayerController*> trackedGrenades;
    std::map<CS2::C_BaseCSGrenadeProjectile*, CS2::CCSPlayerController*> trackedProjectiles;
    std::shared_mutex grenadeMutex;
    std::atomic<CS2::C_BaseCSGrenade*> SelectedGrenade = nullptr;
    std::atomic<CS2::C_BaseCSGrenadeProjectile*> pTargetWatchProjectile = nullptr;

    void HandleGrenadeAdd(CS2::C_BaseCSGrenade* pGrenade, std::string&& name);
    void HandleGrenadeRemove(CS2::C_BaseCSGrenade* pGrenade);

    void HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile, std::string&& name);
    void HandleProjectileRemove(CS2::C_BaseCSGrenadeProjectile* pProjectile);

    void ProcessMsg(MulNX::Message& msg)override;
    void Menu(MulNX::UINode* node);
    void Update();
    std::atomic<std::shared_ptr<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>> currentView = nullptr;
public:
    bool Init()override;
    std::optional<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>> GetView();
};