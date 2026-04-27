#include "UISystem.hpp"

#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/MessageManager/IMessageManager.hpp>
#include <MulNX/Systems/InputSystem/InputSystem.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>
#include <yaml-cpp/yaml.h>
#include <MulNXThirdParty/ImGuiStyleSerializer.h>
#include <Windows.h>
#include <fstream>

bool MulNX::UISystem::Init() {
    this->ISys()
        .SubscribeAsync("UISystem/Start")
        .SubscribeAsync("UISystem/ModulePush");
    this->UIContext.Core = this->Core;
    return true;
}


// auto stylePath = this->ISys().PathGet("Config") / "ImStyle.yaml";
// ImGuiStyle& style = ImGui::GetStyle();
// YAML::Node sroot;
// ImGuiYaml::StyleToYaml(style, sroot);
// std::ofstream fout(stylePath);
// fout << sroot;

void MulNX::UISystem::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "UISystem/Start"_hash: {
        std::string* pStr = Msg.asp.get<std::string>();
        this->UIContext.EntryDraw = std::move(*pStr);
        this->UISystemRunning = true;
        this->ISys().LogWarning("接收到启动消息，UI系统开始启动");

        // 设置ini文件路径
        ImGuiIO& io = ImGui::GetIO();
        auto IniPath = this->ISys().PathGet("Config") / "MulNXUIConfig.ini";
        // 这里需要进行转换，以适配ImGui的接口
        this->strImguiIniPath = MulNX::Base::CharUtility::FilePathToString(IniPath);
        io.IniFilename = this->strImguiIniPath.c_str();
        try {
            {
                // 加载字体
                auto cfgPath = this->ISys().PathGet("Config") / "ui.yaml";
                this->ISys().LogInfo(I18n("ui.font.cfg.load", cfgPath.string()));
                YAML::Node root = YAML::LoadFile(cfgPath.string());
                auto fontFilePath = root["font"]["path"].as<std::string>();
                auto fontSize = root["font"]["size"].as<float>();
                this->ISys().LogSucc(I18n("ui.font.cfg.load_succ", fontFilePath, fontSize));
                this->ISys().LogInfo(I18n("ui.font.load", fontFilePath));
                io.Fonts->AddFontFromFileTTF(fontFilePath.c_str(), fontSize);
                this->ISys().LogSucc(I18n("ui.font.load_succ", fontFilePath));
            }
            {
                // 加载Style
                auto stylePath = this->ISys().PathGet("Config") / "ImStyle.yaml";
                YAML::Node root = YAML::LoadFile(stylePath.string());
                ImGuiStyle newStyle;
                if (ImGuiYaml::YamlToStyle(root, newStyle)) {
                    ImGui::GetStyle() = newStyle;
                }
            }
        }
        catch (const std::exception& e) {
            this->ISys().LogError(e.what());
        }
        catch (...) {
            MulNX::ErrorTerminate("Unknown Error On Load Font!");
        }

        break;
    }
    case "UISystem/ModulePush"_hash: {
        MulNX::UINode* node = Msg.asp.get<MulNX::UINode>();
        this->UIContext.AddUINode(node->hSelf, std::move(*node));
        this->ISys().LogSucc("接收到一个UI节点");
        break;
    }
    }
}

// ImGui窗口处理函数导入
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int MulNX::UISystem::Render() {
    this->EntryProcessMsg();
    if (!this->UISystemRunning) {
        return 0;
    }
    MulNX::Win32::Msg4 msg4;
    while (this->winMsgs.try_dequeue(msg4)) {
        ImGui_ImplWin32_WndProcHandler(msg4.hWnd, msg4.uMsg, msg4.wParam, msg4.lParam);
    }
    ImGuiIO& io = ImGui::GetIO();
    if (this->WantCaptureMouse.load(std::memory_order_acquire)!= io.WantCaptureMouse) {
        this->WantCaptureMouse.store(io.WantCaptureMouse, std::memory_order_release);
    }
    if (this->WantTextInput.load(std::memory_order_acquire) != io.WantTextInput) {
        this->WantTextInput.store(io.WantTextInput, std::memory_order_release);
    }

    this->FrameBefore();
    if (this->pInputSystem->CheckComboClick(VK_INSERT, 1)) {
        this->UIContext.Active = !this->UIContext.Active;
    }
    if (this->UIContext.Active) {
        this->UIContext.Draw();
    }
    this->FrameBehind();

    return 0;
}

void MulNX::UISystem::SetFrameBefore(std::function<void(void)>Before) {
    this->FrameBefore = Before;
    return;
}
void MulNX::UISystem::SetFrameBehind(std::function<void(void)>Behind) {
    this->FrameBehind = Behind;
    return;
}