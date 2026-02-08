#include "MulNXUIContext.hpp"

#include "../../../ThirdParty/All_ImGui.hpp"

bool MulNXUIContext::CallSingleContext(const std::string& Name) {
    auto ItEntry = this->CallMap.find(Name);
    if (ItEntry == this->CallMap.end())return false;
    MulNXHandle& hContext = ItEntry->second;
    auto ItSContext = this->ContextMap.find(hContext);
    if (ItSContext == this->ContextMap.end())return false;
    MulNX::Base::any_unique_ptr& SContext = ItSContext->second;
    MulNXSingleUIContext* pSContext = SContext.get<MulNXSingleUIContext>();
    pSContext->Draw();
    return true;
}

void MulNXUIContext::Draw() {
    if (this->EnableErrorHandle) {
        ImGui::Begin("错误");
        ImGui::Text("请等待响应");
        if (ImGui::Button("关闭")) {
            this->EnableErrorHandle = false;
        }
        ImGui::End();
    }
    this->next = this->EntryDraw;
    bool CallResult = true;
    while (CallResult) {
        std::string current = this->next;
        this->next = std::string{};
        CallResult = this->CallSingleContext(current);
    }

    /*for (const auto& It : this->ContextOrder) {
        auto& SContext = this->ContextMap[It];
        MulNXSingleUIContext* SContextPtr = SContext.get<MulNXSingleUIContext>();
        SContextPtr->Draw();
    }*/
}
void MulNXUIContext::AddSingleContext(MulNXHandle hContext, MulNX::Base::any_unique_ptr SContext) {
    MulNXSingleUIContext* SContextPtr = SContext.get<MulNXSingleUIContext>();
    this->CallMap[SContextPtr->name] = hContext;
    this->ContextOrder.push_back(hContext);
    SContextPtr->MainContext = this;
    this->ContextMap[hContext] = std::move(SContext);
}
MulNXSingleUIContext* MulNXUIContext::GetSingleContext(const MulNXHandle& hContext) {
    auto It = this->ContextMap.find(hContext);
    if (It != this->ContextMap.end()) {
        return It->second.get<MulNXSingleUIContext>();
    }
    return nullptr;
}