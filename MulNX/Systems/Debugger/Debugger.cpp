#include "Debugger.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

#include <bitset>

bool MySaveStringToFile(const std::string& data,
    const std::filesystem::path& filePath) {
    // 强制按二进制打开可避免换行转换
    std::ofstream out(filePath, std::ios::binary);
    if (!out) {
        return false;
    }

    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return out.good();
}
bool MulNX::Debugger::Init() {
    this->ISys()
        .SubscribeAsync("Debugger/SetMaxInfoCount")
        .SubscribeAsync("Debugger/SaveToFile");
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendTask("MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}
void MulNX::Debugger::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Debugger/SetMaxInfoCount"_hash: {
        this->ResetMaxMsgCount(Msg.p1.low<float>());
    }
    case "Debugger/SaveToFile"_hash: {
        this->SaveToFile();
    }
    }
}
void MulNX::Debugger::Main() {
    this->EntryProcessMsg();
}

void MulNX::Debugger::ResetMaxMsgCount(const int Max) {
    std::unique_lock lock(this->smutex);
    if (Max < 1) {
        this->AddError("最大信息条数不能小于一1!");
        return;
    }
    if (DebugMsg.size() > Max) {
        DebugMsg.erase(DebugMsg.begin(), DebugMsg.end() - Max);
    }
    this->MaxMsgCount = Max;
    lock.unlock();
    this->AddInfo("已重置最大信息条数为 " + std::to_string(Max) + " 条");

    return;
}
void MulNX::Debugger::SaveToFile() {
    std::shared_lock lock(this->smutex);
    std::string data;
    for (const auto& msg : this->DebugMsg) {
        data += msg + "\n";
    }
    
    auto path = this->ISys().PathManager()->PathGetForShared("Log") / ("Log_" + this->Core->GetName() + ".txt");
    if (!MySaveStringToFile(data, path)) {
        // 处理错误
        throw std::runtime_error("无法保存调试日志到文件: " + path.string());
    }

}

void MulNX::Debugger::PushBack(const std::string& NewMsg, const std::string& prefix) {
    // 这里不需要锁，因为调用此函数的上层函数已经加锁
    // 检查字符串是否包含换行符
    if (NewMsg.find('\n') == std::string::npos && NewMsg.find('\r') == std::string::npos) {
        // 没有换行符，直接添加（注意：这里需要加上前缀）
        if (this->DebugMsg.size() == this->MaxMsgCount) {
            this->DebugMsg.pop_front();
        }
        this->DebugMsg.push_back(prefix + NewMsg);
    }
    else {
        // 有换行符，分割字符串并为每一行添加前缀
        std::istringstream iss(NewMsg);
        std::string line;
        bool firstLine = true;
        int lineCount = 0;

        while (std::getline(iss, line)) {
            // 清理回车符
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // 跳过空行
            if (line.empty()) continue;

            std::string formattedLine;
            if (firstLine) {
                // 第一行直接添加前缀
                formattedLine = prefix + line;
                firstLine = false;
            }
            else {
                // 后续行添加前缀//和缩进
                formattedLine = prefix + line;
            }

            // 添加到消息队列
            if (this->DebugMsg.size() == this->MaxMsgCount) {
                this->DebugMsg.pop_front();
            }
            this->DebugMsg.push_back(formattedLine);
            lineCount++;
        }
    }

    if (this->AutoScroll) {
        this->NeedAutoScroll = true;
    }
    return;
}

void MulNX::Debugger::AddInfo(const std::string& NewMsg) {
    std::unique_lock lock(this->smutex);
    this->PushBack(NewMsg, this->Info);
}

void MulNX::Debugger::AddSucc(const std::string& NewMsg) {
    std::unique_lock lock(this->smutex);
    this->PushBack(NewMsg, this->Succ);
}

void MulNX::Debugger::AddWarning(const std::string& NewMsg) {
    std::unique_lock lock(this->smutex);
    this->PushBack(NewMsg, this->Warning);
}

void MulNX::Debugger::AddError(const std::string& NewMsg) {
    std::unique_lock lock(this->smutex);
    this->PushBack(NewMsg, this->Error);
    if (this->ShowWhenError) {
        this->ShowWindow = true;
        this->IfShowStream = true;
    }
}

bool MulNX::Debugger::UINodeFunc(MulNX::UINode* ThisNode) {
    auto w = MulNX::UI::RAIIWindow("调试器", this->ShowWindow);
    if (!w)return true;
    std::shared_lock lock(this->smutex);

    // 在标签页内创建一个子窗口
    ImVec2 childSize = ImGui::GetContentRegionAvail();
    childSize.y -= ImGui::GetStyle().ItemSpacing.y; // 留出一点空间

    // 开始子窗口，占据标签页的剩余空间
    ImGui::BeginChild("信息", childSize, true, ImGuiWindowFlags_HorizontalScrollbar);

    // 使用虚拟列表优化性能
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(this->DebugMsg.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& msg = this->DebugMsg[i];

            // 根据消息类型着色
            if (msg.find(this->Info) == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 50, 255, 255));
            }
            else if (msg.find(this->Succ) == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 100, 0, 255));
            }
            else if (msg.find(this->Warning) == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 0, 255));
            }
            else if (msg.find(this->Error) == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
            }

            ImGui::TextUnformatted(msg.c_str());

            // 弹出
            ImGui::PopStyleColor();
        }
    }

    // 自动滚动到最新消息
    if (this->NeedAutoScroll) {
        ImGui::SetScrollHereY(1.0f);
        this->NeedAutoScroll = false;
    }

    // 结束子窗口
    ImGui::EndChild();
    return true;
}
void MulNX::Debugger::ShowStream() {
    this->IfShowStream = true;
}
void MulNX::Debugger::HideStream() {
    this->IfShowStream = false;
}