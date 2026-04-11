#include "ModuleBase.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

MulNX::ModuleBase::~ModuleBase() {
    this->CloseMyThread();
}
void MulNX::ModuleBase::CloseMyThread() {
    return;
}

bool MulNX::ModuleBase::SetName(std::string&& Name) {
    this->ModuleName = std::move(Name);
    return true;
}
std::string MulNX::ModuleBase::GetName()const {
    return this->ModuleName;
}

// 模块自用
void MulNX::ModuleBase::SetMyThreadDelta(int Delta) {
    this->MyThreadDelta = Delta;
}

// 初始化
bool MulNX::ModuleBase::BaseInit() {
    try {
        auto* moduleManager = this->Core->ModuleManager();
        this->IMsgManager = moduleManager->FindModule<MulNX::IMessageManager>("MessageManager");
        this->IDebugger = moduleManager->FindModule<MulNX::IDebugger>("Debugger");
        this->GlobalVars = moduleManager->FindModule<MulNX::GlobalVars>("GlobalVars");
        this->AL3D = moduleManager->FindAbstractLayer3D();
        this->pInputSystem = moduleManager->FindModule<MulNX::InputSystem>("InputSystem");
        this->pPathManager = moduleManager->FindModule<MulNX::PathManager>("PathManager");

        if (!this->HModule.IsValid()) {
            this->HModule = MulNXHandle::CreateHandle();
        }

        this->MainMsgChannel = this->IMsgManager->GetMessageChannel(this->IMsgManager->CreateMessageChannel());
    }
    catch (...) {
        return false;
    }

    return true;
}

bool MulNX::ModuleBase::SendUINode(std::string&& name, std::function<void(MulNXUINode*)>&& func) {
    // 创建UI节点
    MulNXUINode UINode = MulNXUINode::Create(this);
    // 设置UI节点属性
    UINode.name = std::move(name);
    UINode.MyFunc = std::move(func);
    // 创建UI消息
    auto [msg, rp] = MulNX::Message::Create<MulNXUINode>("UISystem/ModulePush"_hash, std::move(UINode));
    // 发送UI消息
    this->ISys().PublishAsync(std::move(msg));
    this->ISys().LogInfo("发送了一个UI节点进入消息系统");
    return true;
}
void MulNX::ModuleBase::SendTask(std::string&& workerName, std::function<bool()>&& task) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::Task::RegistrationPacket>("Task/Create"_hash);
    rp->targetWorker = std::move(workerName);
    rp->task = std::move(task);
    this->ISys().PublishAsync(std::move(msg));
    this->ISys().LogInfo("发送了一个任务进入消息系统");
}

bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* Core) {
    this->Core = Core;
    if (!this->BaseInit()) {
        return false;
    }
    if (!this->Init()) {
        return false;
    }
    this->ISys().LogSucc("初始化成功!");
    this->Inited = true;
    return true;
}
// 主循环
void MulNX::ModuleBase::BaseVirtualMain() {
    return;
}
void MulNX::ModuleBase::EntryVirtualMain() {
    this->BaseVirtualMain();
    this->VirtualMain();
}
// 消息处理
void MulNX::ModuleBase::BaseProcessMsg(MulNX::Message* Msg) {
    return;
}
void MulNX::ModuleBase::EntryProcessMsg() {
    MulNX::IMessageChannel* Channel = this->MainMsgChannel;
    if (Channel != nullptr) {
        MulNX::Message Msg{};
        while (Channel->PullMessage(Msg)) {
            this->BaseProcessMsg(&Msg);
            this->ProcessMsg(Msg);
        }
    }
    this->UIBusy.store(false, std::memory_order_release);
    return;
}

void MulNX::ModuleBase::SetParent(MulNXHandle hModule) {
    this->hParent = hModule;
}
bool MulNX::ModuleBase::HasParent() {
    return this->hParent.IsValid();
}