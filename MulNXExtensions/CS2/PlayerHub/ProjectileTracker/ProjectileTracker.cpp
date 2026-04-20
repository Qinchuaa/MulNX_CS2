#include "ProjectileTracker.hpp"

#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNX/Base/UI/UI.hpp>

static std::string GetControllerPlayerName(CS2::CCSPlayerController* pController) {
    if (!pController) return "未知玩家";
    auto namePtr = pController->m_iszPlayerName();
    if (!namePtr) return "未知玩家";
    auto name = MulNX::Memory::ReadString(namePtr);
    return name.empty() ? "未知玩家" : name;
}

void ProjectileTracker::Menu(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("投掷物追踪器", this->ShowWindow);
    if (!w) return;

    try {
        std::shared_lock lock(this->grenadeMutex);

        ImGui::TextUnformatted("已追踪的 grenade：");
        if (this->trackedGrenades.empty()) {
            ImGui::TextDisabled("当前暂无 grenade 记录");
            return;
        }

        int index = 0;
        for (const auto& [pGrenade, pController] : this->trackedGrenades) {
            if (!pGrenade || !pController) continue;

            auto grenadeName = pGrenade->GetName();
            auto playerName = GetControllerPlayerName(pController);
            auto steamId = *pController->m_steamID();
            auto label = std::format("[{}] {} - 玩家：{} (SteamID={})", index++, grenadeName, playerName, steamId);

            if (ImGui::Selectable(label.c_str(), this->SelectedGrenade == pGrenade)) {
                this->SelectedGrenade = pGrenade;
            }
            if (this->SelectedGrenade == pGrenade) {
                ImGui::TextUnformatted("已选中");
            }
            ImGui::Separator();
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在绘制投掷物追踪窗口时发生异常：{}", e.what()));
    }
}

bool ProjectileTracker::Init() {
    this->SendTask("CS2控制线程", [this]() {
        this->EntryProcessMsg();
        this->Update();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        this->Menu(node);
        });

    this->ISys()
        .SubscribeAsync("Game/Entity/Added")
        .SubscribeAsync("Game/Entity/Removed");

    return true;
}

void ProjectileTracker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/Entity/Added"_hash: {
        auto pEntity = msg.p1.as<CS2::C_BaseEntity*>();
        auto hEntity = msg.p2.low<CS2::CHandleBase>();
        std::string name;
        try {
            name = pEntity->GetName();
        }
        catch (const std::exception& e) {
            this->ISys().LogWarning("添加：在分析实体消息以分流时发生异常");
        }
        if (name.find("projectile") != std::string::npos) {
            this->HandleProjectileAdd(pEntity->As<CS2::C_BaseCSGrenadeProjectile>(), std::move(name));
        }
        else if (name.find("grenade") != std::string::npos) {
            this->HandleGrenadeAdd(pEntity->As<CS2::C_BaseCSGrenade>(), std::move(name));
        }
        break;
    }
    case "Game/Entity/Removed"_hash: {
        auto pEntity = msg.p1.as<CS2::C_BaseEntity*>();
        if (this->trackedProjectiles.find(pEntity->As<CS2::C_BaseCSGrenadeProjectile>()) != this->trackedProjectiles.end()) {
            this->HandleProjectileRemove(pEntity->As<CS2::C_BaseCSGrenadeProjectile>());
        }
        else if (this->trackedGrenades.find(pEntity->As<CS2::C_BaseCSGrenade>()) != this->trackedGrenades.end()) {
            this->HandleGrenadeRemove(pEntity->As<CS2::C_BaseCSGrenade>());
        }
        break;
    }
    }
}

