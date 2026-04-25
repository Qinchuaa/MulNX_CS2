#include "DllMain.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/CamSysExt.hpp>
#include <MulNXExtensions/MiniMap/MiniMap.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>

#include <Windows.h>

// 这是MulNX_CS2项目的入口文件，也是MulNX项目的示例模块
// 本文件展示了如何使用MulNX核心系统创建一个功能完整的注入式DLL工具。

bool MainDraw::Init() {
    this->SendUINode("MainDraw", [this](MulNX::UINode* node) {this->Window(node);});
    return true;
}

void MainDraw::Window(MulNX::UINode* node) {
    ImGui::Begin(I18n("ui.main").c_str());
    if (ImGui::BeginTabBar("TabBar")) {
        if (ImGui::BeginTabItem(I18n("ui.camera_system").c_str())) {
            node->CallUINode("CameraSystem");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(I18n("ui.game_settings").c_str())) {
            node->CallUINode("GameSettingsManager");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(I18n("ui.game_enhance").c_str())) {
            node->CallUINode("ConsoleManager");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(I18n("ui.mulnx_control").c_str())) {
            node->CallUINode("VirtualUser");
            node->CallUINode("MulNXController");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
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
#ifdef _DEBUG
            starter->AL3D->ExecuteCommand("playdemo 111");
            std::thread([]() {
                MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                }).detach();
#endif
            };

        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .BindAbstractLayer3D<CSController>("CSController")// 创建CS控制器模块为AbstractLayer3D模块，ID固定自动分配为系统模块最大ID 100
            .CreateModule<WebSocketManager>("WebSocketManager", 101)
            // 摄像机系统
            .CreateModule<CameraSystem>("CameraSystem", 102)
            .CreateModule<WorkspaceManager>("WorkspaceManager", 103)
            .CreateModule<ProjectManager>("ProjectManager", 104)
            .CreateModule<SolutionManager>("SolutionManager", 105)
            .CreateModule<ElementManager>("ElementManager", 106)
            // CS2
            .CreateModule<HookEntitySystem>("HookEntitySystem", 110)
            .CreateModule<AdvancedViewController>("AdvancedViewController", 111)
            .CreateModule<FreeCameraController>("FreeCameraController", 112)
            .CreateModule<PlayerHub>("PlayerHub", 210)
            .CreateModule<PlayerFlashController>("PlayerFlashController", 212)
            .CreateModule<NameController>("NameController", 213)
            .CreateModule<GlowController>("GlowController", 214)
            .CreateModule<SmokeController>("SmokeController", 215)
            .CreateModule<ObserverController>("ObserverController", 216)
            .CreateModule<ProjectileTracker>("ProjectileTracker", 217)
            .CreateModule<MiniMap>("MiniMap", 303)
            .CreateModule<VirtualUser>("VirtualUser", 304)
            .CreateModule<GameCfgManager>("GameCfgManager", 406)
            .CreateModule<DemoHelper>("DemoHelper", 407)
            .CreateModule<GameSettingsManager>("GameSettingsManager", 408)
            .CreateModule<ConsoleManager>("ConsoleManager", 409)
            .CreateModule<MulNXController>("MulNXController", 410)
            .CreateModule<MainDraw>("MainDraw", 1000)
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