#include "ElementManager.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CameraSystem/SolutionManager/SolutionManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>
#include <algorithm>

bool ElementManager::MenuElement(MulNX::UINode* node) {
    std::vector<std::string> groups = this->GetElementGroups();
    // 展示预览功能相关状态
    ImGui::TextUnformatted(I18n(
        "camsys.elem.preview_status",
        this->OnPreview ? I18n("text.opened") : I18n("text.closed"),
        this->Preview_CurrentElement ? this->Preview_CurrentElement->GetName() : I18n("text.none"),
        this->Preview_TimeSchema
    ).c_str());

    ImGui::Separator();

    // 元素总设置
    if (ImGui::CollapsingHeader(I18n("camsys.elem.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.elem.preview_draw").c_str(), &this->Config.PreviewDraw);
        ImGui::Checkbox(I18n("camsys.elem.preview_override").c_str(), &this->Config.PreviewOverride);
    }

    // 创建元素
    if (ImGui::CollapsingHeader(I18n("camsys.elem.create").c_str())) {
        ImGui::Text("目标元素组");
        ImGui::SameLine();
        const std::string currentGroupLabel = this->CurrentElementGroup.empty() ? "Elements" : this->CurrentElementGroup;
        if (ImGui::BeginCombo("##当前元素组", currentGroupLabel.c_str())) {
            const bool selectedRoot = this->CurrentElementGroup.empty();
            if (ImGui::Selectable("Elements", selectedRoot)) {
                this->CurrentElementGroup.clear();
            }
            for (const auto& groupName : groups) {
                const bool isSelected = this->CurrentElementGroup == groupName;
                if (ImGui::Selectable(groupName.c_str(), isSelected)) {
                    this->CurrentElementGroup = groupName;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Text(I18n("camsys.elem.new_name").c_str());
        ImGui::SameLine();
        static std::string newElementName = "";
        ImGui::InputText("##新元素名", &newElementName);
        // 创建自由摄像机轨道
        if (ImGui::Button(I18n("camsys.elem.new_free_camera_path").c_str())) {
            if (newElementName.empty()) {
                this->ISys().LogError(I18n("result.error_empty_name").c_str());
            }
            else {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Create"_hash);
                rp->str1 = std::move(newElementName);
                rp->str2 = this->CurrentElementGroup;
                this->ISys().PublishAsync(std::move(msg));
            }
            newElementName.clear();
        }

        static std::string newGroupName = "";
        ImGui::Separator();
        ImGui::Text("新元素组");
        ImGui::SameLine();
        ImGui::InputText("##新元素组名", &newGroupName);
        ImGui::SameLine();
        if (ImGui::Button("创建元素组")) {
            if (newGroupName.empty()) {
                this->ISys().LogError(I18n("result.error_empty_name").c_str());
            }
            else if (this->ElementGroup_Create(newGroupName)) {
                this->CurrentElementGroup = newGroupName;
            }
            newGroupName.clear();
        }
    }
    // 展示修改元素
    if (ImGui::CollapsingHeader(I18n("camsys.elem.list").c_str())) {
        if (ImGui::Selectable("Elements##选择根元素", this->CurrentElementGroup.empty())) {
            this->CurrentElementGroup.clear();
        }
        for (const auto& [name, element] : this->elements) {
            if (!element->InGroup()) {
                this->Element_ShowInLine(element);
            }
        }
        for (const auto& groupName : groups) {
            this->ElementGroup_ShowInLine(groupName);
        }
    }

    return true;
}

void ElementManager::Element_ShowInLine(const std::shared_ptr<ElementBase> element) {
    ImGui::Text(I18n("camsys.elem.name_label").c_str());
    ImGui::SameLine();

    if (ImGui::Selectable(element->Name.data(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentElement.store(element, std::memory_order_release);
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }

    if (ImGui::BeginPopupContextItem((I18n("camsys.elem.context_menu") + element->Name).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(element->Name.c_str());
        }
        if (element->Drawable) {
            MulNX::UI::Checkbox(I18n("camsys.elem.draw").c_str(), element->draw);
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Delete"_hash);
            rp->str1 = std::move(element->Name);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text(I18n("camsys.elem.type_duration", element->TypeGet_String(), std::to_string(element->DurationTime)).c_str());
}

void ElementManager::ElementGroup_ShowInLine(const std::string& groupName) {
    auto groupElements = this->GetElementsInGroup(groupName);
    std::sort(groupElements.begin(), groupElements.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->GetName() < rhs->GetName();
    });

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (this->CurrentElementGroup == groupName) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    const bool open = ImGui::TreeNodeEx((groupName + "##元素组").c_str(), flags, "%s (%d)", groupName.c_str(), static_cast<int>(groupElements.size()));
    if (ImGui::IsItemClicked()) {
        this->CurrentElementGroup = groupName;
    }
    if (!open) {
        return;
    }
    for (const auto& element : groupElements) {
        this->Element_ShowInLine(element);
    }
    ImGui::TreePop();
}

bool ElementManager::UINodeFunc(MulNX::UINode* node) {
    std::unique_lock lock(this->CamSys()->smutex);
    for (auto& [name, elem] : this->elements) {
        elem->DrawBase(this->CamDrawer, this->AL3D->GetViewMatrix(), this->AL3D->GetWinWidth(), this->AL3D->GetWinHeight());
    }
    if (this->needDrawCamera.load(std::memory_order_acquire)) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, I18n("camsys.elem.preview_draw_label").c_str());
    }
    auto w = MulNX::UI::RAIIWindow("元素调试", this->ShowWindow);
    if (!w)return true;
    // 检查当前是否有操作元素
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current) {
        // 根据元素类型调用不同的调试菜单
        current->DebugUI(this);
    }
    // 如果没有操作元素
    else {
        ImGui::Text(I18n("text.no_selected").c_str());
    }
    return true;
}
//元素管理器基本函数
bool ElementManager::Init() {
    this->CamDrawer = &this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendUINode("MenuElement", [this](MulNX::UINode* node) {return this->MenuElement(node);});

    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("Elements", "Elements",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Elements");
            this->ISys().LogSucc("成功设置元素路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Elements", "CurrentProject");
    }

    this->ISys()
        .SubscribeAsync("Element/Create")
        .SubscribeAsync("Element/Delete");

    return true;
}

void ElementManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Element/Create"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        auto& groupName = msg.asp.get<MulNX::NetExt>()->str2;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Element_Create(ElementType::FreeCameraPath, name, groupName)) {
            this->ISys().LogError(std::format("元素创建失败：{}", name));
        }
        break;
    }
    case "Element/Delete"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Element_Delete(name)) {
            this->ISys().LogError(std::format("元素删除失败：{}", name));
        }
        break;
    }
    }
}

