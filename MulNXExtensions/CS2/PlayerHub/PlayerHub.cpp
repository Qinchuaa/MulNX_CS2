#include "PlayerHub.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool PlayerHub::Window(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("玩家信息管理", this->ShowWindow);
    if (!w) return false;
    static int controlledPlayerIndex = 1;
    try {
        for (int i = 1; i <= 10; ++i) {
            ImGui::Separator();
            if (ImGui::Selectable(std::format("玩家{}", i).c_str())) {
                controlledPlayerIndex = i;
            }
            auto* playerController = this->CS->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!playerController)continue;
            auto hPawn = MulNX::MRead(playerController->hPawn());
            auto* pawn = this->CS->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pawn)continue;

            auto name = MulNX::Memory::ReadString(playerController->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("玩家名字: {}", name).c_str());
            // 展示覆盖名字
            auto overrideName = this->overridePlayerNames[i - 1].load();
            if (overrideName) {
                ImGui::TextUnformatted(std::format("覆盖名字: {}", *overrideName).c_str());
            }

            ImGui::Separator();
        }
        ImGui::Separator();
        // 显示当前控制玩家的信息
        ImGui::TextUnformatted(std::format("当前控制玩家: 玩家{}", controlledPlayerIndex).c_str());
        auto* playerController = this->CS->Modules.client.GetBaseEntity(controlledPlayerIndex)->As<CS2::CCSPlayerController>();
        if (!playerController)return true;
        auto hPawn = MulNX::MRead(playerController->hPawn());
        auto* pawn = this->CS->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pawn)return true;

        auto pos = MulNX::MRead(pawn->vOldOrigin());
        auto rot = MulNX::MRead(pawn->angEyeAngles());
        ImGui::TextUnformatted(std::format("位置: x: {:.2f}, y: {:.2f}, z: {:.2f}", pos.x, pos.y, pos.z).c_str());
        ImGui::TextUnformatted(std::format("旋转: pitch: {:.2f}, yaw: {:.2f}, roll: {:.2f}", rot.x, rot.y, rot.z).c_str());

        static std::string overrideName;
        ImGui::InputText("强制玩家名字", &overrideName);
        if (ImGui::Button("创建替换玩家名字的覆盖")) {
            if (0 < overrideName.size() && overrideName.size() <= 127) {
                this->overridePlayerNames[controlledPlayerIndex - 1].store(std::make_shared<std::string>(overrideName));
            }
            else {
                this->ISys().LogError("玩家名字长度不能为0或超过127个字符！");
            }
        }
        if(ImGui::Button("移除替换玩家名字的覆盖")) {
            this->overridePlayerNames[controlledPlayerIndex - 1].store(nullptr);
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
    this->NeedThread(3);
    return true;
}

void PlayerHub::ThreadMain() {
    while (this->MyThreadRunning) {
        try {
            try {
                for (int i = 1;i <= 10;++i) {
                    auto* playerController = this->CS->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
                    if (!playerController)continue;
                    auto hPawn = MulNX::MRead(playerController->hPawn());
                    auto* pawn = this->CS->Modules.client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
                    if (!pawn)continue;

                    if(this->overridePlayerNames[i - 1].load()) {
                        auto overrideName = this->overridePlayerNames[i - 1].load();
                        if (overrideName) {
                            for (size_t j = 0; j < overrideName->size(); ++j) {
                                MulNX::MWrite(playerController->m_iszPlayerName() + j, (*overrideName)[j]);
                            }
                            // 写入字符串结束符
                            MulNX::MWrite(playerController->m_iszPlayerName() + overrideName->size(), '\0');
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                this->ISys().LogWarning(std::format("在更新玩家信息时捕获到异常：{}", e.what()));
            }
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
    }
    return;
}