#include "PlayerHub.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

using GetDecoratedPlayerName_t = const char* (*)(CS2::CCSPlayerController* This_CCSPlayerController,
    char* pBuffer, unsigned int bufferSize, unsigned int maybeShortenLength);

using GetPlayerName_t = const char* (__fastcall*)(CS2::CCSPlayerController*);

bool PlayerHub::Window(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("玩家信息管理", this->ShowWindow);
    if (!w) return false;

    static uint64_t choosingSteamID = 0;
    static std::string newNameBuffer;           // 输入缓冲区
    static bool needClearBuffer = false;        // 标记是否需要清空缓冲区

    try {
        std::shared_lock lock(this->GetMutex());
        // 当选中 SteamID 变化时，自动加载已有名称到缓冲区
        if (needClearBuffer || (choosingSteamID != 0 && newNameBuffer.empty())) {
            auto it = this->nameReplaceInfo.find(choosingSteamID);
            if (it != this->nameReplaceInfo.end()) {
                newNameBuffer = this->nameReplace[it->second];
            }
            else {
                newNameBuffer.clear();
            }
            needClearBuffer = false;
        }

        // ---------- 玩家列表区域 ----------
        ImGui::TextUnformatted("检测到如下玩家信息：");
        static int showMax = 10;
        ImGui::SliderInt("搜索的最大数量", &showMax, 1, 255);

        int playerNum = 0;

        for (int i = 0; i <= std::min(this->CS->Modules.client.dwGameEntitySystem_highestEntityIndex(), showMax); ++i) {
            auto* baseEntity = this->CS->Modules.client.GetBaseEntity(i);
            if (!baseEntity) continue;

            auto* playerController = baseEntity->As<CS2::CCSPlayerController>();
            if (!playerController) continue;

            auto hPawn = MulNX::MRead(playerController->hPawn());
            auto* pawn = this->CS->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pawn) continue;

            uint64_t SteamID = MulNX::MRead(playerController->m_steamID());
            if (SteamID == 0) continue;

            ++playerNum;
            std::string displayName = std::format("玩家 {} (SteamID: {})", playerNum, SteamID);
            if (ImGui::Selectable(displayName.c_str(), choosingSteamID == SteamID)) {
                choosingSteamID = SteamID;
                needClearBuffer = true;   // 下次循环时刷新缓冲区
            }

            auto naturalName = MulNX::Memory::ReadString(playerController->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("自然名字: {}", naturalName).c_str());

            auto it = this->nameReplaceInfo.find(SteamID);
            if (it != this->nameReplaceInfo.end()) {
                ImGui::TextUnformatted(std::format("替换名称: {}", this->nameReplace[it->second]).c_str());
            }
            else {
                ImGui::TextUnformatted("未设置替换名称");
            }
            ImGui::Separator();
        }

        if (ImGui::CollapsingHeader("所有已设置的替换规则")) {
            for (auto& pair : this->nameReplaceInfo) {
                ImGui::Text("SteamID: %llu  ->  %s", pair.first, this->nameReplace[pair.second]);
            }
        }

        lock.unlock();

        // ---------- 编辑区域 ----------
        ImGui::SeparatorText("名称替换设置");
        ImGui::Text("当前选中的 SteamID: %llu", choosingSteamID);
        if (choosingSteamID == 0) {
            ImGui::TextDisabled("请先在上方列表中选择一名玩家");
            return true;
        }
        ImGui::InputText("新名称 (最多127字符)", &newNameBuffer);
        ImGui::SameLine();
        if (ImGui::Button("保存/更新")) {
            if (newNameBuffer.size() >= 128) {
                this->ISys().LogError("名称长度不能超过127个字符！");
            }
            else {
                std::unique_lock lock(this->GetMutex());
                auto it = this->nameReplaceInfo.find(choosingSteamID);
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
                    }
                    else {
                        this->nameReplaceInfo[choosingSteamID] = idx;
                    }
                }

                if (idx != -1) {
                    // 安全复制字符串
                    strncpy_s(this->nameReplace[idx], newNameBuffer.c_str(), 127);
                    this->nameReplace[idx][127] = '\0';
                    this->ISys().LogInfo(std::format("已为 SteamID {} 设置替换名称: {}", choosingSteamID, newNameBuffer));
                }
            }
        }

        // 删除按钮（仅当存在替换规则时显示）
        if (ImGui::Button("删除")) {
            std::unique_lock lock(this->GetMutex());
            auto it = this->nameReplaceInfo.find(choosingSteamID);
            if (it != this->nameReplaceInfo.end()) {
                int idx = it->second;
                this->nameReplaceInfo.erase(it);
                memset(this->nameReplace[idx], 0, 128);   // 清空槽位
                newNameBuffer.clear();
                this->ISys().LogInfo(std::format("已删除 SteamID {} 的名称替换规则", choosingSteamID));
            }
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在绘制玩家信息时捕获到异常：{}", e.what()));
    }
    return true;
}

bool PlayerHub::Init() {
    this->CS = this->Core->ModuleManager()->FindModule<CSController>("CSController");
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->Window(node);});

    auto FnGetDecoratedPlayerName = this->CS->Modules.client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::GetDecoratedPlayerName);
    this->hkGetDecoratedPlayerName = MulNX::Memory::HookEx::Create(FnGetDecoratedPlayerName.Data(), 0, false,
        [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
            std::shared_lock lock(this->GetMutex());

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

void PlayerHub::HandleVHook(CS2::CCSPlayerController* pPlayerController) {
    if (this->bGetPlayerNameHooked)return;
    this->hkGetPlayerName = MulNX::Memory::HookEx::Create(reinterpret_cast<uint8_t*>(pPlayerController->GetVFuncPtr(223)), 0, false,
        [this](RegContext* ctx, MulNX::Memory::HookEx* hk)->bool {
            std::shared_lock lock(this->GetMutex());

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