void ElementManager::HandleUpdate() {
    this->EntryProcessMsg();
    if (this->PManager && this->PManager->ActiveProject) {
        for (const auto& [name, element] : this->elements) {
            auto bindingIt = this->PManager->ActiveProject->ElementKeybinds.find(name);
            if (bindingIt == this->PManager->ActiveProject->ElementKeybinds.end() || !bindingIt->second.Usable) {
                continue;
            }
            if (this->pInputSystem->CheckWithPack(bindingIt->second)) {
                this->Preview_SetElement(name);
                this->Preview_SetPreviewSchema(this->AL3D->Time()->GetReal());
                this->Preview_Enable();
                break;
            }
        }
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
                if (this->Config.PreviewDraw) {
                    auto frame = this->drawCamera.Write();
                    *frame = IO.Frame;
                    this->needDrawCamera.store(true, std::memory_order_release);
                }
                else {
                    this->needDrawCamera.store(false, std::memory_order_release);
                }
            }
        }
        //其它类型预览
    }
}
//ElementBase，Create和Get已在头文件中实现

//创建元素函数，支持传递任意参数给元素构造函数
ElementBase* ElementManager::Element_Create(const ElementType type, const std::string& name) {
    return this->Element_Create(type, name, "");
}

ElementBase* ElementManager::Element_Create(const ElementType type, const std::string& name, const std::string& groupName) {
    // 检查是否已存在同名元素
    if (this->elements.find(name) != this->elements.end()) {
        this->ISys().LogError("元素名已占用！ 元素名：" + name);
        return nullptr;
    }
    if (!groupName.empty() && !this->ElementGroup_Exists(groupName)) {
        this->ISys().LogError("目标元素组不存在，无法创建元素！ 元素组：" + groupName);
        return nullptr;
    }
    std::shared_ptr<ElementBase> pElement = nullptr;
    // 分发到具体类型的加载函数
    switch (type) {
    case ElementType::FreeCameraPath:
        pElement = std::make_shared<FreeCameraPath>(name);
        break;
    case ElementType::ElementBase:
        break;
    case ElementType::None:
        break;
    }
    // 输出成功信息
    this->ISys().LogSucc("成功创建元素！  元素名：" + name);
    // 设置元素类型
    pElement->Type = type;
    pElement->SetGroupName(groupName);
    // 添加进Elements
    this->elements[name] = std::move(pElement);
    return this->elements[name].get();
}

