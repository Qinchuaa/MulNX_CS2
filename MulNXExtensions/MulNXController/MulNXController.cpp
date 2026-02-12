#include "MulNXController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Systems.hpp>
#include <MulNX/ThirdParty/All_ImGui.hpp>

bool MulNXController::Init() {
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();

    auto SC = MulNXUINode::Create(this);
    auto* pSC = SC.get<MulNXUINode>();
    pSC->name = "MulNXController";
    pSC->MyFunc = [this](MulNXUINode* This)->void {
        // 调试功能设置
        // 调试模式下提供更多功能，但可能影响性能和稳定性
        static bool debugMode = this->GlobalVars->DebugMode;
        if (ImGui::Checkbox("调试模式（Debug Mode），提供更多功能，但可能影响性能和稳定性", &debugMode)) {
            this->GlobalVars->DebugMode = debugMode;
        }
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
        if (ImGui::CollapsingHeader("初始化控制")) {
            if (ImGui::Button("初始化IPCer")) {
                this->IDebugger->AddInfo("正在尝试初始化IPCer");
                this->Core->IPCer().Init();
            }
            ImGui::SameLine();
            if (ImGui::Button("查看IPCer结果")) {
                if (this->Core->IPCer().Inited) {
                    this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
                    this->IDebugger->AddInfo(this->Core->IPCer().GetAllPathMsg());
                    this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
                }
                else {
                    this->IDebugger->AddError("IPCer尚未初始化成功！");
                }
            }
        }
        };
    MulNX::Message Msg(MulNX::MsgType::UISystem_ModulePush);
    Msg.Handle = this->Core->IHandleSystem().RegisteUnique(std::move(SC));
    this->IPublish(std::move(Msg));

    this->IDebugger->SetShowFunc([](MulNX::Debugger* This)->void {
        ImGui::Begin("调试器");
        // 在标签页内创建一个子窗口
        ImVec2 childSize = ImGui::GetContentRegionAvail();
        childSize.y -= ImGui::GetStyle().ItemSpacing.y; // 留出一点空间

        // 开始子窗口，占据标签页的剩余空间
        ImGui::BeginChild("信息", childSize, true, ImGuiWindowFlags_HorizontalScrollbar);

        // 使用虚拟列表优化性能
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(This->DebugMsg.size()));
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto& msg = This->DebugMsg[i];

                // 根据消息类型着色
                if (msg.find(This->Info) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 50, 255, 255));
                }
                else if (msg.find(This->Succ) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 100, 0, 255));
                }
                else if (msg.find(This->Warning) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 0, 255));
                }
                else if (msg.find(This->Error) == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
                }

                ImGui::TextUnformatted(msg.c_str());

                //弹出
                ImGui::PopStyleColor();
            }
        }

        // 自动滚动到最新消息
        if (This->NeedAutoScroll) {
            ImGui::SetScrollHereY(1.0f);
            This->NeedAutoScroll = false;
        }

        // 结束子窗口
        ImGui::EndChild();
        ImGui::End();
        });

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