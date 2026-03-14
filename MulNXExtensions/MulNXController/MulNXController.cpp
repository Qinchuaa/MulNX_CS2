#include "MulNXController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNXThirdParty/All_ImGui.hpp>

bool MulNXController::UINodeFunc(MulNXUINode* ThisNode) {
    static bool debugMode = this->GlobalVars->DebugMode;
    if (ImGui::Checkbox("调试模式（Debug Mode），提供更多功能，但可能影响性能和稳定性", &debugMode)) {
        this->GlobalVars->DebugMode = debugMode;
    }
    if (ImGui::Button("打开调试器")) {
        this->IDebugger->ShowWindow = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭调试器")) {
        this->IDebugger->ShowWindow = false;
    }
    if (ImGui::Button("保存调试日志到文件")) {
        MulNX::Message Msg("Debugger/SaveToFile"_hash);
        this->ISys().PublishAsync(std::move(Msg));
    }
    ImGui::Checkbox("当有错误信息时弹出调试器", &this->IDebugger->ShowWhenError);
    ImGui::Checkbox("自动滚动到最新消息", &this->IDebugger->AutoScroll);
    static int MaxDebugMsgs = 1000;
    ImGui::Text("设置最大消息数量（至少100）:");
    ImGui::SameLine();
    ImGui::InputInt("##最大消息数量", &MaxDebugMsgs);
    ImGui::SameLine();
    if (ImGui::Button("应用")) {
        MulNX::Message Msg("Debugger/SetMaxInfoCount"_hash);
        Msg.p1.i = MaxDebugMsgs;
        this->ISys().PublishAsync(std::move(Msg));
    }
    if (ImGui::Button("尝试拉取所有模块信息")) {
        MulNX::Message Msg("ModuleManager/ModuleInfo/Request"_hash);
        this->ISys().PublishAsync(std::move(Msg));
    }
    static std::string msg{};
    ImGui::InputText("手动注入消息", &msg);
    if (ImGui::Button("注入到框架")) {
        this->ISys().PublishAsync(MulNX::HashString(msg));
        msg.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("注入到游戏")) {
        this->AL3D->ExecuteCommand(msg);
        msg.clear();
    }
    return true;
}

bool MulNXController::Init() {
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();
    this->ISys()
        .SubscribeAsync("ModuleManager/ModuleInfo/Response");
    this->NeedUINode = true;

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
    switch (Msg->type) {
    case "ModuleManager/ModuleInfo/Response"_hash: {
        auto* pInfo = Msg->asp.get<ModuleInfo>();
        this->ISys().LogInfo("检测到以下注册模块");
        for (auto& [Name, Handle] : pInfo->Info) {
            this->ISys().LogInfo(Name);
        }
    }
    }
}
void MulNXController::VirtualMain() {
    this->EntryProcessMsg();
}