#include "ElementManager.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CameraSystem/SolutionManager/SolutionManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>

bool ElementManager::UINodeFunc(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("元素调试", this->ShowWindow);
    if (!w)return true;
    // 检查当前是否有操作元素
    if (this->CurrentElement) {
        // 根据元素类型调用不同的调试菜单
        this->CurrentElement->DebugUI(this->CamDrawer, this);
    }
    // 如果没有操作元素
    else {
        ImGui::Text("当前未选择任何元素");
    }
    return true;
}
//元素管理器基本函数
bool ElementManager::Init() {
    this->CamDrawer = &this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->UINodeFunc(node);});

    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("Elements", "Elements",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Elements");
            this->ISys().LogSucc("成功设置元素路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Elements", "CurrentProject");
    }
    return true;
}
void ElementManager::VirtualMain() {
    for (auto& elem : this->Elements) {
        elem->DrawBase(this->CamDrawer, this->AL3D->GetViewMatrix(), this->AL3D->GetWinWidth(), this->AL3D->GetWinHeight());
    }

    if (this->OnPreview) {
        CameraSystemIO IO;
        IO.ElementTime = this->AL3D->Time()->GetReal();
        IO.FrameGameTime = this->AL3D->Time()->GetReal();
        if (this->Preview_Call(&IO)) {
            //自由摄像机轨道预览
            if (this->Preview_CurrentElement->Type == ElementType::FreeCameraPath) {
                if (this->Config.PreviewOverride) {
                    this->AL3D->CameraSystemIOOverride(&IO);
                }
                else if (this->Config.PreviewDraw) {
                    this->CamDrawer->DrawFrameCamera(IO.Frame, "预览摄像机");
                }
            }
        }
        //其它类型预览
    }
}
//ElementBase，Create和Get已在头文件中实现

