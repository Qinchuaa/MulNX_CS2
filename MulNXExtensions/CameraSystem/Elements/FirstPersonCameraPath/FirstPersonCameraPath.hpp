#pragma once

#include <MulNXExtensions/CameraSystem/Elements/ElementBase.hpp>

class FirstPersonCameraPath final :public ElementBase {
public:
	static constexpr ElementType TemplateType = ElementType::FirstPersonCameraPath;

    uint8_t TargetPlayerIndexInMap = 0;

    //构造函数
    explicit FirstPersonCameraPath(const std::string& name) :
        ElementBase(name) {
        this->Type = ElementType::FirstPersonCameraPath;
    }

    //虚函数：

    //析构函数（虚）
    ~FirstPersonCameraPath()override = default;
    //调用函数（虚）（Mode:0为默认，1自动减去头时间）
    bool Call(CameraSystemIO* IO)const override final;
    //刷新状态（虚）
    void Refresh()override final;
    //信息获取函数（虚）
    std::string GetMsg()const override final;

    std::pair<bool, std::string> SaveImpl(YAML::Node& root)override;

    std::pair<bool, std::string> Load(YAML::Node& root)override{
        return { {},{} };
    }

    void DebugUI(CameraDrawer* CamDrawer, ElementManager* EManager)override;
};