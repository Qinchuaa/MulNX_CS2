#include "DllMain.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/MiniMap/MiniMap.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>

#include <Windows.h>

// 这是MulNX_CS2项目的入口文件，也是MulNX项目的示例模块
// 本文件展示了如何使用MulNX核心系统创建一个功能完整的注入式DLL工具。

static void MainDraw(MulNXUINode* node) {
    ImGui::Begin("主窗口");
    if (ImGui::BeginTabBar("主标签页集")) {
        if (ImGui::BeginTabItem("摄像机系统")) {
            node->CallUINode("CameraSystem");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("游戏设置")) {
            node->CallUINode("GameSettingsManager");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("游戏增强")) {
            node->CallUINode("ConsoleManager");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MulNX控制")) {
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
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        HANDLE hThread = CreateThread(NULL, 0, MulNX_CS2_Start, NULL, 0, NULL);
        // 这里不需要等待线程结束，因为它会在完成初始化后自动退出，然后等待进程结束时被操作系统清理
        break;
    }
                           // 本系统不考虑卸载，计划生命周期与游戏进程相同！
    case DLL_THREAD_ATTACH: {
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

        // 创建核心启动器实例
        std::unique_ptr<MulNX::Core::CoreStarterBase> starter = std::make_unique<HookManager>();
        // 手动创建的模块需要手动设置名称
        starter->SetName("HookManager");
        // 设置初始化完成回调
        starter->InitEndCall = [core]() {
            MulNX::Core::CoreStarterBase* pStarter = core->GetStarter();
            // 初始化额外任务（对系统无影响）
            pStarter->ISys().LogSucc("注入成功！");
            pStarter->ISys().LogInfo("各模块初始化完成！");
            pStarter->ISys().LogWarning("您正在使用测试版本！！");
#ifdef _DEBUG
            pStarter->AL3D->ExecuteCommand("playdemo 111");
            std::thread([]() {
                MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                }).detach();
#endif
            // 注册主窗口UI上下文
            pStarter->RegisterMainDrawWith(MainDraw);
            // UI系统的启动由HookManager在Hook完成后自主启动
            };

        // 设置核心启动器
        core->SetCoreStarter(std::move(starter));

        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .BindAbstractLayer3D<CSController>("CSController")// 创建CS控制器模块为AbstractLayer3D模块，ID固定自动分配为系统模块最大ID 100
            .CreateModule<WebSocketManager>("WebSocketManager", 101)// 网络管理模块
            .CreateModule<CameraSystem>("CameraSystem", 102)// 摄像机系统模块
            .CreateModule<WorkspaceManager>("WorkspaceManager", 103)// 工作区管理模块
            .CreateModule<ProjectManager>("ProjectManager", 104)// 项目管理模块
            .CreateModule<SolutionManager>("SolutionManager", 105)// 解决方案管理模块
            .CreateModule<ElementManager>("ElementManager", 106)// 元素管理模块
            .CreateModule<AdvancedViewController>("AdvancedViewController", 111)// 高级视角控制模块
            .CreateModule<FreeCameraController>("FreeCameraController", 112)// 自由摄像机控制模块
            .CreateModule<PlayerHub>("PlayerHub", 210)// 玩家信息管理模块
            .CreateModule<PlayerFlashController>("PlayerFlashController", 212)// 闪光控制模块
            .CreateModule<NameController>("NameController", 213)
            .CreateModule<GlowController>("GlowController", 214)
            .CreateModule<SmokeController>("SmokeController", 215)
            .CreateModule<ObserverController>("ObserverController", 216)
            .CreateModule<MiniMap>("MiniMap", 303)// 小地图模块
            .CreateModule<VirtualUser>("VirtualUser", 304)// 虚拟用户模块
            .CreateModule<GameCfgManager>("GameCfgManager", 406)// 游戏配置管理模块
            .CreateModule<DemoHelper>("DemoHelper", 407)// Demo辅助模块
            .CreateModule<GameSettingsManager>("GameSettingsManager", 408)// 游戏设置管理模块
            .CreateModule<ConsoleManager>("ConsoleManager", 409)// 控制台管理模块
            .CreateModule<MulNXController>("MulNXController", 410)// MulNX控制器模块
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