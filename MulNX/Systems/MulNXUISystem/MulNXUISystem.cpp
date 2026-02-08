#include "MulNXUISystem.hpp"

#include "../MessageManager/IMessageManager.hpp"
#include "../IPCer/IPCer.hpp"
#include "../HandleSystem/HandleSystem.hpp"
#include "../KeyTracker/KeyTracker.hpp"
#include "../MulNXiGlobalVars/MulNXiGlobalVars.hpp"

#include "../../Core/Core.hpp"

#include "../../ThirdParty/All_ImGui.hpp"

bool MulNX::UISystem::Init() {
    this->MainMsgChannel = this->ICreateAndGetMessageChannel();
    (*this->MainMsgChannel)
        .Subscribe(MulNX::MsgType::UISystem_Start)
        .Subscribe(MulNX::MsgType::UISystem_ModulePush);
    this->UIContext.Core = this->Core;
    return true;
}

void MulNX::UISystem::ProcessMsg(MulNX::Message* Msg) {
    switch (Msg->Type) {
    case MulNX::MsgType::UISystem_Start: {
        MulNX::Base::any_unique_ptr pEntryStr = this->Core->IHandleSystem().ReleaseUnique(Msg->Handle);
        std::string* pStr = pEntryStr.get<std::string>();
        this->UIContext.EntryDraw = std::move(*pStr);
        this->UISystemRunning = true;
        break;
    }
    case MulNX::MsgType::UISystem_ModulePush: {
        MulNX::Base::any_unique_ptr pCtx = this->Core->IHandleSystem().ReleaseUnique(Msg->Handle);
        this->UIContext.AddSingleContext(Msg->Handle, std::move(pCtx));
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