void ProjectileTracker::HandleGrenadeAdd(CS2::C_BaseCSGrenade* pGrenade, std::string&& name) {
    try {
        auto hPawn = MulNX::MRead(pGrenade->m_hOwnerEntity());
        auto pPawn = this->CS2()->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        auto hController = MulNX::MRead(pPawn->m_hController());
        auto pController = this->CS2()->Modules.client.GetBaseEntityFromHandle(hController)->As<CS2::CCSPlayerController>();

        if (pController) {
            std::unique_lock lock(this->grenadeMutex);
            this->trackedGrenades[pGrenade] = pController;
            this->ISys().LogInfo(std::format("记录 grenade({}) -> 控制器 SteamID={} ", name, *pController->m_steamID()));
        }
        else {
            this->ISys().LogWarning(std::format("检测到 grenade({}) 但无法解析所属控制器", name));
        }
        return;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在分析新增实体时发生异常：{}", e.what()));
    }
}
void ProjectileTracker::HandleGrenadeRemove(CS2::C_BaseCSGrenade* pGrenade) {
    try {
        std::unique_lock lock(this->grenadeMutex);
        auto it = this->trackedGrenades.find(pGrenade);
        if (it != this->trackedGrenades.end()) {
            this->ISys().LogInfo(std::format("移除 grenade 实体，释放追踪记录 SteamID={} ", *it->second->m_steamID()));
            if (this->SelectedGrenade == pGrenade) {
                this->SelectedGrenade = nullptr;
            }
            this->trackedGrenades.erase(it);
        }
        return;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在处理实体移除时发生异常：{}", e.what()));
    }
}
void ProjectileTracker::HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile, std::string&& name) {
    try {
        auto hThrower = MulNX::MRead(pProjectile->m_hThrower());
        auto* pPawn = this->CS2()->Modules.client.GetBaseEntityFromHandle(hThrower)->As<CS2::C_CSPlayerPawn>();
        auto hController = MulNX::MRead(pPawn->m_hController());
        auto* pController = this->CS2()->Modules.client.GetBaseEntityFromHandle(hController)->As<CS2::CCSPlayerController>();
        if (!pController)return;
        std::unique_lock lock(this->grenadeMutex);
        this->trackedProjectiles[pProjectile] = pController;
        this->ISys().LogInfo(std::format("记录 projectile({}) -> 控制器 SteamID={} ", name, MulNX::MRead(pController->m_steamID())));

        auto it = this->trackedGrenades.find(this->SelectedGrenade.load(std::memory_order_acquire));
        if (it == this->trackedGrenades.end())return;

        if (pController == it->second) {
            this->pTargetWatchProjectile.store(pProjectile, std::memory_order_release);
        }

        return;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在分析新增实体时发生异常：{}", e.what()));
    }
}
void ProjectileTracker::HandleProjectileRemove(CS2::C_BaseCSGrenadeProjectile* pProjectile) {
    try {
        auto it = this->trackedProjectiles.find(pProjectile);
        if (it != this->trackedProjectiles.end()) {
            this->ISys().LogInfo(std::format("移除 projectile 实体，释放追踪记录 SteamID={} ", *it->second->m_steamID()));
            this->trackedProjectiles.erase(it);
        }
        if (pProjectile == this->pTargetWatchProjectile.load(std::memory_order_acquire)) {
            this->pTargetWatchProjectile.store(nullptr, std::memory_order_release);
        }
        return;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在处理实体移除时发生异常：{}", e.what()));
    }
}

void ProjectileTracker::Update() {
    try {
        auto pProjectile = this->pTargetWatchProjectile.load(std::memory_order_acquire);
        if (!pProjectile) return;

        auto pGameSceneNode = MulNX::MRead(pProjectile->pGameSceneNode());
        if (!pGameSceneNode) return;

        auto pos = MulNX::MRead(pGameSceneNode->vecOrigin());
        auto vel = MulNX::MRead(pProjectile->m_vecVelocity());

        // 相机后移距离（可根据投掷物尺寸调整）
        constexpr float kCameraOffsetDistance = 80.0f;

        DirectX::XMFLOAT3 viewRot{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 cameraPos = pos; // 默认相机位置为投掷物位置

        // 检查速度是否有效（非零向量）
        float speedSq = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
        const float kEpsilon = 1e-6f;
        if (speedSq > kEpsilon) {
            // 归一化速度方向
            float invLen = 1.0f / std::sqrt(speedSq);
            DirectX::XMFLOAT3 dir{ vel.x * invLen, vel.y * invLen, vel.z * invLen };

            // 将方向向量转换为欧拉角（pitch, yaw, roll）
            MulNX::Math::CSDirToEuler(dir, viewRot);

            // 后移相机：位置 = 投掷物位置 - 速度方向 * 距离
            cameraPos.x = pos.x - dir.x * kCameraOffsetDistance;
            cameraPos.y = pos.y - dir.y * kCameraOffsetDistance;
            cameraPos.z = pos.z - dir.z * kCameraOffsetDistance;
        }
        // 若速度为零，保持相机位置与投掷物重合，角度保持不变（可扩展为平滑过渡）

        auto newView = std::make_shared<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>(
            cameraPos, viewRot
        );
        this->currentView.store(newView, std::memory_order_release);
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在追踪投掷物时发生异常：{}", e.what()));
    }
}

std::optional<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>> ProjectileTracker::GetView() {
    if (!this->pTargetWatchProjectile.load(std::memory_order_acquire)) return std::nullopt;
    auto currentViewPtr = this->currentView.load(std::memory_order_acquire);
    if (!currentViewPtr) return std::nullopt;

    return *currentViewPtr;

}