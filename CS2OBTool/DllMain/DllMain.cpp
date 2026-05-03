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

namespace {
    enum class MainPanelPage {
        Overview = 0,
        CameraSystem,
        GameSettings,
        GameEnhance,
        ControlCenter,
        UISettings,
    };

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
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("##MainNav", ImVec2(280, 0), true)) {
        ImGui::TextUnformatted("主导航");
        ImGui::Separator();
        if (ImGui::BeginChild("##MainNavPages", ImVec2(0, 260), true)) {
            MainNavButton("总览", MainPanelPage::Overview, currentPage);
            MainNavButton(I18n("ui.camera_system").c_str(), MainPanelPage::CameraSystem, currentPage);
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
                if (ImGui::Button("进入游戏设置", ImVec2(180, 0))) {
                    currentPage = MainPanelPage::GameSettings;
                }
                ImGui::SameLine();
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
