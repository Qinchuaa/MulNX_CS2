#include "PlayerHub.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool PlayerHub::Window(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("玩家信息管理", this->ShowWindow);
    if (!w) return false;
    try {
        std::shared_lock lock(this->smutex);
        
        // ---------- 玩家列表区域 ----------
        ImGui::TextUnformatted("检测到如下玩家信息：");
        static int showMax = 20;
        ImGui::SliderInt("搜索的最大数量", &showMax, 1, 255);

        int playerNum = 0;

        for (int i = 0; i <= std::min(this->CS2()->Modules.client.dwGameEntitySystem_highestEntityIndex(), showMax); ++i) {
            auto* baseEntity = this->CS2()->Modules.client.GetBaseEntity(i);
            if (!baseEntity) continue;

            auto* playerController = baseEntity->As<CS2::CCSPlayerController>();
            if (!playerController) continue;

            auto hPawn = MulNX::MRead(playerController->hPawn());
            auto* pawn = this->CS2()->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pawn) continue;

            uint64_t SteamID = MulNX::MRead(playerController->m_steamID());
            if (SteamID == 0) continue;

            ++playerNum;
            std::string displayName = std::format("玩家 {} (SteamID: {})", playerNum, SteamID);
            if (ImGui::Selectable(displayName.c_str(), this->currentSteamId.load(std::memory_order_acquire) == SteamID)) {
                this->currentSteamId.store(SteamID, std::memory_order_release);
                this->ShowCompanionWindow.store(true, std::memory_order_release);
            }

            auto naturalName = MulNX::Memory::ReadString(playerController->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("自然名字: {}", naturalName).c_str());
            ImGui::Separator();
        }
        auto pos=ImGui::GetWindowPos();
        auto size = ImGui::GetWindowSize();

        ImGui::SetNextWindowPos(ImVec2(pos.x + size.x, pos.y));
        MulNX::UI::RAIIWindow infoWindow("玩家信息", this->ShowCompanionWindow);
        if (!infoWindow) return true;
        ImGui::SeparatorText("进行修改");

        ImGui::Text("当前选中的 SteamID: %llu", this->currentSteamId.load(std::memory_order_acquire));
        if (this->currentSteamId.load(std::memory_order_acquire) == 0) {
            ImGui::TextDisabled("请先在列表中选择一名玩家");
            return true;
        }
        node->CallUINode("NameController");
        node->CallUINode("GlowController");
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在绘制玩家信息时捕获到异常：{}", e.what()));
    }
    return true;
}

bool PlayerHub::Init() {
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->Window(node);});
    return true;
}