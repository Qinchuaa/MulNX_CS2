#include "VirtualUser.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/ICameraSystem.hpp>

bool VirtualUser::Init() {
    this->CameraSystem = this->Core->ModuleManager()->FindModule<ICameraSystem>("CameraSystem");
    this->Running = true;

    this->ISys()
#ifdef _DEBUG
        .SubscribeAsync("Core/Tick30min")
#endif // _DEBUG
        .SubscribeAsync("Core/Tick60")
        .SubscribeAsync("CameraSystem/Play/Shutdown")
        .SubscribeAsync("Command/SpecPlayer")
        .SubscribeAsync("Game/NewRound");
    return true;
}
void VirtualUser::Menu() {
    bool temp = this->Running.load();
    if (ImGui::Checkbox("启动自动化控制", &temp)) {
        this->Running.store(temp);
    }
    static std::string TargetSolutionName{};
    ImGui::Text("想调用的解决方案：");
    ImGui::SameLine();
    ImGui::InputText("##TargetWorkspaceName", &TargetSolutionName);
    ImGui::SameLine();
    if (ImGui::Button("调用")) {
        this->CameraSystem->CallSolution(TargetSolutionName);
    }

    if (ImGui::Button("停止播放")) {
        this->CameraSystem->ShutDown();
    }
    static std::string StrText{};
    ImGui::Text("命令执行：");
    ImGui::SameLine();
    ImGui::InputText("##String", &StrText);
    ImGui::SameLine();
    if (ImGui::Button("执行")) {
        if (!StrText.empty()) {
            this->AL3D->ExecuteCommand(StrText);
            StrText.clear();
        }
    }

    return;
}
void VirtualUser::VirtualMain() {
    this->EntryProcessMsg();
    if (this->KT->CheckWithPack(MulNX::KeyCheckPack{ true,false,false,true,'P',1 })) {
        this->CameraSystem->ShutDown();
    }
    if (this->KT->CheckWithPack(MulNX::KeyCheckPack{ true,false,false,true,'T',1 })) {
        bool AutoRunning = this->Running.load();
        if (AutoRunning) {
            this->Running.store(false);
            this->ISys().LogWarning("自动化功能已关闭");
        }
        else {
            this->Running.store(true);
            this->ISys().LogWarning("自动化功能已开启");
        }
    }
    return;
}

void VirtualUser::ProcessMsg(MulNX::Message& Msg) {
    if (!this->Running)return;
    switch (Msg.type) {
    case "Game/NewRound"_hash: {
        this->ISys().LogInfo("接收到新回合信息");
        this->CameraSystem->CallSolution(Msg);
        break;
    }
    case "CameraSystem/Play/Shutdown"_hash: {
        // 处理播放停止消息
        this->ISys().LogInfo("接收到摄像机系统播放停止信息");
        this->CameraSystem->ShutDown();
        break;
    }
    case "Core/Tick1"_hash: {
        // this->Debugger->AddInfo("一秒");
        break;
    }
    case "Core/Tick60"_hash: {
        this->CameraSystem->Save();
        this->ISys().LogInfo("已触发自动保存（频率：每分钟）");
        break;
    }
#ifdef _DEBUG
    case "Core/Tick30min"_hash: {
        this->AL3D->ExecuteCommand("playdemo 111");
        break;
    }
#endif // _DEBUG
    case "Command/SpecPlayer"_hash: {
        this->CameraSystem->ShutDown();
        this->AL3D->SpecPlayer(Msg.p1.i);
        break;
    }
    default: {
        break;
    }

    }
    return;
}