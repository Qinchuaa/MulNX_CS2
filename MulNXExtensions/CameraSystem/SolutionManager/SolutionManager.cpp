#include "SolutionManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>

bool SolutionManager::MenuSolution(MulNX::UINode* node) {
    static std::string CreateSolutionName = "";

    auto saveSolution = [&](Solution* solution) {
        auto path = this->ISys().PathManager()->PathGetFromKey("Solutions");
        auto [ok, msg] = solution->Save(path);
        if (ok) {
            this->ISys().LogSucc(std::move(msg));
        }
        else {
            this->ISys().LogError(std::move(msg));
        }
        };

    ImGui::Text(I18n(
        "camsys.sol.play_status",
        this->Playing ? I18n("text.opened") : I18n("text.closed"),
        this->Playing_pSolution ? this->Playing_pSolution->GetName() : I18n("text.none")
    ).c_str());
    ImGui::Text("当前编辑: %s", this->CurrentSolution ? this->CurrentSolution->GetName().c_str() : I18n("text.none").c_str());
    ImGui::Separator();
    // 解决方案总设置
    if (ImGui::CollapsingHeader(I18n("camsys.sol.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.sol.shortcut_enable").c_str(), &this->Config.SolutionShortcutEnable);
        ImGui::Checkbox(I18n("camsys.sol.playing_draw").c_str(), &this->Config.PlayingDraw);
        ImGui::Checkbox(I18n("camsys.sol.playing_override").c_str(), &this->Config.PlayingOverride);
    }
    // 创建解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.create").c_str())) {
        ImGui::Text(I18n("camsys.sol.new_name").c_str());
        ImGui::SameLine();
        ImGui::InputText("##新解决方案名", &CreateSolutionName);
        ImGui::SameLine();
        // 创建成功则清空输入框
        if (ImGui::Button(I18n("camsys.sol.create_btn").c_str())) {
            if (CreateSolutionName.empty()) {
                this->ISys().LogError(I18n("result.error_empty_name").c_str());
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Create"_hash);
            rp->str1 = std::move(CreateSolutionName);
            this->ISys().PublishAsync(std::move(msg));
            CreateSolutionName.clear();
        }
    }
    // 展示修改解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.list").c_str())) {
        if (this->CurrentSolution) {
            ImGui::Text("当前选中: %s", this->CurrentSolution->GetName().c_str());
            if (ImGui::Button("打开调试")) {
                this->ShowWindow.store(true, std::memory_order_release);
            }
            ImGui::SameLine();
            if (ImGui::Button(I18n("camsys.sol.enable_current").c_str())) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
                rp->str1 = this->CurrentSolution->name;
                this->ISys().PublishAsync(std::move(msg));
            }
            ImGui::SameLine();
            if (ImGui::Button(I18n("text.save").c_str())) {
                saveSolution(this->CurrentSolution);
            }
            ImGui::SameLine();
            if (ImGui::Button(I18n("text.delete").c_str())) {
                this->PendingDeleteSolutionName = this->CurrentSolution->name;
                ImGui::OpenPopup("删除解决方案确认");
            }
            ImGui::SameLine();
            if (ImGui::Button("保存全部")) {
                this->Solution_SaveAll();
            }
            ImGui::Separator();
        }

        for (const auto& [name, solution] : this->solutions) {
            this->Solution_ShowInLine(solution.get());
        }
    }

    if (ImGui::BeginPopupModal("删除解决方案确认", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("确定删除这个解决方案吗？");
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.55f, 1.0f), "%s", this->PendingDeleteSolutionName.c_str());
        if (ImGui::Button("确认删除")) {
            if (!this->PendingDeleteSolutionName.empty()) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Delete"_hash);
                rp->str1 = this->PendingDeleteSolutionName;
                this->ISys().PublishAsync(std::move(msg));
            }
            this->PendingDeleteSolutionName.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            this->PendingDeleteSolutionName.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return true;
}
void SolutionManager::Solution_ShowInLine(Solution* solution) {
    const bool isCurrent = this->CurrentSolution == solution;
    const bool isPlaying = this->Playing_pSolution == solution && this->Playing;
    std::string label = solution->name;
    if (isCurrent) {
        label = "[编辑] " + label;
    }
    if (isPlaying) {
        label = "[播放] " + label;
    }
    if (solution->dirty) {
        label += " *";
    }

    if (isCurrent || isPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.45f, 0.25f, 0.55f, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.52f, 0.30f, 0.63f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.40f, 0.22f, 0.50f, 0.90f));
    }
    if (ImGui::Selectable(label.c_str(), isCurrent, ImGuiSelectableFlags_AllowDoubleClick)) {
        this->CurrentSolution = solution;
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }
    if (isCurrent || isPlaying) {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->name).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(solution->name.c_str());
        }
        if (ImGui::MenuItem(I18n("text.save").c_str())) {
            auto path = this->ISys().PathManager()->PathGetFromKey("Solutions");
            auto [ok, msg] = solution->Save(path);
            if (ok) {
                this->ISys().LogSucc(std::move(msg));
            }
            else {
                this->ISys().LogError(std::move(msg));
            }
        }
        if (ImGui::MenuItem(I18n("text.print_debug").c_str())) {
            this->ISys().LogLine();
            this->ISys().LogInfo(solution->GetMsg());
            this->ISys().LogLine();
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            this->PendingDeleteSolutionName = solution->name;
            ImGui::OpenPopup("删除解决方案确认");
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text(I18n("camsys.sol.element_count_duration",
        solution->elements.size(),
        solution->totalDurationTime
    ).c_str());
}
bool SolutionManager::UINodeFunc(MulNX::UINode* node) {
    if (this->needDrawCamera.load(std::memory_order_acquire)) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, I18n("camsys.sol.playing_draw_label").c_str());
    }

    if (!this->ShowWindow.load(std::memory_order_acquire))return true;
    this->Solution_DebugWindow();
    return true;
}
void SolutionManager::Solution_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow(I18n("camsys.sol.debug_window").c_str(), this->ShowWindow);
    if (!w)return;
    // 检查当前是否操作解决方案
    if (!this->CurrentSolution) {
        ImGui::Text(I18n("ui.button.no_selected").c_str());
        return;
    }
    
    this->CurrentSolution->playmode = PlaybackMode::Orchestration;
    ImGui::Text(I18n("camsys.sol.current_info",
        this->CurrentSolution->name,
        this->CurrentSolution->elements.size(),
        this->CurrentSolution->totalDurationTime,
        PlaybackModeToString(this->CurrentSolution->playmode)
    ).c_str());
    if (ImGui::Button(I18n("camsys.sol.enable_current").c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = this->CurrentSolution->name;
        this->ISys().PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    ImGui::Separator();

    static std::string NewElementName = "";
    static std::string NewGroupName = "";
    ImGui::InputText(I18n("camsys.sol.new_element_name").c_str(), &NewElementName);
    if (ImGui::Button("添加元素")) {
        auto it = this->EManager->elements.find(NewElementName);
        if (it == this->EManager->elements.end()) {
            this->ISys().LogError("找不到目标元素   元素名：" + NewElementName);
        }
        else {
            const float appendOffset = this->CurrentSolution->GetAppendOffset();
            if (!this->CurrentSolution->AddElement(it->second, appendOffset)) {
                this->ISys().LogError("无法添加元素到解决方案，可能是元素已存在于解决方案中   元素名：" + NewElementName);
            }
            else {
                this->ISys().LogSucc("成功添加元素到解决方案   元素名：" + NewElementName);
                NewElementName.clear();
            }
        }
    }
    ImGui::SameLine();
    ImGui::InputText("元素组", &NewGroupName);
    ImGui::SameLine();
    if (ImGui::Button("添加元素组")) {
        const float appendOffset = this->CurrentSolution->GetAppendOffset();
        if (!this->CurrentSolution->AddElementGroup(NewGroupName, appendOffset)) {
            this->ISys().LogError("无法添加元素组到解决方案   元素组名：" + NewGroupName);
        }
        else {
            this->ISys().LogSucc("成功添加元素组到解决方案   元素组名：" + NewGroupName);
            NewGroupName.clear();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("text.clear").c_str())) {
        this->CurrentSolution->Clear();
        this->ISys().LogSucc("成功清空解决方案所有元素");
    }

    ImGui::Separator();
    for (size_t i = 0; i < this->CurrentSolution->elements.size(); ++i) {
        const auto& item = this->CurrentSolution->elements[i];
        ImGui::Text("%d. %s | 时长 %.2f | 偏移 %.2f",
            static_cast<int>(i),
            this->CurrentSolution->GetEntryLabel(item).c_str(),
            this->CurrentSolution->GetEntryDuration(item),
            item.Offset);
    }

    if (this->CurrentSolution->elements.empty())return;

    ImGui::Separator();

    static int IndexForReset = 0;
    static int PreIndex = -1;
    if (IndexForReset >= static_cast<int>(this->CurrentSolution->elements.size())) {
        IndexForReset = static_cast<int>(this->CurrentSolution->elements.size()) - 1;
    }
    ImGui::SliderInt(I18n("camsys.sol.adjust_element_index").c_str(), &IndexForReset, 0, this->CurrentSolution->elements.size() - 1);
    const auto& item = this->CurrentSolution->elements.at(IndexForReset);
    const float offset = item.Offset;
    ImGui::Text("%d | %s | 时长 %.2f | 偏移 %.2f",
        IndexForReset,
        this->CurrentSolution->GetEntryLabel(item).c_str(),
        this->CurrentSolution->GetEntryDuration(item),
        offset);
    ImGui::Separator();
    static float tempOffset{};
    if (IndexForReset != PreIndex) {
        tempOffset = offset;
    }
    ImGui::SliderFloat(I18n("camsys.sol.offset_time").c_str(), &tempOffset, 0, 100000);
    if (ImGui::Button(I18n("text.confirm_modify").c_str())) {
        this->CurrentSolution->SetElementOffsetAt(IndexForReset, tempOffset);
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("text.remove").c_str())) {
        this->CurrentSolution->RemoveElementAt(IndexForReset);
        if (IndexForReset >= static_cast<int>(this->CurrentSolution->elements.size()) && !this->CurrentSolution->elements.empty()) {
            IndexForReset = static_cast<int>(this->CurrentSolution->elements.size()) - 1;
        }
    }
    PreIndex = IndexForReset;
}

