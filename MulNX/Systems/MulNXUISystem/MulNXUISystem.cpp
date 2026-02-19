#include "MulNXUISystem.hpp"

#include "../MessageManager/IMessageManager.hpp"
#include "../IPCer/IPCer.hpp"
#include "../HandleSystem/HandleSystem.hpp"
#include "../KeyTracker/KeyTracker.hpp"
#include "../MulNXGlobalVars/MulNXGlobalVars.hpp"

#include "../../Core/Core.hpp"

#include <MulNXThirdParty/All_ImGui.hpp>

bool MulNX::UISystem::Init() {
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();
    this->ISys()
        .SubscribeAsync(MulNX::MsgType::UISystem_Start)
        .SubscribeAsync(MulNX::MsgType::UISystem_ModulePush);
    this->UIContext.Core = this->Core;
    return true;
}

void MulNX::UISystem::ProcessMsg(MulNX::Message* Msg) {
    switch (Msg->Type) {
    case MulNX::MsgType::UISystem_Start: {
        MulNX::Base::any_unique_ptr EntryStr = this->Core->IHandleSystem().ReleaseUnique(Msg->Handle);
        std::string* pStr = EntryStr.get<std::string>();
        this->UIContext.EntryDraw = std::move(*pStr);
        this->UISystemRunning = true;
        break;
    }
    case MulNX::MsgType::UISystem_ModulePush: {
        MulNX::Base::any_unique_ptr UINode = this->Core->IHandleSystem().ReleaseUnique(Msg->Handle);
        this->UIContext.AddUINode(Msg->Handle, std::move(UINode));
        break;
    }
    }
}

int MulNX::UISystem::Render() {
    std::unique_lock lock(this->UIMtx);
    this->EntryProcessMsg();

    if (!this->UISystemRunning) {
        return 0;
    }

    this->FrameBefore();

    if (this->KT->CheckComboClick(VK_INSERT, 3)) {
        this->UIContext.Active = !this->UIContext.Active;
    }
    if (this->UIContext.Active) {
        this->UIContext.Draw();
    }
    this->Core->VirtualMain();
    this->FrameBehind();

    return 0;
}

void MulNX::UISystem::SetFrameBefore(std::function<void(void)>Before) {
    this->FrameBefore = Before;
    return;
}
void MulNX::UISystem::SetFrameBehind(std::function<void(void)>Behind) {
    this->FrameBehind = Behind;
    return;
}