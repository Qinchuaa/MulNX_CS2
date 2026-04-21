#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/Elements/Elements.hpp>
#include "ElementConfig.hpp"

#include <filesystem>

class SolutionManager;
class ProjectManager;

class ElementDebugger;

//元素管理器，用于管理元素
class ElementManager final : public MulNX::ModuleBase {
    //对于元素，我们只给出三个通用接口：创建、获取、删除，具体的各个元素的功能由各个元素类自己实现
private:
    CameraDrawer* CamDrawer = nullptr;
    SolutionManager* SManager = nullptr;
    ProjectManager* PManager = nullptr;

    // 当前操作的元素指针
    std::atomic<std::shared_ptr<ElementBase>> CurrentElement = nullptr;
public:
    ElementConfig Config{};
    // 使用智能指针存储多态对象，以存储不同类型的元素
    std::unordered_map<std::string, std::shared_ptr<ElementBase>> elements;

    // 预览相关

    // 预览时间偏移
    float Preview_TimeSchema{};
    // 预览结束时间点
    float Preview_EndTime{};
    // 当前预览元素指针
    std::shared_ptr<ElementBase> Preview_CurrentElement;
    // 是否处于预览状态
    bool OnPreview = false;


    bool Init()override;
    bool MenuElement(MulNX::UINode* node);
    bool UINodeFunc(MulNX::UINode* node);
    void ProcessMsg(MulNX::Message& msg)override;
    void HandleUpdate();

    //创建元素函数，支持传递任意参数给元素构造函数
    ElementBase* Element_Create(const ElementType type, const std::string& name);
    // 保存所有元素到磁盘文件
    bool Element_SaveAll();
    // 从磁盘文件加载元素的预处理函数，内部会创建对应类型的元素，并调用具体加载函数加载信息
    bool Element_Load(const std::filesystem::path& FullPath);
    // 删除函数元素，返回true表示名称现在可用
    bool Element_Delete(const std::string Name);
    // 清空所有元素
    bool Element_ClearAll();
    // 展示单个元素信息在一行上
    void Element_ShowInLine(const std::shared_ptr<ElementBase> element);
    // 展示某个元素的详细信息到调试窗口
    void Element_ShowMsgToDebugMenu(const std::shared_ptr<ElementBase> element);

    //预览功能相关：
    //启用预览
    void Preview_Enable();
    //禁用预览
    void Preview_Disable();
    //切换预览元素
    void Preview_SetElement(const std::string& Name);
    //设置预览偏移
    void Preview_SetPreviewSchema(const float Time);
    //预览Call
    bool Preview_Call(CameraSystemIO* IO);
};