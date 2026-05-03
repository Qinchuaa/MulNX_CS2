#include "DllMain.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/CamSysExt.hpp>
#include <MulNXExtensions/MiniMap/MiniMap.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>
#include <MulNXExtensions/MediaRemoter/MediaRemoter.hpp>

#include <Windows.h>
#include <algorithm>
#include <fstream>

namespace {
    enum class MainPanelPage {
        Overview = 0,
        CameraSystem,
        Keybinds,
        GameSettings,
        GameEnhance,
        ControlCenter,
        UISettings,
    };

    bool IsModifierKey(const unsigned char vkCode) {
        return vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL
            || vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT
            || vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU;
    }

    std::vector<std::string> GetSortedProjectNames(const std::vector<std::shared_ptr<Project>>& projects) {
        std::vector<std::string> names;
        names.reserve(projects.size());
        for (const auto& project : projects) {
            if (project) {
                names.push_back(project->Name);
            }
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    std::vector<std::string> GetSortedElementNames(const ElementManager* elementManager) {
        std::vector<std::string> names;
        if (!elementManager) {
            return names;
        }
        names.reserve(elementManager->elements.size());
        for (const auto& [name, element] : elementManager->elements) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    std::vector<std::string> GetSortedSolutionNames(const SolutionManager* solutionManager) {
        std::vector<std::string> names;
        if (!solutionManager) {
            return names;
        }
        names.reserve(solutionManager->solutions.size());
        for (const auto& [name, solution] : solutionManager->solutions) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    bool MainNavButton(const char* label, MainPanelPage page, MainPanelPage& currentPage) {
        const bool selected = currentPage == page;
        const ImVec2 buttonSize(ImGui::GetContentRegionAvail().x, 38.0f);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.25f, 0.55f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.30f, 0.63f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.22f, 0.50f, 1.0f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        }
        const bool clicked = ImGui::Button(label, buttonSize);
        ImGui::PopStyleColor(3);
        if (clicked) {
            currentPage = page;
            return true;
        }
        return false;
    }

    void QuickWindowToggle(const char* label, MulNX::ModuleBase* module) {
        if (!module) {
            ImGui::BeginDisabled();
            bool unavailable = false;
            ImGui::Checkbox(label, &unavailable);
            ImGui::EndDisabled();
            return;
        }
        MulNX::UI::Checkbox(label, module->ShowWindow);
    }

    void QuickBoolToggleThrottled(const char* label, std::atomic<bool>& target, ULONGLONG& lastUpdateTick, ULONGLONG throttleMs) {
        bool value = target.load(std::memory_order_acquire);
        if (ImGui::Checkbox(label, &value)) {
            ULONGLONG now = GetTickCount64();
            if (lastUpdateTick == 0 || now - lastUpdateTick >= throttleMs) {
                target.store(value, std::memory_order_release);
                lastUpdateTick = now;
            }
        }
    }
}

// 这是MulNX_CS2项目的入口文件，也是MulNX项目的示例模块
// 本文件展示了如何使用MulNX核心系统创建一个功能完整的注入式DLL工具。

bool MainDraw::Init() {
    this->SendUINode("MainDraw", [this](MulNX::UINode* node) {this->Window(node);});
    return true;
}

void MainDraw::StartKeybindRecording(KeybindRecordTarget target, const std::string& itemName) {
    this->RecordingTarget = target;
    this->RecordingItemName = itemName;
    this->RecordingMainKey = 0;
    this->PendingRecordedKeybind = {};
    for (int vk = 0; vk < 256; ++vk) {
        this->RecordingPrevKeys[vk] = this->pInputSystem->IsKeyPressed(static_cast<unsigned char>(vk));
    }
}

void MainDraw::StopKeybindRecording() {
    this->RecordingTarget = KeybindRecordTarget::None;
    this->RecordingItemName.clear();
    this->RecordingMainKey = 0;
    this->PendingRecordedKeybind = {};
    this->RecordingPrevKeys.fill(false);
}

bool MainDraw::UpdateKeybindRecording(MulNX::KeyCheckPack& outBinding) {
    if (this->RecordingTarget == KeybindRecordTarget::None) {
        return false;
    }

    for (int vk = 0; vk < 256; ++vk) {
        const auto key = static_cast<unsigned char>(vk);
        const bool isPressed = this->pInputSystem->IsKeyPressed(key);
        const bool wasPressed = this->RecordingPrevKeys[vk];

        if (!wasPressed && isPressed && !IsModifierKey(key) && this->RecordingMainKey == 0) {
            this->RecordingMainKey = key;
            this->PendingRecordedKeybind.Ctrl =
                this->pInputSystem->IsKeyPressed(VK_CONTROL) ||
                this->pInputSystem->IsKeyPressed(VK_LCONTROL) ||
                this->pInputSystem->IsKeyPressed(VK_RCONTROL);
            this->PendingRecordedKeybind.Shift =
                this->pInputSystem->IsKeyPressed(VK_SHIFT) ||
                this->pInputSystem->IsKeyPressed(VK_LSHIFT) ||
                this->pInputSystem->IsKeyPressed(VK_RSHIFT);
            this->PendingRecordedKeybind.Alt =
                this->pInputSystem->IsKeyPressed(VK_MENU) ||
                this->pInputSystem->IsKeyPressed(VK_LMENU) ||
                this->pInputSystem->IsKeyPressed(VK_RMENU);
            this->PendingRecordedKeybind.vkCode = key;
            this->PendingRecordedKeybind.ComboClick = 1;
        }

        if (this->RecordingMainKey != 0 && key == this->RecordingMainKey && wasPressed && !isPressed) {
            this->PendingRecordedKeybind.Refresh();
            outBinding = this->PendingRecordedKeybind;
            this->StopKeybindRecording();
            return true;
        }

        this->RecordingPrevKeys[vk] = isPressed;
    }

    return false;
}

void MainDraw::RenderKeybindPage(ProjectManager* projectManager, ElementManager* elementManager, SolutionManager* solutionManager) {
    ImGui::TextUnformatted("绑键");
    ImGui::Separator();

    if (!projectManager) {
        ImGui::TextUnformatted("项目管理器不可用");
        return;
    }

    auto projects = projectManager->GetProjects();
    auto projectNames = GetSortedProjectNames(projects);

    if (this->SelectedKeybindProjectName.empty() && !projectNames.empty()) {
        this->SelectedKeybindProjectName = projectNames.front();
    }

    ImGui::Text("目标项目");
    ImGui::SameLine();
    const char* currentProjectLabel = this->SelectedKeybindProjectName.empty() ? "无" : this->SelectedKeybindProjectName.c_str();
    if (ImGui::BeginCombo("##绑键项目", currentProjectLabel)) {
        for (const auto& projectName : projectNames) {
            const bool selected = this->SelectedKeybindProjectName == projectName;
            if (ImGui::Selectable(projectName.c_str(), selected)) {
                this->SelectedKeybindProjectName = projectName;
                this->StopKeybindRecording();
            }
        }
        ImGui::EndCombo();
    }

    auto selectedProject = projectManager->FindProject(this->SelectedKeybindProjectName);
    const bool isActiveProject = selectedProject && projectManager->ActiveProject && projectManager->ActiveProject->Name == selectedProject->Name;
    ImGui::Text("当前生效项目: %s", projectManager->ActiveProject ? projectManager->ActiveProject->Name.c_str() : "无");
    ImGui::SameLine();
    if (!selectedProject) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(isActiveProject ? "当前项目已生效" : "切换到当前项目")) {
        if (selectedProject) {
            projectManager->Project_Apply(selectedProject);
        }
    }
    if (!selectedProject) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (!selectedProject) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("导出按键绑定")) {
        std::filesystem::path exportPath = this->ISys().PathGet("Config") / ("keybinds_" + selectedProject->Name + ".txt");
        std::ofstream fout(exportPath);
        if (fout.is_open()) {
            auto elementNames = GetSortedElementNames(elementManager);
            for (const auto& elementName : elementNames) {
                auto it = selectedProject->ElementKeybinds.find(elementName);
                if (it != selectedProject->ElementKeybinds.end() && it->second.Usable) {
                    fout << "Element/" << elementName << " : " << it->second.GetMsg() << "\n";
                }
            }
            auto solutionNames = GetSortedSolutionNames(solutionManager);
            for (const auto& solutionName : solutionNames) {
                auto it = selectedProject->SolutionKeybinds.find(solutionName);
                if (it != selectedProject->SolutionKeybinds.end() && it->second.Usable) {
                    fout << "Solution/" << solutionName << " : " << it->second.GetMsg() << "\n";
                }
            }
            fout.close();
            this->ISys().LogSucc("已导出按键绑定：" + exportPath.string());
        }
        else {
            this->ISys().LogError("导出按键绑定失败");
        }
    }
    if (!selectedProject) {
        ImGui::EndDisabled();
    }

    if (!selectedProject) {
        ImGui::Separator();
        ImGui::TextUnformatted("无可用项目");
        return;
    }

    if (!isActiveProject) {
        ImGui::Separator();
        ImGui::TextUnformatted("当前仅可编辑所选项目的绑键，切换到该项目后这些绑键才会生效");
    }

    const auto recordedTarget = this->RecordingTarget;
    const auto recordedItemName = this->RecordingItemName;
    MulNX::KeyCheckPack recordedBinding{};
    const bool hasRecordedBinding = this->UpdateKeybindRecording(recordedBinding);
    if (hasRecordedBinding) {
        if (recordedTarget == KeybindRecordTarget::Element) {
            selectedProject->ElementKeybinds[recordedItemName] = recordedBinding;
        }
        else if (recordedTarget == KeybindRecordTarget::Solution) {
            selectedProject->SolutionKeybinds[recordedItemName] = recordedBinding;
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("元素", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto elementNames = GetSortedElementNames(elementManager);
        for (const auto& elementName : elementNames) {
            ImGui::TextUnformatted(elementName.c_str());
            ImGui::SameLine(320.0f);
            auto bindingIt = selectedProject->ElementKeybinds.find(elementName);
            const std::string bindingText = (bindingIt != selectedProject->ElementKeybinds.end() && bindingIt->second.Usable)
                ? bindingIt->second.GetMsg()
                : "无绑键";
            ImGui::Text("%s", bindingText.c_str());
            ImGui::SameLine(540.0f);
            const bool isRecordingThis = this->RecordingTarget == KeybindRecordTarget::Element && this->RecordingItemName == elementName;
            const std::string bindButtonLabel = std::string(isRecordingThis ? "记录中..." : "绑键") + "##ElementBind" + elementName;
            if (ImGui::Button(bindButtonLabel.c_str())) {
                this->StartKeybindRecording(KeybindRecordTarget::Element, elementName);
            }
            ImGui::SameLine();
            if (ImGui::Button(("重置##ElementReset" + elementName).c_str())) {
                selectedProject->ElementKeybinds.erase(elementName);
                if (isRecordingThis) {
                    this->StopKeybindRecording();
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("解决方案", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto solutionNames = GetSortedSolutionNames(solutionManager);
        for (const auto& solutionName : solutionNames) {
            ImGui::TextUnformatted(solutionName.c_str());
            ImGui::SameLine(320.0f);
            auto bindingIt = selectedProject->SolutionKeybinds.find(solutionName);
            const std::string bindingText = (bindingIt != selectedProject->SolutionKeybinds.end() && bindingIt->second.Usable)
                ? bindingIt->second.GetMsg()
                : "无绑键";
            ImGui::Text("%s", bindingText.c_str());
            ImGui::SameLine(540.0f);
            const bool isRecordingThis = this->RecordingTarget == KeybindRecordTarget::Solution && this->RecordingItemName == solutionName;
            const std::string bindButtonLabel = std::string(isRecordingThis ? "记录中..." : "绑键") + "##SolutionBind" + solutionName;
            if (ImGui::Button(bindButtonLabel.c_str())) {
                this->StartKeybindRecording(KeybindRecordTarget::Solution, solutionName);
            }
            ImGui::SameLine();
            if (ImGui::Button(("重置##SolutionReset" + solutionName).c_str())) {
                selectedProject->SolutionKeybinds.erase(solutionName);
                if (isRecordingThis) {
                    this->StopKeybindRecording();
                }
            }
        }
    }
}

void MainDraw::Window(MulNX::UINode* node) {
    auto* workspaceManager = this->Core->ModuleManager()->FindModule<WorkspaceManager>("WorkspaceManager");
    auto* projectManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");
    auto* solutionManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    auto* elementManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");

    auto* miniMap = this->Core->ModuleManager()->FindModule("MiniMap");
    auto* gameCfgManager = this->Core->ModuleManager()->FindModule("GameCfgManager");
    auto* csController = this->Core->ModuleManager()->FindModule("CSController");
    auto* demoHelper = this->Core->ModuleManager()->FindModule("DemoHelper");
    auto* playerHub = this->Core->ModuleManager()->FindModule("PlayerHub");
    auto* projectileTracker = this->Core->ModuleManager()->FindModule("ProjectileTracker");
    auto* deathMsgController = this->Core->ModuleManager()->FindModule("DeathMsgController");
    auto* mediaRemoter = this->Core->ModuleManager()->FindModule("MediaRemoter");
    auto* debugger = this->Core->ModuleManager()->FindModule("Debugger");

    static MainPanelPage currentPage = MainPanelPage::Overview;
    static ULONGLONG lastCompactElementToggleTick = 0;

    ImGui::SetNextWindowSize(ImVec2(1280.0f, 760.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin(I18n("ui.main").c_str());

    if (ImGui::BeginChild("##MainTopBar", ImVec2(0, 88), true)) {
        ImGui::Text("快捷键: Insert 隐藏/显示主 UI");
        ImGui::SameLine();
        ImGui::Text("| Alt+M 小地图");
        ImGui::SameLine();
        ImGui::Text("| Alt+T 自动化增强");
        ImGui::SameLine();
        ImGui::Text("| Alt+P 停止播放");
        ImGui::SameLine();
        ImGui::Text("| F 添加关键帧");
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("##MainNav", ImVec2(280, 0), true)) {
        ImGui::TextUnformatted("主导航");
        ImGui::Separator();
        if (ImGui::BeginChild("##MainNavPages", ImVec2(0, 260), true)) {
            MainNavButton("总览", MainPanelPage::Overview, currentPage);
            MainNavButton(I18n("ui.camera_system").c_str(), MainPanelPage::CameraSystem, currentPage);
            MainNavButton("绑键", MainPanelPage::Keybinds, currentPage);
            MainNavButton(I18n("ui.game_settings").c_str(), MainPanelPage::GameSettings, currentPage);
            MainNavButton(I18n("ui.game_enhance").c_str(), MainPanelPage::GameEnhance, currentPage);
            MainNavButton(I18n("ui.mulnx_control").c_str(), MainPanelPage::ControlCenter, currentPage);
            MainNavButton(I18n("ui.settings").c_str(), MainPanelPage::UISettings, currentPage);
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::TextUnformatted("快捷开关");
        if (ImGui::BeginChild("##MainNavToggles", ImVec2(0, 0), true)) {
            QuickWindowToggle("小地图", miniMap);
            QuickWindowToggle("快捷操作", csController);
            QuickWindowToggle("Demo辅助", demoHelper);
            QuickWindowToggle("玩家信息", playerHub);
            QuickWindowToggle("投掷物追踪", projectileTracker);
            QuickWindowToggle("击杀信息", deathMsgController);
            QuickWindowToggle("调试器", debugger);
            if (elementManager) {
                QuickBoolToggleThrottled("简洁元素", elementManager->CompactElementDisplay, lastCompactElementToggleTick, 10000);
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("##MainContent", ImVec2(0, 0), true)) {
        switch (currentPage) {
        case MainPanelPage::Overview: {
            ImGui::TextUnformatted("总览");
            ImGui::Separator();

            if (ImGui::BeginChild("##OverviewStatus", ImVec2(0, 150), true)) {
                ImGui::Text("工作区状态: %s", workspaceManager && workspaceManager->InWorkspace.load() ? "已进入" : "未进入");
                ImGui::Text("当前项目: %s",
                    projectManager && projectManager->ActiveProject ? projectManager->ActiveProject->Name.c_str() : "无");
                ImGui::Text("解决方案数量: %d", solutionManager ? static_cast<int>(solutionManager->solutions.size()) : 0);
                ImGui::Text("元素数量: %d", elementManager ? static_cast<int>(elementManager->elements.size()) : 0);
                ImGui::Separator();
                ImGui::TextUnformatted("常用入口");
                if (ImGui::Button("进入摄像机系统", ImVec2(180, 0))) {
                    currentPage = MainPanelPage::CameraSystem;
                }
                ImGui::SameLine();
                if (ImGui::Button("进入绑键", ImVec2(180, 0))) {
                    currentPage = MainPanelPage::Keybinds;
                }
                ImGui::SameLine();
                if (ImGui::Button("进入游戏设置", ImVec2(180, 0))) {
                    currentPage = MainPanelPage::GameSettings;
                }
                if (ImGui::Button("进入增强面板", ImVec2(180, 0))) {
                    currentPage = MainPanelPage::GameEnhance;
                }
            }
            ImGui::EndChild();

            if (ImGui::BeginChild("##OverviewQuick", ImVec2(0, 0), false)) {
                if (ImGui::CollapsingHeader("常用浮窗控制", ImGuiTreeNodeFlags_DefaultOpen)) {
                    QuickWindowToggle("小地图窗口", miniMap);
                    QuickWindowToggle("游戏配置管理器窗口", gameCfgManager);
                    QuickWindowToggle("快捷操作窗口", csController);
                    QuickWindowToggle("Demo辅助窗口", demoHelper);
                    QuickWindowToggle("玩家信息管理窗口", playerHub);
                    QuickWindowToggle("投掷物追踪器窗口", projectileTracker);
                    QuickWindowToggle("击杀信息窗口", deathMsgController);
                    QuickWindowToggle("媒体遥控窗口", mediaRemoter);
                }
            }
            ImGui::EndChild();
            break;
        }
        case MainPanelPage::CameraSystem: {
            ImGui::TextUnformatted("摄像机系统");
            ImGui::Separator();
            node->CallUINode("CameraSystem");
            break;
        }
        case MainPanelPage::Keybinds: {
            this->RenderKeybindPage(projectManager, elementManager, solutionManager);
            break;
        }
        case MainPanelPage::GameSettings: {
            ImGui::TextUnformatted("游戏设置");
            ImGui::Separator();
            node->CallUINode("GameSettingsManager");
            break;
        }
        case MainPanelPage::GameEnhance: {
            ImGui::TextUnformatted("游戏增强");
            ImGui::Separator();
            node->CallUINode("ConsoleManager");
            break;
        }
        case MainPanelPage::ControlCenter: {
            ImGui::TextUnformatted("MulNX 控制中心");
            ImGui::Separator();
            if (ImGui::BeginTabBar("##ControlCenterTabs")) {
                if (ImGui::BeginTabItem("自动化")) {
                    node->CallUINode("VirtualUser");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("框架控制")) {
                    node->CallUINode("MulNXController");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("辅助窗口")) {
                    QuickWindowToggle("小地图窗口", miniMap);
                    QuickWindowToggle("游戏配置管理器窗口", gameCfgManager);
                    QuickWindowToggle("快捷操作窗口", csController);
                    QuickWindowToggle("Demo辅助窗口", demoHelper);
                    QuickWindowToggle("玩家信息管理窗口", playerHub);
                    QuickWindowToggle("投掷物追踪器窗口", projectileTracker);
                    QuickWindowToggle("击杀信息窗口", deathMsgController);
                    QuickWindowToggle("媒体遥控窗口", mediaRemoter);
                    QuickWindowToggle("调试器窗口", debugger);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            break;
        }
        case MainPanelPage::UISettings: {
            ImGui::TextUnformatted("UI 个性配置");
            ImGui::Separator();
            node->CallUINode("UISystem");
            break;
        }
        default:
            break;
        }
    }
    ImGui::EndChild();
    ImGui::End();
    node->CallUINode("Debugger");
    node->CallUINode("GameCfgManager");
    node->CallUINode("MiniMap");
    node->CallUINode("CSController");
    node->CallUINode("ElementManager");
    node->CallUINode("SolutionManager");
    node->CallUINode("ProjectManager");
    node->CallUINode("DemoHelper");
    node->CallUINode("PlayerHub");
    node->CallUINode("ProjectileTracker");
    node->CallUINode("DeathMsgController");
    node->CallUINode("MediaRemoter");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        HANDLE hThread = CreateThread(NULL, 0, MulNX_CS2_Start, NULL, 0, NULL);
        // 这里不需要等待线程结束，因为它会在完成初始化后自动退出，然后等待进程结束时被操作系统清理
        break;
    }     
    case DLL_THREAD_ATTACH: {
        // 本系统不考虑卸载，计划生命周期与游戏进程相同！
        break;
    }
    case DLL_THREAD_DETACH: {
        break;
    }
    case DLL_PROCESS_DETACH: {
        break;
    }
    default: {
        break;
    }
    }
    return TRUE;
}

DWORD MulNX_CS2_Start(void*) {
    try {
        // 创建核心
        auto* core = MulNX::Core::Core::Create("CS2OBTool");
        // 创建核心启动器
        auto* starter = core->CreateCoreStarter<HookManager>();
        // 手动创建的模块需要手动设置名称
        starter->SetName("HookManager");
        // 设置初始化完成回调
        starter->InitEndCall = [starter]() {
            starter->ISys().LogWarning(I18n("disclaimer"));
#ifdef _DEBUG
            starter->ISys().AsyncCommand("playdemo 111");
            std::thread([]() {
                MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                }).detach();
#endif
            };

        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .BindAbstractLayer3D<CSController>("CSController")
            .CreateModule<MulNX::ShaderCompiler>("ShaderCompiler")
            .CreateModule<MulNX::GraphicsManager>("GraphicsManager")
            .CreateModule<WebSocketManager>("WebSocketManager")
            // 摄像机系统
            .CreateModule<CameraSystem>("CameraSystem")
            .CreateModule<WorkspaceManager>("WorkspaceManager")
            .CreateModule<ProjectManager>("ProjectManager")
            .CreateModule<SolutionManager>("SolutionManager")
            .CreateModule<ElementManager>("ElementManager")
            // CS2
            .CreateModule<HookEntitySystem>("HookEntitySystem")
            .CreateModule<AdvancedViewController>("AdvancedViewController")
            .CreateModule<FreeCameraController>("FreeCameraController")
            .CreateModule<PlayerHub>("PlayerHub")
            .CreateModule<PlayerFlashController>("PlayerFlashController")
            .CreateModule<NameController>("NameController")
            .CreateModule<GlowController>("GlowController")
            .CreateModule<SmokeController>("SmokeController")
            .CreateModule<ObserverController>("ObserverController")
            .CreateModule<ProjectileTracker>("ProjectileTracker")
            .CreateModule<DeathMsgController>("DeathMsgController")
            // 较为上层
            .CreateModule<MiniMap>("MiniMap")
            .CreateModule<VirtualUser>("VirtualUser")
            .CreateModule<GameCfgManager>("GameCfgManager")
            .CreateModule<DemoHelper>("DemoHelper")
            .CreateModule<GameSettingsManager>("GameSettingsManager")
            .CreateModule<ConsoleManager>("ConsoleManager")
            .CreateModule<MediaRemoter>("MediaRemoter")
            // 管理
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<MainDraw>("MainDraw")
            ;

        // 启动核心
        core->Init();
    }
    catch (std::exception& e) {
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    return 0;
}
