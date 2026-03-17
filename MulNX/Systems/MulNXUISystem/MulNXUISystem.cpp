#include "MulNXUISystem.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Core.hpp>

#include "../MessageManager/IMessageManager.hpp"
#include "../HandleSystem/HandleSystem.hpp"
#include "../KeyTracker/KeyTracker.hpp"
#include "../MulNXGlobalVars/MulNXGlobalVars.hpp"

#include <Windows.h>

bool MulNX::UISystem::Init() {
    this->ISys()
        .SubscribeAsync("UISystem/Start")
        .SubscribeAsync("UISystem/ModulePush");
    this->UIContext.Core = this->Core;
    return true;
}

void MulNX::UISystem::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "UISystem/Start"_hash: {
        std::string* pStr = Msg.asp.get<std::string>();
        this->UIContext.EntryDraw = std::move(*pStr);
        this->UISystemRunning = true;
        break;
    }
    case "UISystem/ModulePush"_hash: {
        MulNXUINode* node = Msg.asp.get<MulNXUINode>();
        this->UIContext.AddUINode(node->hSelf, std::move(*node));
        this->ISys().LogSucc("接收到一个UI节点");
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