//解决方案管理器

bool SolutionManager::Init() {
    this->CamDrawer = &this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendUINode("MenuSolution", [this](MulNX::UINode* node) {return this->MenuSolution(node);});

    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("Solutions", "Solutions",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Solutions");
            this->ISys().LogSucc("成功设置解决方案路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Solutions", "CurrentProject");
    }
    this->ISys()
        .SubscribeAsync("CameraSystem/Element/Deleted")
        .SubscribeAsync("CameraSystem/Solution/Create")
        .SubscribeAsync("CameraSystem/Solution/Delete")
        .SubscribeAsync("CameraSystem/Solution/Play");

    return true;
}

void SolutionManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CameraSystem/Solution/Create"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Solution_Create(name)) {
            this->ISys().LogError(std::format("创建解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Delete"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Solution_Delete(name)) {
            this->ISys().LogError(std::format("删除解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Play"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        this->Playing_Solution(name);
        break;
    }
    case "CameraSystem/Element/Deleted"_hash: {
        //全部刷新用于清理失效元素
        for (auto& [name, pSolution] : this->solutions) {
            pSolution->Refresh();
        }
        break;
    }
    }
}

void SolutionManager::HandleUpdate() {
    this->EntryProcessMsg();
    
    if (this->CurrentSolution) {
        this->CurrentSolution->Refresh();//刷新当前调试的解决方案确保操作反馈及时（当前播放的解决方案由Playing_Call负责更新）
    }
    this->Playing_Call();
    if (!this->Config.SolutionShortcutEnable)return;
    if (!this->PManager || !this->PManager->ActiveProject) {
        return;
    }
    //遍历
    for (const auto& [name, pSolution] : this->solutions) {
        auto bindingIt = this->PManager->ActiveProject->SolutionKeybinds.find(name);
        if (bindingIt == this->PManager->ActiveProject->SolutionKeybinds.end() || !bindingIt->second.Usable) {
            continue;
        }
        if (this->pInputSystem->CheckWithPack(bindingIt->second)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
            rp->str1 = pSolution.get()->name;
            this->ISys().PublishAsync(std::move(msg));
        }
        //后续其它任务待补充
    }
    return;
}
//创建，得到，删除

bool SolutionManager::Solution_Create(const std::string& name) {
    // 检查是否已存在同名解决方案
    if (this->solutions.find(name) != this->solutions.end()) {
        this->ISys().LogError("解决方案名已占用！ 解决方案名：" + name);
        return false;
    }
    //输出成功信息
    this->ISys().LogSucc("成功创建解决方案！  解决方案名：" + name);
    //创建新解决方案
    std::unique_ptr<Solution> newSolution = std::make_unique<Solution>(name, this->EManager);
    Solution* created = newSolution.get();
    this->solutions[name] = std::move(newSolution);
    this->CurrentSolution = created;
    this->ShowWindow.store(true, std::memory_order_release);

    return true;
}
bool SolutionManager::Solution_SaveAll() {
    if (this->solutions.empty()) {
        this->ISys().LogWarning("尝试在没有任何解决方案的情况下保存");
        return true;
    }
    std::filesystem::path SolutionFolderPath = this->ISys().PathManager()->PathGetFromKey("Solutions");
    //遍历所有解决方案保存
    for (const auto& [name, solution] : this->solutions) {
        if (!solution->dirty) {
            continue;//不脏不需保存
        }
        auto [ok, msg] = solution->Save(SolutionFolderPath);
        if (!ok) {
            this->ISys().LogError(msg);
            return false;
        }
        this->ISys().LogSucc(msg);
    }
    this->ISys().LogSucc("成功保存所有解决方案到文件！");
    return true;
}
bool SolutionManager::Solution_Load(const std::filesystem::path& FullPath) {
    // 输出调试信息
    this->ISys().LogInfo("尝试从yaml文件加载解决方案，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("yaml文件不存在！文件路径：" + FullPath.string());
        return false;
    }
    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        // 获取解决方案名称并检查是否为空
        std::string NewSolutionName = root["name"].as<std::string>();

        if (NewSolutionName.empty()) {
            this->ISys().LogError("尝试从yaml文件加载解决方案失败，解决方案名称为空！");
            return false;
        }

        // 检查是否存在同名解决方案
        if (this->solutions.find(NewSolutionName) != this->solutions.end()) {
            this->ISys().LogError("解决方案名已占用，无法从yaml文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName));
            return false;
        }
        // 获取持续时长信息
        float TargetDurationTime = root["duration"].as<float>();
        // 制作解决方案
        auto newSolution = std::make_unique<Solution>(NewSolutionName, this->EManager);
        auto [ok, msg] = newSolution->Load(root, this->EManager);

        if (!ok) {
            this->ISys().LogError(std::move(msg));
            return false;
        }

        // 检验时间关系
        if (newSolution->totalDurationTime != TargetDurationTime) {
            this->ISys().LogWarning("该解决方案实际持续时长与预估持续时长不同，可能出现问题");
        }

        // 添加进解决方案组
        this->solutions[NewSolutionName] = std::move(newSolution);
        this->ISys().LogSucc(std::move(msg));
        this->ISys().LogLine();
        return true;
    }
    catch (const YAML::Exception& e) {
        this->ISys().LogError("在加载yaml时遇到异常：" + std::string(e.what()));
        return false;
    }
}

bool SolutionManager::Solution_Delete(const std::string& name) {
    //安全检查
    if (name.empty()) {
        this->ISys().LogError("尝试删除空名称的解决方案！");
        return false;
    }

    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->ISys().LogError("未找到指定名称的解决方案：" + name);
        return false;
    }

    //检查是否正在播放此解决方案
    if (this->Playing) {
        if (this->Playing_pSolution == it->second.get()) {
            this->Playing_Disable(); //禁用播放
            this->Playing_pSolution = nullptr;
        }
    }

    //检查是否当前正在操作此解决方案
    if (this->CurrentSolution) {
        if (this->CurrentSolution == it->second.get())
            this->CurrentSolution = nullptr;
    }

    this->solutions.erase(it);

    this->ISys().LogSucc("成功删除解决方案：" + name);
    return true;
}
bool SolutionManager::Solution_ClearAll() {
    //禁用播放
    this->Playing_Disable();
    this->Playing_pSolution = nullptr;
    //清空当前操作解决方案
    this->CurrentSolution = nullptr;

    if (this->solutions.empty()) {
        this->ISys().LogWarning("当前没有任何解决方案，跳过清空操作！");
        return true;
    }
    //清空所有解决方案
    this->solutions.clear();
    this->ISys().LogSucc("成功删除所有解决方案！");
    return true;
}

