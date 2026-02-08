#include "DllMain.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/MiniMap/MiniMap.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNX/ThirdParty/All_ImGui.hpp>

#include <Windows.h>

static void MainDraw(MulNXSingleUIContext* This) {
    ImGui::Begin("主窗口");
    if (ImGui::BeginTabBar("主标签页集")) {
        if (ImGui::BeginTabItem("Demo助手")) {
            This->CallSingleUIContext("DemoHelper");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("游戏设置")) {
            This->CallSingleUIContext("GameSettings");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("总控台")) {
            This->CallSingleUIContext("Control");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("调试")) {
            This->CallSingleUIContext("Debug");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("摄像机系统")) {
            This->CallSingleUIContext("CameraSystem");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
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
    // 创建核心实例
    MulNX::Core::Core* Core = new MulNX::Core::Core();

    // 创建核心启动器实例
    std::unique_ptr<MulNX::Core::CoreStarterBase> Starter = std::make_unique<HookManager>();
    // 设置初始化完成回调
    Starter->InitEndCall = [Core]() {
        MulNX::Core::CoreStarterBase* starter = Core->GetStarter();
        // 初始化额外任务（对系统无影响）
        starter->IDebugger->AddSucc("注入成功！");
        starter->IDebugger->AddSucc("各模块初始化完成！");
        starter->IDebugger->AddWarning("您正在使用测试版本！！");
        starter->AL3D->ExecuteCommand("playdemo 111");
        std::thread([]() {
            MessageBoxW(NULL, L"MulNXDLL 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
            }).detach();
        // 注册主窗口UI上下文
        starter->RegisteMainDrawWith(MainDraw);
        // UI系统的启动由HookManager在Hook完成后自主启动
        };

    // 设置核心启动器
    Core->SetCoreStarter(std::move(Starter));
    // 模块集合
    MulNX::ModulePack Modules;
    
    Modules
        .CreateBack<CameraSystem>("CameraSystem", 101)// 摄像机系统模块
        .CreateBack<MiniMap>("MiniMap", 103)// 小地图模块
        .CreateBack<VirtualUser>("VirtualUser", 104)// 虚拟用户模块
        .CreateBack<CSController>("CSController", 201)// CS控制模块
        .CreateBack<GameCfgManager>("GameCfgManager", 206)// 游戏配置管理模块
        .CreateBack<DemoHelper>("DemoHelper", 207)// Demo辅助模块
        .CreateBack<GameSettingsManager>("GameSettingsManager", 208)// 游戏设置管理模块
        .CreateBack<ConsoleManager>("ConsoleManager", 209);// 控制台管理模块

    // 注册所有模块
    Core->ModuleManager()->RegisteModules(std::move(Modules));

    // 启动核心
    Core->Init();

    return 0;
}