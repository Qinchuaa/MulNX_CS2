#include "ModuleBase.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Cores.hpp>
#include <MulNX/Systems/ISystems.hpp>

MulNX::ModuleBase::~ModuleBase() {
    this->CloseMyThread();
}
void MulNX::ModuleBase::CloseMyThread() {
    if (this->MyThreadRunning) {
        this->MyThreadRunning = false;
        this->MyThread.join();
    }
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
        this->KT = moduleManager->FindModule<MulNX::KeyTracker>("KeyTracker");
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
bool MulNX::ModuleBase::CreateUINode() {
    try {
        // 创建UI节点
        MulNXUINode UINode = MulNXUINode::Create(this);
        // 设置UI节点属性
        UINode.name = this->GetName();
        UINode.MyFunc = [this](MulNXUINode* ThisNode) {
            return this->UINodeFunc(ThisNode);
            };
        // 创建UI消息
        auto msg = MulNX::Message::Create<MulNXUINode>("UISystem/ModulePush"_hash, std::move(UINode));
        // 发送UI消息
        this->ISys().PublishAsync(std::move(msg));
    }
    catch (...) {
        return false;
    }
    return true;
}
bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* Core) {
    this->Core = Core;
    if (!this->BaseInit()) {
        return false;
    }
    if (!this->Init()) {
        return false;
    }
    if(this->NeedUINode) {
        if (!this->CreateUINode()) {
            return false;
        }
        this->ISys().LogInfo("推送了UI节点到UI系统");
    }
    if (this->InitNeedThread) {
        if (!this->CreateThread()) {
            return false;
        }
        this->ISys().LogInfo("申请了一个线程创建");
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
// 线程创建
void MulNX::ModuleBase::NeedThread(int TimeDelta) {
    this->InitNeedThread = true;
    this->SetMyThreadDelta(TimeDelta);
}
bool MulNX::ModuleBase::CreateThread() {
    for (;;) {
        //如果线程已经创建，则立即返回
        if (this->MyThreadRunning) return true;
        //尝试创建线程
        try {
            //先设置线程处于启动状态
            this->MyThreadRunning = true;
            //打开线程
            this->MyThread = std::thread([this]() {this->ThreadMain();});
            //如果成功返回成功
            return true;
        }
        //捕获所有创建异常
        catch (...) {
            //先把运行置空
            this->MyThreadRunning = false;
            //重新尝试，直到成功
            continue;
        }
    }
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