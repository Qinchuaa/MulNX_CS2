#include "NameController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

using GetDecoratedPlayerName_t = const char* (*)(CS2::CCSPlayerController* This_CCSPlayerController,
    char* pBuffer, unsigned int bufferSize, unsigned int maybeShortenLength);

using GetPlayerName_t = const char* (*)(CS2::CCSPlayerController*);

void NameController::CheckMenu(Steam64UID uid) {
    auto it = this->nameReplaceInfo.find(uid);
    if (it != this->nameReplaceInfo.end()) {
        ImGui::TextUnformatted(std::format("替换名称: {}", this->nameReplace[it->second]).c_str());
    }
    else {
        ImGui::TextUnformatted("未设置替换名称");
    }
}

void NameController::SetMenu(Steam64UID uid) {
    ImGui::InputText("新名称 (最多127字符)", &this->newNameBuffer);
    ImGui::SameLine();
    if (ImGui::Button("设定（空则清除）")) {
        if (this->SetReplace(uid, this->newNameBuffer)) {
            this->newNameBuffer.clear();
        }
    }
}

bool NameController::Init() {
    this->Hub()->ModulesAboutPlayer.push_back(this);

    auto FnGetDecoratedPlayerName = this->CS2()->Modules.client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::GetDecoratedPlayerName);
    this->hkGetDecoratedPlayerName = MulNX::Memory::HookEx::Create(FnGetDecoratedPlayerName.Data(), 0, false,
        [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
            std::shared_lock lock(this->Hub()->GetMutex());

            auto playerController = (CS2::CCSPlayerController*)ctx->rcx;
            char* pBuffer = (char*)ctx->rdx;
            unsigned int bufferSize = *reinterpret_cast<unsigned int*>(&ctx->r8);
            unsigned int maybeShortenLength = *reinterpret_cast<unsigned int*>(&ctx->r9);

            this->HandleVHook(playerController);

            // 调用原始函数，获取装饰名并写入 pBuffer
            const char* result = reinterpret_cast<GetDecoratedPlayerName_t>(hk->pMaybeRawFunc)(
                playerController, pBuffer, bufferSize, maybeShortenLength
                );

            // 获取玩家的 SteamID
            uint64_t steamId = *playerController->m_steamID();
            auto it = this->nameReplaceInfo.find(steamId);
            if (it != this->nameReplaceInfo.end()) {
                const char* newName = this->nameReplace[it->second];
                memcpy(pBuffer, newName, 127);
                pBuffer[127] = '\0';
                ctx->rax = (uintptr_t)pBuffer;
            }
            else {
                // 未找到替换规则，直接使用原始结果
                ctx->rax = (uintptr_t)result;
            }

            return false; // 已经调用了原始函数，不再重复执行
        }).value();
    this->hkGetDecoratedPlayerName->Attach();

    return true;
}

void NameController::HandleVHook(CS2::CCSPlayerController* pPlayerController) {
    if (this->bGetPlayerNameHooked)return;
    this->hkGetPlayerName = MulNX::Memory::HookEx::Create(reinterpret_cast<uint8_t*>(pPlayerController->GetVFuncPtr(223)), 0, false,
        [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
            std::shared_lock lock(this->Hub()->GetMutex());

            auto playerController = (CS2::CCSPlayerController*)ctx->rcx;

            // 1. 调用原始函数获取原始名字
            const char* originalName = reinterpret_cast<GetPlayerName_t>(hk->pMaybeRawFunc)(playerController);

            // 2. 获取 SteamID
            uint64_t steamId = *playerController->m_steamID();
            auto it = this->nameReplaceInfo.find(steamId);

            // 3. 根据映射表决定返回值
            if (it != this->nameReplaceInfo.end()) {
                ctx->rax = (uintptr_t)this->nameReplace[it->second];
            }
            else {
                ctx->rax = (uintptr_t)originalName;
            }

            return false; // 已调用原始函数，不再重复执行
        }).value();
    this->hkGetPlayerName->Attach();
    this->bGetPlayerNameHooked = true;
}

bool NameController::SetReplace(Steam64UID uid, const std::string& newName) {
    std::unique_lock lock(this->Hub()->GetMutex());
    if (newName.empty()) {
        auto it = this->nameReplaceInfo.find(uid);
        if (it == this->nameReplaceInfo.end()) {
            this->ISys().LogError("无法删除不存在的替换规则！");
            return false;
        }
        int idx = it->second;
        this->nameReplaceInfo.erase(it);
        memset(this->nameReplace[idx], 0, 128);   // 清空槽位
        this->ISys().LogInfo(std::format("已删除 SteamID {} 的名称替换规则", uid));
        return true;
    }

    if (newName.size() >= 128) {
        this->ISys().LogError("名称长度不能超过127个字符！");
        return false;
    }

    auto it = this->nameReplaceInfo.find(uid);
    int idx = -1;

    if (it != this->nameReplaceInfo.end()) {
        // 更新现有条目
        idx = it->second;
    }
    else {
        // 寻找空闲索引
        for (int i = 0; i < 64; ++i) {
            bool used = false;
            for (auto& pair : this->nameReplaceInfo) {
                if (pair.second == i) { used = true; break; }
            }
            if (!used) { idx = i; break; }
        }
        if (idx == -1) {
            this->ISys().LogError("名称替换槽位已满 (最多64条)！");
            return false;
        }
        this->nameReplaceInfo[uid] = idx;
    }

    // 安全复制字符串
    strncpy_s(this->nameReplace[idx], newName.c_str(), 127);
    this->nameReplace[idx][127] = '\0';
    this->ISys().LogInfo(std::format("已为 SteamID {} 设置替换名称: {}", uid, newName));

    return true;
}