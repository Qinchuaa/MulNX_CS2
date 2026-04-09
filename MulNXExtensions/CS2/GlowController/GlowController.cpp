#include "GlowController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

void GlowController::CheckMenu(Steam64UID uid) {
    auto itPlayer = this->playerColors.find(uid);
    if (itPlayer == this->playerColors.end()) {
        ImGui::Text("无颜色替换规则");
    }
    else {
        ImGui::ColorButton("##ColorPreview", ImGui::ColorConvertU32ToFloat4(itPlayer->second),
            ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip,
            ImVec2(20, 20));
    }
}

void GlowController::SetMenu(Steam64UID uid) {
    ImGui::Text("颜色修改");

    // 1. 获取当前为该玩家设置的颜色（若存在），否则使用默认白色
    uint32_t currentColorU32 = IM_COL32(255, 255, 255, 255); // 默认白色
    {
        // 注意：读取 playerColors 也需要加锁，因为它可能被其他线程访问
        std::shared_lock lock(this->Hub()->GetMutex());
        auto it = this->playerColors.find(uid);
        if (it != this->playerColors.end()) {
            currentColorU32 = it->second;
        }
    }

    // 2. 将 uint32_t 颜色转换为 ImVec4，以便使用 ImGui 颜色编辑器
    ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(currentColorU32);

    // 3. 显示颜色选择器
    //    使用 ColorEdit4 可以同时展示预览色块和数值，也可以只用 ColorPicker4
    if (ImGui::ColorEdit4("##PlayerGlowColor", (float*)&colorVec4,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
        // 颜色被修改后，转换回 uint32_t 并保存到 playerColors
        uint32_t newColorU32 = ImGui::ColorConvertFloat4ToU32(colorVec4);

        // 加锁写入
        std::unique_lock lock(this->Hub()->GetMutex());
        this->playerColors[uid] = newColorU32;
    }

    // 可选：添加一个重置按钮，删除该玩家的自定义颜色规则
    ImGui::SameLine();
    if (ImGui::Button("重置")) {
        std::unique_lock lock(this->Hub()->GetMutex());
        this->playerColors.erase(uid);
    }
}

bool GlowController::Init() {
    auto region = this->CS2()->Modules.client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::SetGlowColor);
    auto target = region.Data();
    this->hkSetGlowColor = MulNX::Memory::HookEx::Create(target, 0, false, [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
        this->MySetGlowColor(*ctx->P1<CS2::CGlowProperty*>(), ctx->P2<uint32_t>());
        return true;
        }).value();
    this->hkSetGlowColor->Attach();
    this->Hub()->ModulesAboutPlayer.push_back(this);

    return true;
}

void GlowController::MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color) {
    std::shared_lock lock(this->Hub()->GetMutex());
    try {
        auto pBaseModelEntity = pGlowProperty->GetOwner();
        auto name = MulNX::MRead(MulNX::MRead(pBaseModelEntity->pClassInfo())->pName());

        auto hController = MulNX::MRead(pBaseModelEntity->As<CS2::C_BasePlayerPawn>()->m_hController());
        auto pController = this->CS2()->Modules.client.GetBaseEntityFromHandle(hController)->As<CS2::CBasePlayerController>();
        if (!pController)return;
        Steam64UID uid = *pController->m_steamID();
        auto itPlayer = this->playerColors.find(uid);
        if (itPlayer != this->playerColors.end()) {
            *color = itPlayer->second;
            return;
        }
        auto team = *pController->iTeamNum();
        auto itTeam = this->teamColors.find(team);
        if (itTeam != this->teamColors.end()) {
            *color = itTeam->second;
            return;
        }
    }
    catch (...) {
        
    }

    return;
}