void SolutionManager::Playing_Solution(const std::string& name) {
    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->ISys().LogError(std::format("目标解决方案不存在：{}", name));
        return;
    }
    this->Playing_pSolution = it->second.get();
    this->Playing_pSolution->playmode = PlaybackMode::Orchestration;
    this->Playing_pSolution->SetSolutionOffset(this->AL3D->Time()->GetReal());//偏移时间轴播放
    this->ISys().LogInfo(std::format("偏移时间轴播放，偏移时间设置为：{}", this->AL3D->Time()->GetReal()));
    this->Playing = true;
    this->ISys().PublishAsync("CameraSystem/Play/Started"_hash);
    this->ISys().LogInfo(std::format("播放解决方案：{}", name));
    return;
}
void SolutionManager::Playing_Disable() {
    this->Playing = false;
    this->AL3D->ClearViewOverride();
    this->ISys().PublishAsync("CameraSystem/Play/Ended"_hash);
    this->ISys().LogInfo("已关闭播放");
}
void SolutionManager::Playing_Call() {
    if (!this->Playing) {
        return;
    }
    //调用插值
    //理论上如果超出解决方案工作时间区，就不再调用
    if (!this->Playing_pSolution) {
        this->Playing_Disable();
        return;
    }
    CameraSystemIO IO;

    IO.SolutionTime = this->AL3D->Time()->Get();
    IO.FrameGameTime = this->AL3D->Time()->GetReal();
    IO.isPlaying = this->Playing;

    if (!this->Playing_pSolution->Call(&IO)) {
        this->AL3D->ClearViewOverride();
        // 这里不关闭播放，因为解决方案可能还有内容
        // 不应该由管理器因为仅仅没有结果就关闭
        if (IO.isPlaying == false) {
            // 如果解决方案自己关闭了播放
            this->Playing_Disable();
            return;
        }
        return;
    }
    if (this->Config.PlayingOverride) {
        this->AL3D->CameraSystemIOOverride(&IO);
    }
    if (this->Config.PlayingDraw) {
        auto frame = this->drawCamera.Write();
        *frame = IO.Frame;
        this->needDrawCamera.store(true, std::memory_order_release);
    }
    else {
        this->needDrawCamera.store(false, std::memory_order_release);
    }
}
