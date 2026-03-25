#include "FirstPersonCameraPath.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <sstream>

bool FirstPersonCameraPath::Call(CameraSystemIO* IO)const {
    //如果不在理论影响范围内，应当直接返回而不做任何修改
    //是与被遍历的其它call一起工作
    if (!this->TargetPlayerIndexInMap)return false;

	float Time;

    IO->Frame.TargetOBMode = 2;

    IO->Frame.TargetPlayerIndexInMap = this->TargetPlayerIndexInMap;

    return true;
}
void FirstPersonCameraPath::Refresh() {
    if (this->TargetPlayerIndexInMap == 0) {
        this->StartTime = 0;
        this->EndTime = 0;
        this->DurationTime = 0;
        return;
    }
    else {
        this->DurationTime = this->EndTime - this->StartTime;
        return;
    }
    this->Dirty = true;
    return;
}

std::string FirstPersonCameraPath::GetMsg()const {
    std::ostringstream oss;
    oss << this->GetBaseMsg()
        << "\n目标玩家索引：" << static_cast<int>(this->TargetPlayerIndexInMap);

    return oss.str();
}


// std::pair<bool, std::string> FirstPersonCameraPath::ReadElementMain(const pugi::xml_node& node_ElementMain) {
//     pugi::xml_node node_Target = node_ElementMain.child("Target");
//     if (!node_Target)return { false,"找不到Target节点！ 元素名：" + this->Name };
    
//     this->TargetPlayerIndexInMap = static_cast<uint8_t>(node_Target.attribute("PlayerIndex").as_int());
// 	this->StartTime = node_Target.attribute("StartTime").as_float();
// 	this->EndTime = node_Target.attribute("EndTime").as_float();

//     return { true,"成功读取第一人称摄像机轨道：" + this->Name };
// }
// std::pair<bool, std::string> FirstPersonCameraPath::SaveToXML(const std::filesystem::path& FolderPath)const {
//     if (FolderPath.empty())return { false,"文件夹路径为空，无法保存元素到XML文件！" };
    
//     pugi::xml_document XML;
//     pugi::xml_node node_ElementMain;
//     auto [ok, msg] = this->SaveBase(XML, node_ElementMain);
//     if (!ok)return { false,std::move(msg) };
    
//     std::filesystem::path FullPath = FolderPath / (this->Name + ".xml");
    
//     pugi::xml_node node_Target = node_ElementMain.append_child("Target");

// 	node_Target.append_attribute("PlayerIndex") = static_cast<int>(this->TargetPlayerIndexInMap);
// 	node_Target.append_attribute("StartTime") = this->StartTime;
// 	node_Target.append_attribute("EndTime") = this->EndTime;

//     // 保存XML到文件
//     if (!XML.save_file(FullPath.c_str()))return { false,"尝试保存到XML文件失败，无法保存XML文件！ 文件路径：" + FullPath.string() };
    
//     return { true,"成功保存到XML文件！ 文件路径：" + FullPath.string() };
// }

std::pair<bool, std::string> FirstPersonCameraPath::SaveImpl(YAML::Node& node) {
    return { {},{} };
}

void FirstPersonCameraPath::DebugUI(CameraDrawer* CamDrawer, ElementManager* EManager) {
    ImGui::Text(this->GetMsg().c_str());
    ImGui::Separator();
    static int TargetPlayerIndex = this->TargetPlayerIndexInMap;
    ImGui::SliderInt("目标玩家索引", &TargetPlayerIndex, 0, 10);
    if (ImGui::Button("应用索引")) {
        this->TargetPlayerIndexInMap = static_cast<uint8_t>(TargetPlayerIndex);
        this->Refresh();
    }
    static float StartBuffer;
    ImGui::InputFloat("开始时间", &StartBuffer);
    static float EndBuffer;
    ImGui::InputFloat("结束时间", &EndBuffer);
    if (ImGui::Button("应用时间")) {
        this->StartTime = StartBuffer;
        this->EndTime = EndBuffer;
        this->Refresh();
    }
    if (ImGui::Button("测试切换")) {
        EManager->AL3D->SpecPlayer(this->TargetPlayerIndexInMap);
    }

    return;
}