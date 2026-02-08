#include "MulNXController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Systems.hpp>
#include <MulNX/ThirdParty/All_ImGui.hpp>

bool MulNXController::Init() {
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();

    auto SC = MulNXSingleUIContext::Create(this);
    auto* pSC = SC.get<MulNXSingleUIContext>();
    pSC->name = "MulNXController";
    pSC->MyFunc = [this](MulNXSingleUIContext* This)->void {
        if (ImGui::Button("打开调试器")) {
            this->IDebugger->OpenWindow();
        }
        ImGui::SameLine();
        if (ImGui::Button("关闭调试器")) {
            this->IDebugger->CloseWindow();
        }
        ImGui::Checkbox("当有错误信息时弹出调试器", &this->IDebugger->ShowWhenError);
        ImGui::Checkbox("自动滚动到最新消息", &this->IDebugger->AutoScroll);
        static int MaxDebugMsgs = 1000;
        ImGui::Text("设置最大消息数量（至少100）:");
        ImGui::SameLine();
        ImGui::InputInt("##最大消息数量", &MaxDebugMsgs);
        ImGui::SameLine();
        if (ImGui::Button("应用")) {
            MulNX::Message Msg(MulNX::MsgType::Debugger_SetMaxInfoCount);
            Msg.ParamInt = MaxDebugMsgs;
            this->IPublish(std::move(Msg));
        }
        if (ImGui::Button("尝试拉取所有模块信息")) {
            MulNX::Message Msg(MulNX::MsgType::ModuleManager_RequestModuleInfo);
            Msg.pMsgChannel = this->MainMsgChannel;
            this->IPublish(std::move(Msg));
        }
        };
    MulNX::Message Msg(MulNX::MsgType::UISystem_ModulePush);
    Msg.Handle = this->Core->IHandleSystem().RegisteUnique(std::move(SC));
    this->IPublish(std::move(Msg));

    return true;
}

void MulNXController::ProcessMsg(MulNX::Message* Msg) {
    switch (Msg->Type) {
    case MulNX::MsgType::ModuleManager_ResponseModuleInfo: {
        auto Info = this->Core->IHandleSystem().ReleaseUnique(Msg->Handle);
        auto pInfo = Info.get<ModuleInfo>();
        this->IDebugger->AddInfo("检测到以下注册模块");
        for (auto& [Name, Handle] : pInfo->Info) {
            this->IDebugger->AddInfo(Name);
        }
    }
    }
}
void MulNXController::VirtualMain() {
    this->EntryProcessMsg();
}