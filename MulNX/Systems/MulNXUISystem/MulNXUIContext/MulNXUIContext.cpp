#include "MulNXUIContext.hpp"

#include <MulNXThirdParty/All_ImGui.hpp>

bool MulNXUIContext::CallUINode(const std::string& Name) {
    auto ItEntry = this->CallMap.find(Name);
    if (ItEntry == this->CallMap.end())return false;
    MulNXHandle& hUINode = ItEntry->second;
    auto ItUINode = this->UINodeMap.find(hUINode);
    if (ItUINode == this->UINodeMap.end())return false;
    MulNXUINode& UINode = ItUINode->second;
    UINode.Draw();
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
    this->Next = this->EntryDraw;
    bool CallResult = true;
    while (CallResult) {
        std::string current = this->Next;
        this->Next = std::string{};
        CallResult = this->CallUINode(current);
    }
}
void MulNXUIContext::AddUINode(MulNXHandle hUINode, MulNXUINode&& UINode) {
    this->CallMap[UINode.name] = hUINode;
    UINode.MainContext = this;
    this->UINodeMap[hUINode] = std::move(UINode);
}
MulNXUINode* MulNXUIContext::GetUINode(const MulNXHandle& hUINode) {
    auto It = this->UINodeMap.find(hUINode);
    if (It != this->UINodeMap.end()) {
        return &(It->second);
    }
    return nullptr;
}