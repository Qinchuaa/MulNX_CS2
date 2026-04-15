#include "ProjectileTracker.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

void ProjectileTracker::Menu(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("投掷物追踪器", this->ShowWindow);
    if(!w) return;
    std::unique_lock lock(this->smutex);
    ImGui::TextUnformatted(std::format("追踪到的投掷物数量: {}", this->trackedProjectiles.size()).c_str());
    ImGui::Separator();
    for (const auto& [pGrenade, info] : this->trackedProjectiles) {
        try {
            auto name = MulNX::Memory::ReadString(info.controller->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("投掷物指针: 0x{:X}, 所属玩家: {}", reinterpret_cast<uintptr_t>(pGrenade), std::move(name)).c_str());
        }
        catch (const std::exception& e) {
            this->ISys().LogWarning(std::format("在读取投掷物信息时发生错误: {}", e.what()));
        }
        catch (...) {
            this->ISys().LogWarning("在读取投掷物信息时发生未知错误");
        }
    }
}

bool ProjectileTracker::Init() {
    this->CS2()->handlesControlPlayer.push_back(
        [this](CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
            std::unique_lock uniqueLock(this->smutex);
            this->HandleFindProjectile(controller, pawn);
            return true;
        });

    this->SendTask("CS2控制线程", [this]() {
        std::unique_lock lock(this->smutex);
        this->HandleUpdate();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {
        this->Menu(node);
        });

    return true;
}

void ProjectileTracker::ProcessMsg(MulNX::Message& Msg) {
    // 处理消息
}

void ProjectileTracker::HandleFindProjectile(CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
    try {
        auto pWeaponServices = MulNX::MRead(pawn->pWeaponServices());
        if (!pWeaponServices) return;
        auto hActiveWeapon = MulNX::MRead(pWeaponServices->hActiveWeapon());
        auto pGrenade = this->CS2()->Modules.client.GetBaseEntityFromHandle(hActiveWeapon)->As<CS2::C_BaseCSGrenadeProjectile>();
        if (!pGrenade) return;

        ProjectileInfo info;
        info.createTime = MulNX::MRead(pGrenade->m_flCreateTime());
        info.controller = controller;

        std::string weaponName = pGrenade->GetName();
        if (weaponName.find("grenade") != std::string::npos) {
            if (info.createTime != MulNX::MRead(pGrenade->m_flCreateTime()))return;
            auto it = this->trackedProjectiles.find(pGrenade);
            if (it == this->trackedProjectiles.end()) {
                this->trackedProjectiles[pGrenade] = info;
                this->ISys().LogInfo(std::format("追踪到新投掷物: {} (Handle: 0x{:X})", weaponName, hActiveWeapon.handle));
            }
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在更新投掷物追踪器时发生错误: {}", e.what()));
    }
    catch (...) {
        this->ISys().LogWarning("在更新投掷物追踪器时发生未知错误");
    }
}

void ProjectileTracker::CheckAndDelete() {
    for (auto it = this->trackedProjectiles.begin(); it != this->trackedProjectiles.end();) {
        auto pGrenade = it->first;

        auto needDelete = [&pGrenade,&it]()->bool {
            auto name = pGrenade->GetName();
            // 先检查还是不是投掷物
            if (name.find("grenade") == std::string::npos) {
                return true;
            }
            // 再检查创建时间是否匹配
            if (MulNX::MRead(pGrenade->m_flCreateTime()) != it->second.createTime) {
                return true;
            }
            return false;
            };
        
        auto flag = needDelete();

        if (flag) {
            this->ISys().LogInfo(std::format("投掷物已销毁: 0x{:X}", reinterpret_cast<uintptr_t>(pGrenade)));
            //it = this->trackedProjectiles.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ProjectileTracker::HandleUpdate() {
    this->CheckAndDelete();
}