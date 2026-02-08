#include "ModuleBase.hpp"

#include "../Core.hpp"
#include "../CoreImpl.hpp"

#include "../../Systems/MessageManager/IMessageManager.hpp"
#include "../../Systems/HandleSystem/IHandleSystem.hpp"

#include "../../ThirdParty/All_ImGui.hpp"

//构造与析构函数，线程自动析构
MulNX::ModuleBase::ModuleBase() {

}
MulNX::ModuleBase::~ModuleBase() {
    this->CloseMyThread();
}
void MulNX::ModuleBase::CloseMyThread() {
    std::unique_lock lock(this->MyThreadMutex);
    if (this->MyThreadRunning) {
        this->MyThreadRunning = false;
        this->MyThread.join();
    }
    return;
}

//模块自用
void MulNX::ModuleBase::SetMyThreadDelta(int Delta) {
    this->MyThreadDelta = Delta;
}

//默认虚函数实现

void MulNX::ModuleBase::VirtualMain() {
    return;
}
void MulNX::ModuleBase::ThreadMain() {
    return;
}
void MulNX::ModuleBase::ProcessMsg(MulNX::Message* Msg) {
    return;
}
void MulNX::ModuleBase::Menu() {
    return;
}
void MulNX::ModuleBase::Windows() {
    return;
}


//窗口控制

void MulNX::ModuleBase::OpenWindow() {
    this->ShowWindow = true;
}
void MulNX::ModuleBase::CloseWindow() {
    this->ShowWindow = false;
}
bool MulNX::ModuleBase::IsWindowOpen()const {
    return this->ShowWindow;
}

//基本函数
bool MulNX::ModuleBase::BaseInit() {
    try {
        this->IMsgManager = &this->Core->pImpl->MessageManager;
        this->IDebugger = &this->Core->pImpl->Debugger;
        this->GlobalVars = &this->Core->pImpl->GlobalVars;
        this->AL3D = &this->Core->pImpl->AL3D;
        this->KT = &this->Core->pImpl->KT;

        if (!this->HModule.IsValid()) {
            this->HModule = MulNXHandle::CreateHandle();
        }
        this->IRegiste();
    }
    catch (...) {
        return false;
    }

    return true;
}
void MulNX::ModuleBase::BaseVirtualMain() {
    return;
}
void MulNX::ModuleBase::BaseProcessMsg() {
    MulNX::IMessageChannel* Channel = this->MainMsgChannel;
    if (Channel != nullptr) {
        MulNX::Message Msg{ MulNX::MsgType::Null };
        while (Channel->PullMessage(Msg)) {
            this->ProcessMsg(&Msg);
        }
    }
    return;
}
void MulNX::ModuleBase::BaseMenu() {
    return;
}
void MulNX::ModuleBase::BaseWindows() {
    return;
}


//各类入口点函数
bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* Core) {
    this->Core = Core;
    if (!this->BaseInit()) {
        return false;
    }
    if (!this->Init()) {
        return false;
    }
    return true;
}
void MulNX::ModuleBase::EntryVirtualMain() {
    this->BaseVirtualMain();
    this->VirtualMain();
}
bool MulNX::ModuleBase::EntryCreateThread() {
    for (;;) {
        //如果线程已经创建，则立即返回
        if (this->MyThreadRunning) return true;
        //尝试创建线程
        try {
            //先设置线程处于启动状态
            this->MyThreadRunning = true;
            //打开线程
            this->MyThread = std::thread([this]() {
                while (this->MyThreadRunning) {
                    this->ThreadMain();
                    std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
                }
                });
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
void MulNX::ModuleBase::EntryProcessMsg() {
    this->BaseProcessMsg();
    auto pMsg = this->CurrentMsg.load();
    if (pMsg) {
        this->ProcessMsg(pMsg);
        this->CurrentMsg.store(nullptr);
        this->IMsgManager->Release();
    }
}
void MulNX::ModuleBase::EntryMenu() {
    this->BaseMenu();
    this->Menu();
}
void MulNX::ModuleBase::EntryWindows() {
    this->BaseWindows();
    this->Windows();
}

void MulNX::ModuleBase::IRegiste() {
    this->IMsgManager->Registe(this);
}
void MulNX::ModuleBase::ISubscribe(MulNX::MsgType MsgType) {
    this->IMsgManager->Subscribe(MsgType, this);
}
void MulNX::ModuleBase::IPublish(MulNX::Message&& Msg) {
    this->IMsgManager->Publish(std::move(Msg));
}
void MulNX::ModuleBase::IPublish(MulNX::MsgType Type) {
    //消息管理器会负责消息生命周期
    MulNX::Message Msg(Type);
    this->IMsgManager->Publish(std::move(Msg));
}
MulNX::IMessageChannel* MulNX::ModuleBase::ICreateAndGetMessageChannel() {
    return this->IMsgManager->GetMessageChannel(this->IMsgManager->CreateMessageChannel());
}

MulNX::AutoChild::AutoChild(ModuleBase* Module, const std::string& Name, const float HeightRatio, const float WidthRatio)
    :Module(Module) {
    ImVec2 Available = ImGui::GetContentRegionAvail();
    ImGui::BeginChild(Name.c_str(), ImVec2(Available.x * WidthRatio, Available.y * HeightRatio), true);
}
MulNX::AutoChild::~AutoChild() {
    ImGui::EndChild();
}