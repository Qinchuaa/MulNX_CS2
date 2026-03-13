#include "DllMain.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/MiniMap/MiniMap.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXThirdParty/All_ImGui.hpp>

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
        if (ImGui::BeginTabItem("控制台")) {
            node->CallUINode("ConsoleManager");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Demo助手")) {
            node->CallUINode("DemoHelper");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MulNX框架控制器")) {
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
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        HANDLE hThread = CreateThread(NULL, 0, MulNX_CS2_Start, NULL, 0, NULL);
        break;
    }
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
    // 得到唯一核心，必须设置核心名称，路径管理依赖核心名
    auto* Core = MulNX::Core::Core::Create("CS2OBTool");// 注意此函数只允许被调用一次，除此之外会报错

    // 创建核心启动器实例
    std::unique_ptr<MulNX::Core::CoreStarterBase> Starter = std::make_unique<HookManager>();
    // 手动创建的模块需要手动设置名称
    Starter->SetName("HookManager");
    // 设置初始化完成回调
    Starter->InitEndCall = [Core]() {
        MulNX::Core::CoreStarterBase* starter = Core->GetStarter();
        // 初始化额外任务（对系统无影响）
        starter->ISys().LogSucc("注入成功！");
        starter->ISys().LogInfo("各模块初始化完成！");
        starter->ISys().LogWarning("您正在使用测试版本！！");
#ifdef _DEBUG
        starter->AL3D->ExecuteCommand("playdemo 111");
        std::thread([]() {
            MessageBoxW(NULL, L"MulNXDLL 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
            }).detach();
#endif
        // 注册主窗口UI上下文
        starter->RegisterMainDrawWith(MainDraw);
        // UI系统的启动由HookManager在Hook完成后自主启动
        };

    // 设置核心启动器
    Core->SetCoreStarter(std::move(Starter));

    // 注册所有模块
    (*Core->ModuleManager())
        .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
        .BindAbstractLayer3D<CSController>("CSController")// 创建CS控制器模块为AbstractLayer3D模块，ID固定自动分配为系统模块最大ID 100
        .CreateModule<CameraSystem>("CameraSystem", 101)// 摄像机系统模块
        .CreateModule<MiniMap>("MiniMap", 103)// 小地图模块
        .CreateModule<VirtualUser>("VirtualUser", 104)// 虚拟用户模块
        .CreateModule<GameCfgManager>("GameCfgManager", 206)// 游戏配置管理模块
        .CreateModule<DemoHelper>("DemoHelper", 207)// Demo辅助模块
        .CreateModule<GameSettingsManager>("GameSettingsManager", 208)// 游戏设置管理模块
        .CreateModule<ConsoleManager>("ConsoleManager", 209)// 控制台管理模块
        .CreateModule<MulNXController>("MulNXController", 210)// MulNX控制器模块
        ;

    // 启动核心
    Core->Init();

    return 0;
}