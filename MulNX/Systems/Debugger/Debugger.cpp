#include "Debugger.hpp"

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
    this->NeedUINode = true;
    return true;
}
void MulNX::Debugger::ProcessMsg(MulNX::Message* Msg) {
    switch (Msg->type) {
    case "Debugger/SetMaxInfoCount"_hash: {
        this->ResetMaxMsgCount(Msg->p1.i);
    }
    case "Debugger/SaveToFile"_hash: {
        this->SaveToFile();
    }
    }
}
void MulNX::Debugger::VirtualMain() {
    this->EntryProcessMsg();
}

void MulNX::Debugger::ResetMaxMsgCount(const int Max) {
    std::unique_lock lock(this->GetMutex());
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
    std::shared_lock lock(this->GetMutex());
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
    std::unique_lock lock(this->GetMutex());
    this->PushBack(NewMsg, this->Info);
}

void MulNX::Debugger::AddSucc(const std::string& NewMsg) {
    std::unique_lock lock(this->GetMutex());
    this->PushBack(NewMsg, this->Succ);
}

void MulNX::Debugger::AddWarning(const std::string& NewMsg) {
    std::unique_lock lock(this->GetMutex());
    this->PushBack(NewMsg, this->Warning);
}

void MulNX::Debugger::AddError(const std::string& NewMsg) {
    std::unique_lock lock(this->GetMutex());
    this->PushBack(NewMsg, this->Error);
    if (this->ShowWhenError) {
        this->ShowWindow = true;
        this->IfShowStream = true;
    }
}

bool MulNX::Debugger::UINodeFunc(MulNXUINode* ThisNode) {
    if (!this->ShowWindow)return true;
    std::shared_lock lock(this->GetMutex());
    this->ShowFunc(this);
    return true;
}
void MulNX::Debugger::ShowStream() {
    this->IfShowStream = true;
}
void MulNX::Debugger::HideStream() {
    this->IfShowStream = false;
}