std::vector<std::shared_ptr<ElementBase>>::iterator ElementManager::Element_GetIterator(const std::string_view Name) {
    return std::find_if(Elements.begin(), Elements.end(),
        [&Name](const std::shared_ptr<ElementBase>& elem) {
            return elem->Name == Name;
        });
}
bool ElementManager::Element_SaveAll() {
    //检查是否有元素
    if (this->Elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过保存操作！");
        return true;
    }
    std::filesystem::path ElementFolderPath = this->ISys().PathManager()->PathGetFromKey("Elements");
    //遍历所有元素并保存
    for (const auto& elem : this->Elements) {
        if (!elem->Dirty) {
            //如果不脏则跳过保存
            continue;
        }
        auto [ok, msg] = elem->Save(ElementFolderPath);
        if (ok) {
            this->ISys().LogSucc(std::move(msg));
        }
        else {
            this->ISys().LogError(std::move(msg));
            return false;
        }
    }
    this->ISys().LogSucc("成功保存所有元素到磁盘！");
    return true;
}
bool ElementManager::Element_Load(const std::filesystem::path& FullPath) {
    this->ISys().LogInfo("尝试从磁盘文件加载元素，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("磁盘文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());

        // 获取元素类型
        std::string NewElementTypeString = root["type"].as<std::string>();
        ElementType NewElementType = ElementType_StringToEnum(NewElementTypeString);
        if (static_cast<int>(NewElementType) <= 0) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，不可加载的元素类型！");
            return false;
        }

        // 获取元素名称
        std::string NewElementName = root["name"].as<std::string>();
        // 检查元素名是否为空
        if (NewElementName.empty()) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，元素名称为空！");
            return false;
        }
        // 检查是否存在同名元素
        if (this->Element_Get<ElementBase>(NewElementName)) {
            this->ISys().LogError("元素名已占用，无法从磁盘文件加载元素！ 元素名：" + NewElementName);
            return false;
        }
        // 创建基类指针
        std::shared_ptr<ElementBase> pElement = nullptr;
        this->ISys().LogInfo("尝试进行分发，元素类型为 " + NewElementTypeString + " ，文件路径：" + FullPath.string());
        // 分发到具体类型的加载函数
        switch (NewElementType) {
        case ElementType::FreeCameraPath:
            pElement = std::make_shared<FreeCameraPath>(std::move(NewElementName));
            break;
        case ElementType::ElementBase:
            break;
        case ElementType::None:
            break;
        }

        // 判空
        if (!pElement) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，无法创建指定类型的元素实例！ 元素类型：" + NewElementTypeString);
            return false;
        }
        // 设置元素类型
        pElement->Type = NewElementType;
        // 统一加载信息
        auto [ok, msg] = pElement->Load(root);
        if (!ok) {
            this->ISys().LogError(std::move(msg));
            return false;
        }
        pElement->Refresh();
        pElement->Dirty = false;// 刚刚进入内存，非脏
        this->ISys().LogSucc(std::move(msg));
        this->Elements.push_back(std::move(pElement));
        return true;
    }
    catch (...) {
        MulNX::ErrorTerminate("元素加载异常");
    }
}
bool ElementManager::Element_Delete(const std::string Name) {
    // 安全检查
    if (Name.empty()) {
        this->ISys().LogError("尝试删除空名称的元素！");
        return false;
    }

    // 获取迭代器
    auto it = this->Element_GetIterator(Name);
    // 判空
    if (it == this->Elements.end()) {
        this->ISys().LogError("未找到指定名称的元素：" + Name);
        return false;
    }

    // 检查是否正在预览此元素
    if (this->Preview_CurrentElement && this->Preview_CurrentElement->Name == Name) {
        this->Preview_Disable(); // 禁用预览
    }

    // 检查是否当前正在操作此元素
    if (this->CurrentElement && this->CurrentElement->Name == Name) {
        this->CurrentElement = nullptr;
    }

    // 先标记为需要清理
    it->get()->NeedBeDelete = true;
    // 通过迭代器删除元素
    Elements.erase(it);
    // 添加刷新信息
    this->SManager->NeedRefresh = true;

    this->ISys().LogSucc("成功删除元素：" + Name);
    return true;
}
bool ElementManager::Element_ClearAll() {
    // 检查是否有元素
    if (this->Elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过清空操作！");
        return true;
    }
    // 禁用预览
    this->Preview_Disable();
    this->Preview_CurrentElement = nullptr;
    // 清空当前操作元素
    this->CurrentElement = nullptr;
    // 把所有元素标记为需要清理并从Elements中释放
    for (auto& elem : this->Elements) {
        elem->NeedBeDelete = true;
    }
    this->Elements.clear();
    // 添加刷新信息
    this->SManager->NeedRefresh = true;
    this->ISys().LogSucc("成功清空所有元素！");
    return true;
}
void ElementManager::Element_ShowInLine(const std::shared_ptr<ElementBase> element) {
    if (!element) {
        return;
    }
    ImGui::Text("|元素名称：");
    ImGui::SameLine();

    if (ImGui::Selectable(element->Name.data(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentElement = element;
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }

    if (ImGui::BeginPopupContextItem(("右键菜单" + element->Name).c_str())) {
        if (ImGui::MenuItem("复制名称")) {
            ImGui::SetClipboardText(element->Name.c_str());
        }
        if (element->Drawable) {
            ImGui::Checkbox("绘制", &element->IfDraw);
        }
        if (ImGui::MenuItem("打印元素信息到调试窗口")) {
            this->Element_ShowMsgToDebugMenu(element);
        }
        if (ImGui::MenuItem("保存到磁盘")) {
            auto path = this->ISys().PathManager()->PathGetFromKey("Elements");
            auto [ok, msg] = element->Save(path);
            if (ok) {
                this->ISys().LogSucc(std::move(msg));
            }
            else {
                this->ISys().LogError(std::move(msg));
            }
        }
        if (ImGui::MenuItem("删除元素")) {
            this->Element_Delete(element->Name);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text(("   元素类型：" + element->TypeGet_String() + "   持续时长：" + std::to_string(element->DurationTime)).c_str());
}

void ElementManager::Element_ShowMsgToDebugMenu(const std::shared_ptr<ElementBase> element) {
    this->ISys().LogLine();
    this->ISys().LogInfo("元素名称：" + element->Name);
    this->ISys().LogInfo("元素类型：" + element->TypeGet_String());
    this->ISys().LogInfo("持续时长：" + std::to_string(element->DurationTime));
    this->ISys().LogInfo("详细信息：");

    this->ISys().LogInfo(element->GetMsg());
    this->ISys().LogLine();
}
std::vector<std::string> ElementManager::Element_GetNames()const {
    std::vector<std::string> ElementsNames;
    if (this->Elements.empty())return ElementsNames;
    ElementsNames.reserve(this->Elements.size());
    for (size_t i = 0; i < this->Elements.size(); ++i) {
        ElementsNames.push_back(this->Elements[i]->Name);
    }
    return ElementsNames;
}



//预览相关
void ElementManager::Preview_Enable() {
    if (!this->Preview_CurrentElement) {
        this->ISys().LogError("无法开启预览：未设置预览元素！");
        return;
    }
    this->OnPreview = true;
    this->ISys().LogInfo("已开启预览");
}
void ElementManager::Preview_Disable() {
    this->OnPreview = false;
    this->AL3D->ClearViewOverride();
    this->ISys().LogInfo("已关闭预览");
}
void ElementManager::Preview_SetElement(const std::string& Name) {
    std::shared_ptr<ElementBase>element = this->Element_Get<ElementBase>(Name);
    if (!element) {
        this->ISys().LogError("找不到目标元素   元素名：" + Name);
        return;
    }
    this->Preview_CurrentElement = element;
    this->ISys().LogInfo("准备预览该元素   元素名：" + Name);
}
void ElementManager::Preview_SetPreviewSchema(const float Time) {
    this->Preview_TimeSchema = Time;
    this->ISys().LogInfo("元素预览时间偏移设置为：" + std::to_string(this->Preview_TimeSchema));
    this->Preview_EndTime = this->Preview_CurrentElement->StartTime + this->Preview_CurrentElement->DurationTime;
}
bool ElementManager::Preview_Call(CameraSystemIO* IO) {
    if (!this->OnPreview)return false;
    if (this->Preview_CurrentElement) {
        IO->ElementTime += this->Preview_CurrentElement->GetStartTime() - this->Preview_TimeSchema;
        if (!this->Preview_CurrentElement->CalculateFrame(IO)) {
            this->Preview_Disable();
            return false;
        }
        return true;
    }
    else {
        this->Preview_Disable();
        return false;
    }
}