std::filesystem::path ElementManager::GetElementSaveFolder(const std::shared_ptr<ElementBase>& element) {
    std::filesystem::path elementsRoot = this->ISys().PathManager()->PathGetFromKey("Elements");
    if (!element || !element->InGroup()) {
        return elementsRoot;
    }
    return elementsRoot / element->GetGroupName();
}

bool ElementManager::RegisterGroupFromPath(const std::filesystem::path& groupPath) {
    const std::string groupName = groupPath.filename().string();
    if (groupName.empty()) {
        return false;
    }
    this->elementGroups.insert(groupName);
    return true;
}

bool ElementManager::ElementGroup_Create(const std::string& groupName) {
    if (groupName.empty()) {
        this->ISys().LogError("元素组名为空，无法创建元素组！");
        return false;
    }
    if (this->ElementGroup_Exists(groupName)) {
        this->ISys().LogWarning("元素组已存在：" + groupName);
        return true;
    }
    std::filesystem::path groupPath = this->ISys().PathManager()->PathGetFromKey("Elements") / groupName;
    try {
        std::filesystem::create_directories(groupPath);
        this->elementGroups.insert(groupName);
        this->ISys().LogSucc("成功创建元素组：" + groupName);
        return true;
    }
    catch (const std::exception& e) {
        this->ISys().LogError("创建元素组失败：" + groupName + "  原因：" + e.what());
        return false;
    }
}

bool ElementManager::ElementGroup_Exists(const std::string& groupName) const {
    return this->elementGroups.find(groupName) != this->elementGroups.end();
}

std::vector<std::string> ElementManager::GetElementGroups() const {
    std::vector<std::string> groups(this->elementGroups.begin(), this->elementGroups.end());
    std::sort(groups.begin(), groups.end());
    return groups;
}

float ElementManager::GetElementGroupMaxDuration(const std::string& groupName) const {
    float maxDuration = 0.0f;
    for (const auto& element : this->GetElementsInGroup(groupName)) {
        if (element) {
            maxDuration = std::max(maxDuration, element->GetDurationTime());
        }
    }
    return maxDuration;
}

std::vector<std::shared_ptr<ElementBase>> ElementManager::GetElementsInGroup(const std::string& groupName) const {
    std::vector<std::shared_ptr<ElementBase>> result;
    for (const auto& [name, element] : this->elements) {
        if (element && element->GetGroupName() == groupName) {
            result.push_back(element);
        }
    }
    return result;
}

