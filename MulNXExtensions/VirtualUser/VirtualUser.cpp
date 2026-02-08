#include "VirtualUser.hpp"

#include <MulNX/MulNX.hpp>

#include "../CameraSystem/ICameraSystem.hpp"

#include <MulNX/ThirdParty/All_ImGui.hpp>

bool VirtualUser::Init() {
    this->CameraSystem = this->Core->ModuleManager()->FindModule<ICameraSystem>("CameraSystem");
    this->Running = true;

    // this->ISubscribe(MsgType::Core_Tick1);
    this->ISubscribe(MulNX::MsgType::Core_Tick60);
#ifdef _DEBUG
    this->ISubscribe(MulNX::MsgType::Core_Tick30min);
#endif // _DEBUG
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();
    (*this->MainMsgChannel)
        .Subscribe(MulNX::MsgType::CameraSystem_PlayingShutdown)
        .Subscribe(MulNX::MsgType::Command_SpecPlayer)
        .Subscribe(MulNX::MsgType::Game_NewRound);

    return true;
}
void VirtualUser::Menu() {
    MulNX::AutoChild Child(this, "VirtualUser");

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
            this->IDebugger->AddWarning("自动化功能已关闭");
        }
        else {
            this->Running.store(true);
            this->IDebugger->AddWarning("自动化功能已开启");
        }
    }
    return;
}

void VirtualUser::ProcessMsg(MulNX::Message* Msg) {
    if (!this->Running)return;
    switch (Msg->Type) {
    case MulNX::MsgType::Game_NewRound: {
        this->IDebugger->AddInfo("接收到新回合信息");
        this->CameraSystem->CallSolution(*Msg);
        break;
    }
    case MulNX::MsgType::CameraSystem_PlayingShutdown: {
        // 处理播放停止消息
        this->IDebugger->AddInfo("接收到摄像机系统播放停止信息");
        this->CameraSystem->ShutDown();
        break;
    }
    case MulNX::MsgType::Core_Tick1: {
        // this->Debugger->AddInfo("一秒");
        break;
    }
    case MulNX::MsgType::Core_Tick60: {
        this->CameraSystem->Save();
        this->IDebugger->AddInfo("已触发自动保存（频率：每分钟）");
        break;
    }
#ifdef _DEBUG
    case MulNX::MsgType::Core_Tick30min: {
        this->AL3D->ExecuteCommand("playdemo 111");
        break;
    }
#endif // _DEBUG


    case MulNX::MsgType::Command_SpecPlayer: {
        this->CameraSystem->ShutDown();
        this->AL3D->SpecPlayer(Msg->ParamInt);
        break;
    }
    default: {
        break;
    }

    }
    return;
}