bool ElementManager::Element_SaveAll() {
    std::filesystem::path ElementFolderPath = this->ISys().PathManager()->PathGetFromKey("Elements");
    std::filesystem::create_directories(ElementFolderPath);
    for (const auto& groupName : this->elementGroups) {
        std::filesystem::create_directories(ElementFolderPath / groupName);
    }
    //检查是否有元素
    if (this->elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过保存操作！");
        return true;
    }
    //遍历所有元素并保存
    for (const auto& [name, elem] : this->elements) {
        if (!elem->Dirty) {
            //如果不脏则跳过保存
            continue;
        }
        auto saveFolderPath = this->GetElementSaveFolder(elem);
        std::filesystem::create_directories(saveFolderPath);
        auto [ok, msg] = elem->Save(saveFolderPath);
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

bool ElementManager::Element_LoadAll(const std::filesystem::path& elementsRoot) {
    this->elementGroups.clear();
    if (!std::filesystem::exists(elementsRoot)) {
        this->ISys().LogWarning("元素目录不存在，跳过元素加载：" + elementsRoot.string());
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(elementsRoot)) {
        if (entry.is_directory()) {
            this->RegisterGroupFromPath(entry.path());
            for (const auto& groupEntry : std::filesystem::directory_iterator(entry.path())) {
                if (groupEntry.is_regular_file()) {
                    this->Element_Load(groupEntry.path());
                }
            }
        }
        else if (entry.is_regular_file()) {
            this->Element_Load(entry.path());
        }
    }
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
        if (this->elements.find(NewElementName) != this->elements.end()) {
            this->ISys().LogError("元素名已占用，无法从磁盘文件加载元素！ 元素名：" + NewElementName);
            return false;
        }
        // 创建基类指针
        this->ISys().LogInfo("尝试进行分发，元素类型为 " + NewElementTypeString + " ，文件路径：" + FullPath.string());

        std::string groupName;
        if (root["group"]) {
            groupName = root["group"].as<std::string>();
        }
        else {
            const auto elementsRoot = this->ISys().PathManager()->PathGetFromKey("Elements");
            const auto parentPath = FullPath.parent_path();
            if (parentPath != elementsRoot) {
                const auto relativeParent = std::filesystem::relative(parentPath, elementsRoot);
                if (relativeParent.begin() != relativeParent.end() && std::next(relativeParent.begin()) == relativeParent.end()) {
                    groupName = relativeParent.begin()->string();
                }
            }
        }
        if (!groupName.empty()) {
            this->elementGroups.insert(groupName);
        }

        auto pElement = this->Element_Create(NewElementType, NewElementName, groupName);
        // 判空
        if (!pElement) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，无法创建指定类型的元素实例！ 元素类型：" + NewElementTypeString);
            return false;
        }
        // 统一加载信息
        auto [ok, msg] = pElement->Load(root);
        if (!ok) {
            this->ISys().LogError(std::move(msg));
            return false;
        }
        pElement->Refresh();
        pElement->Dirty = false;// 刚刚进入内存，非脏
        this->ISys().LogSucc(std::move(msg));
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
    auto it = this->elements.find(Name);
    // 判空
    if (it == this->elements.end()) {
        this->ISys().LogError("未找到指定名称的元素：" + Name);
        return false;
    }

    // 检查是否正在预览此元素
    if (this->Preview_CurrentElement && this->Preview_CurrentElement->Name == Name) {
        this->Preview_Disable(); // 禁用预览
    }

    // 检查是否当前正在操作此元素
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current && current->Name == Name) {
        this->CurrentElement = nullptr;
    }

    // 先标记为需要清理
    it->second->NeedBeDelete = true;
    // 通过迭代器删除元素
    this->elements.erase(it);
    // 添加刷新信息
    this->ISys().PublishAsync("CameraSystem/Element/Deleted"_hash);

    this->ISys().LogSucc("成功删除元素：" + Name);
    return true;
}
bool ElementManager::Element_ClearAll() {
    // 禁用预览
    this->Preview_Disable();
    this->Preview_CurrentElement = nullptr;
    // 清空当前操作元素
    this->CurrentElement = nullptr;
    this->CurrentElementGroup.clear();
    this->elementGroups.clear();
    // 检查是否有元素
    if (this->elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过清空操作！");
        return true;
    }
    // 把所有元素标记为需要清理并从Elements中释放
    for (auto& [name, elem] : this->elements) {
        elem->NeedBeDelete = true;
    }
    this->elements.clear();
    // 添加刷新信息
    this->ISys().PublishAsync("CameraSystem/Element/Deleted"_hash);
    this->ISys().LogSucc("成功清空所有元素！");
    return true;
}

//预览相关
void ElementManager::Preview_Enable() {
    if (!this->Preview_CurrentElement) {
        this->ISys().LogError("无法开启预览：未设置预览元素！");
        return;
    }
    this->OnPreview = true;
    this->ISys().PublishAsync("CameraSystem/Preview/Started"_hash);
    this->ISys().LogInfo("已开启预览");
}
void ElementManager::Preview_Disable() {
    this->OnPreview = false;
    this->AL3D->ClearViewOverride();
    this->ISys().PublishAsync("CameraSystem/Preview/Ended"_hash);
    this->ISys().LogInfo("已关闭预览");
}
void ElementManager::Preview_SetElement(const std::string& name) {
    auto it = this->elements.find(name);
    if (it == this->elements.end()) {
        this->ISys().LogError("找不到目标元素   元素名：" + name);
        return;
    }
    this->Preview_CurrentElement = it->second;
    this->ISys().LogInfo("准备预览该元素   元素名：" + name);
}
void ElementManager::Preview_SetPreviewSchema(const float Time) {
    this->Preview_TimeSchema = Time;
    this->ISys().LogInfo("元素预览时间偏移设置为：" + std::to_string(this->Preview_TimeSchema));
    this->Preview_EndTime = this->Preview_CurrentElement->StartTime + this->Preview_CurrentElement->DurationTime;
}
bool ElementManager::Preview_Call(CameraSystemIO* IO) {
    if (!this->OnPreview)return false;
    if (!this->Preview_CurrentElement) {
        this->Preview_Disable();
        return false;
    }
    IO->ElementTime += this->Preview_CurrentElement->GetStartTime() - this->Preview_TimeSchema;
    if (!this->Preview_CurrentElement->CalculateFrame(IO)) {
        this->Preview_Disable();
        return false;
    }
    return true;
}
