#include "SolutionManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>

//解决方案管理器

bool SolutionManager::Init() {
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->UINodeFunc(node);});
    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("Solutions", "Solutions",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Solutions");
            this->ISys().LogSucc("成功设置解决方案路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Solutions", "CurrentProject");
    }
    return true;
}
void SolutionManager::InjectDependence(CameraDrawer* CamDrawer, ElementManager* EManager, ProjectManager* PManager) {
    //系统服务
    this->CamDrawer = CamDrawer;
    this->EManager = EManager;
    this->PManager = PManager;
}
void SolutionManager::VirtualMain() {
    //判断需不需要刷新所有
    if (this->NeedRefresh) {
        this->Refresh();//全部刷新用于清理失效元素
        this->NeedRefresh = false;
    }
    else if (this->CurrentSolution) {
        this->CurrentSolution->Refresh();//刷新当前调试的解决方案确保操作反馈及时（当前播放的解决方案由Playing_Call负责更新）
    }
    this->Traversal();

    this->Playing_Call();

    return;
}
void SolutionManager::Traversal() {
    if (!this->Config.SolutionShortcutEnable)return;
    //遍历
    for (const auto& pSolution : this->Solutions) {
        //快捷键播放处理
        if (this->pInputSystem->CheckWithPack(pSolution->KCPack)) {
            this->Playing_SetSolution(pSolution.get());//设置播放，偏移时间轴播放
            this->Playing_Enable();//启动播放
        }
        //后续其它任务待补充

    }
}
void SolutionManager::Refresh() {
    //遍历
    for (auto& pSolution : this->Solutions) {
        pSolution->Refresh();
    }
}

//创建，得到，删除

bool SolutionManager::Solution_Create(const std::string& Name) {
    // 检查是否已存在同名解决方案
    if (this->Solution_Get(Name)) {
        this->ISys().LogError("解决方案名已占用！ 解决方案名：" + Name);
        return false;
    }
    //输出成功信息
    this->ISys().LogSucc("成功创建解决方案！  解决方案名：" + Name);
    //创建新解决方案
    std::unique_ptr<Solution> newSolution = std::make_unique<Solution>(Name);
    Solutions.push_back(std::move(newSolution));

    return true;
}
bool SolutionManager::Solution_SaveAll() {
    if (this->Solutions.empty()) {
        this->ISys().LogWarning("尝试在没有任何解决方案的情况下保存");
        return true;
    }
    std::filesystem::path SolutionFolderPath = this->ISys().PathManager()->PathGetFromKey("Solutions");
    //遍历所有解决方案保存
    for (const auto& solution : this->Solutions) {
        if (!solution->Dirty) {
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
        if (this->Solution_Get(NewSolutionName)) {
            this->ISys().LogError("解决方案名已占用，无法从yaml文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName));
            return false;
        }
        // 获取持续时长信息
        float TargetDurationTime = root["duration"].as<float>();
        // 制作解决方案
        auto newSolution = std::make_unique<Solution>(NewSolutionName);
        auto [ok, msg] = newSolution->Load(root, this->EManager);
        
        if (!ok) {
            this->ISys().LogError(std::move(msg));
            return false;
        }

        // 检验时间关系
        if (newSolution->TotalDurationTime != TargetDurationTime) {
            this->ISys().LogWarning("该解决方案实际持续时长与预估持续时长不同，可能出现问题");
        }

        // 添加进解决方案组
        this->Solutions.push_back(std::move(newSolution));
        this->ISys().LogSucc(std::move(msg));
        this->ISys().LogLine();
        return true;
    }
    catch (const YAML::Exception& e) {
        this->ISys().LogError("在加载yaml时遇到异常：" + std::string(e.what()));
        return false;
    }
}
std::vector<std::unique_ptr<Solution>>::iterator SolutionManager::Solution_GetIterator(const std::string& Name) {
    return std::find_if(this->Solutions.begin(), this->Solutions.end(),
        [&Name](const std::unique_ptr<Solution>& solution) {
            return solution->Name == Name;
        });
}
Solution* SolutionManager::Solution_Get(const std::string& Name) {
    auto it = this->Solution_GetIterator(Name);
    if (it == this->Solutions.end()) {
        return nullptr;
    }
    return it->get();
}
bool SolutionManager::Solution_Delete(Solution* Solution) {
    //检查是否正在播放此解决方案
    if (this->Playing) {
        if (this->Playing_pSolution == Solution) {
            this->Playing_Disable(); //禁用播放
            this->Playing_pSolution = nullptr;
        }
    }

    //检查是否当前正在操作此解决方案
    if (this->CurrentSolution) {
        if (this->CurrentSolution == Solution)
            this->CurrentSolution = nullptr;
    }

    //通过迭代器删除元素
    std::string Name = Solution->Name;
    auto it = this->Solution_GetIterator(Name);
    this->Solutions.erase(it);

    this->ISys().LogSucc("成功删除解决方案：" + Name);
    return true;
}
bool SolutionManager::Solution_Delete(const std::string& Name) {
    //安全检查
    if (Name.empty()) {
        this->ISys().LogError("尝试删除空名称的解决方案！");
        return false;
    }

    //获取迭代器
    auto it = this->Solution_GetIterator(Name);
    //判空
    if (it == this->Solutions.end()) {
        this->ISys().LogError("未找到指定名称的解决方案：" + Name);
        return false;
    }

    return this->Solution_Delete(it->get());
}
bool SolutionManager::Solution_ClearAll() {
    //禁用播放
    this->Playing_Disable();
    this->Playing_pSolution = nullptr;
    //清空当前操作解决方案
    this->CurrentSolution = nullptr;

    if (this->Solutions.empty()) {
        this->ISys().LogWarning("当前没有任何解决方案，跳过清空操作！");
        return true;
    }
    //清空所有解决方案
    this->Solutions.clear();
    this->ISys().LogSucc("成功删除所有解决方案！");
    return true;
}

//功能
const std::vector<std::string> SolutionManager::Solution_GetNames()const {
    std::vector<std::string> SolutionsNames;
    if (this->Solutions.empty())return SolutionsNames;
    SolutionsNames.reserve(this->Solutions.size());
    for (size_t i = 0; i < this->Solutions.size(); ++i) {
        SolutionsNames.push_back(this->Solutions[i]->Name);
    }
    return SolutionsNames;
}
void SolutionManager::Solution_ShowInLine(Solution* solution) {
    if (!solution) {
        this->ISys().LogError("解决方案指针为空，无法展示信息！");
        return;
    }
    ImGui::Text("|解决方案名称：");
    ImGui::SameLine();
    if (ImGui::Selectable(solution->Name.data(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentSolution = solution;
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->Name).c_str())) {
        if (ImGui::MenuItem("复制名称")) {
            ImGui::SetClipboardText(solution->Name.c_str());
        }
        if (ImGui::MenuItem("保存到磁盘")) {
            auto path = this->ISys().PathManager()->PathGetFromKey("Solutions");
            auto [ok, msg] = solution->Save(path);
            if (ok) {
                this->ISys().LogSucc(std::move(msg));
            }
            else {
                this->ISys().LogError(std::move(msg));
            }
        }
        if (ImGui::MenuItem("打印信息调试窗口")) {
            this->ISys().LogLine();
            this->ISys().LogInfo(solution->GetMsg());
            this->ISys().LogLine();
        }
        if (ImGui::MenuItem("删除")) {
            this->Solution_Delete(solution->Name);
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text(("   元素数量：" + std::to_string(solution->Elements.size()) + "   总时长：" + std::to_string(solution->TotalDurationTime)).data());

}
void SolutionManager::Solution_ShowAllInLines() {
    //使用迭代器遍历所有项目
    for (const auto& Solution : this->Solutions) {
        this->Solution_ShowInLine(Solution.get());
    }
    return;
}

//调试窗口及菜单

bool SolutionManager::UINodeFunc(MulNXUINode* node) {
    if (this->ShowWindow.load(std::memory_order_acquire)) {
        this->Solution_DebugWindow();
        if (this->OpenSolutionKCPackDebugWindow) {
            this->Solution_KCPack_DebugWindow();
        }
        if (this->OpenSolutionNameDebugWindow) {
            this->Solution_Name_DebugWindow();
        }
    }
    return true;
}
void SolutionManager::Solution_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow("解决方案调试", this->ShowWindow);
    // 检查当前是否操作解决方案
    if (this->CurrentSolution) {
        ImGui::Text(std::format("当前操作解决方案名称：{}   元素数量：{}   总时长：{}   播放模式：{}",
            this->CurrentSolution->Name,
            this->CurrentSolution->Elements.size(),
            this->CurrentSolution->TotalDurationTime,
            PlaybackModeToString(this->CurrentSolution->Playmode)).c_str());

        if (ImGui::Button("切换到激活模式")) {
            this->CurrentSolution->Playmode = PlaybackMode::Activation;
        }
        ImGui::SameLine();
        if (ImGui::Button("切换到编排模式")) {
            this->CurrentSolution->Playmode = PlaybackMode::Orchestration;
        }

        if (ImGui::Button("使能当前解决方案")) {
            this->Playing_SetSolution(this->CurrentSolution);
            this->Playing_Enable();
        }
        ImGui::SameLine();
        if (ImGui::Button("按激活模式生成编排模式偏移")) {
            this->CurrentSolution->TimeLineGenerate();
        }

        if (ImGui::Button("修改按键绑定")) {
            this->Buffer_KCPack = this->CurrentSolution->KCPack;//缓存
            this->OpenSolutionKCPackDebugWindow = true;//打开窗口
        }

        ImGui::Separator();

        static std::string NewElementName = "";
        ImGui::InputText("新元素名称", &NewElementName);
        if (ImGui::Button("添加元素")) {
            std::shared_ptr<ElementBase> element = this->EManager->Element_Get<ElementBase>(NewElementName);
            if (!element) {
                this->ISys().LogError("找不到目标元素   元素名：" + NewElementName);
            }
            else {
                if (!this->CurrentSolution->AddElement(element, 0)) {
                    this->ISys().LogError("无法添加元素到解决方案，可能是元素已存在于解决方案中   元素名：" + NewElementName);
                }
                else {
                    this->ISys().LogSucc("成功添加元素到解决方案   元素名：" + NewElementName);
                    NewElementName.clear();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("清空所有元素")) {
            this->CurrentSolution->Clear();
            this->ISys().LogSucc("成功清空解决方案所有元素");
        }

        ImGui::Separator();

        static int IndexForReset = 0;
        static int PreIndex = -1;
        ImGui::InputInt("要调整的本解决方案的元素", &IndexForReset);
        if (0 <= IndexForReset && IndexForReset < this->CurrentSolution->Elements.size()) {
            std::shared_ptr<ElementBase> element = this->CurrentSolution->Elements.at(IndexForReset).Element;
            if (element) {
                const float& Offset = this->CurrentSolution->Elements.at(IndexForReset).Offset;
                ImGui::Text(("元素信息： 编号： " + std::to_string(IndexForReset) + "  元素名称：" + element->Name + "  元素持续时间：" + std::to_string(element->DurationTime) + "  元素偏移时间：" + std::to_string(Offset)).data());
                ImGui::Separator();
                static float tempOffset{};
                if (IndexForReset != PreIndex) {
                    tempOffset = Offset;
                }
                ImGui::SliderFloat("偏移时间", &tempOffset, 0, 100000);
                if (ImGui::Button("确认修改")) {
                    this->CurrentSolution->RemoveElementAt(IndexForReset);
                    this->CurrentSolution->AddElement(element, tempOffset);
                }
                if (ImGui::Button("从解决方案移除该元素")) {
                    this->CurrentSolution->RemoveElementAt(IndexForReset);
                }
            }

        }
        else {
            ImGui::Text("索引无效！请输入 0 到 %d 之间的值", this->CurrentSolution->Elements.size() - 1);
        }
        PreIndex = IndexForReset;
    }
    else {
        ImGui::Text("当前未选择任何解决方案");
    }
}
void SolutionManager::Solution_KCPack_DebugWindow() {
    ImGui::Begin("按键绑定", &this->OpenSolutionKCPackDebugWindow);

    if (this->CurrentSolution) {
        ImGui::Text(("当前绑键：" + this->CurrentSolution->KCPack.GetMsg()).c_str());
        ImGui::Separator();

        // 修饰键复选框
        ImGui::Checkbox("Ctrl", &this->Buffer_KCPack.Ctrl);
        ImGui::SameLine();
        ImGui::Checkbox("Shift", &this->Buffer_KCPack.Shift);
        ImGui::SameLine();
        ImGui::Checkbox("Alt", &this->Buffer_KCPack.Alt);

        // 按键选择下拉菜单
        static constexpr const char* keyItems[] = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"
        };

        // 查找当前键码对应的索引
        int currentKeyIndex = 0;
        for (int i = 0; i < IM_ARRAYSIZE(keyItems); i++) {
            if (i < 26) { // A-Z
                if (this->Buffer_KCPack.vkCode == 0x41 + i) {
                    currentKeyIndex = i;
                    break;
                }
            }
            else { // F1-F12
                if (this->Buffer_KCPack.vkCode == 0x70 + (i - 26)) {
                    currentKeyIndex = i;
                    break;
                }
            }
        }

        ImGui::Text("按键:");
        ImGui::SameLine();
        if (ImGui::Combo("##KeyCombo", &currentKeyIndex, keyItems, IM_ARRAYSIZE(keyItems))) {
            // 更新键码
            if (currentKeyIndex < 26) {
                this->Buffer_KCPack.vkCode = 0x41 + currentKeyIndex; // A-Z
            }
            else {
                this->Buffer_KCPack.vkCode = 0x70 + (currentKeyIndex - 26); // F1-F12
            }
        }

        // 连击数输入
        int comboClick = static_cast<int>(this->Buffer_KCPack.ComboClick);
        ImGui::Text("连击数:");
        ImGui::SameLine();
        if (ImGui::InputInt("##ComboClick", &comboClick, 1, 5)) {
            // 限制在 1-255 范围内
            comboClick = std::clamp(comboClick, 1, 255);
            this->Buffer_KCPack.ComboClick = static_cast<unsigned char>(comboClick);
        }

        if (ImGui::Button("修改绑键")) {
            this->CurrentSolution->SetKeyCheckPack(this->Buffer_KCPack);
            this->OpenSolutionKCPackDebugWindow = false;
            ImGui::End();
            return;
        }
    }
    else {
        ImGui::Text("当前没有要调试的解决方案，无法修改按键绑定");
    }

    ImGui::Separator();

    if (ImGui::Button("关闭按键绑定调试页面")) {
        this->OpenSolutionKCPackDebugWindow = false;
        ImGui::End();
        return;
    }

    ImGui::End();
}
void SolutionManager::Solution_Name_DebugWindow() {
    ImGui::Begin("解决方案重命名", &this->OpenSolutionNameDebugWindow);

    if (this->CurrentSolution) {
        ImGui::Text(("当前名称：" + this->CurrentSolution->Name).c_str());
        ImGui::Separator();

        ImGui::Text("新解决方案名:");
        ImGui::SameLine();
        ImGui::InputText("##NewName", &this->Buffer_Name);

        static bool IfNewNameBeUsing = false;
        //检查是否已经存在该名称
        if (this->Solution_Get(this->Buffer_Name)) {
            IfNewNameBeUsing = true;
        }
        if (IfNewNameBeUsing) {
            ImGui::Text("该名称已经被占用");
        }
        else {//没有被使用才允许修改
            if (ImGui::Button("修改名称")) {
                this->CurrentSolution->ResetName(this->Buffer_Name);
                this->OpenSolutionNameDebugWindow = false;
                ImGui::End();
                return;
            }
        }
    }
    else {
        ImGui::Text("当前没有要调试的解决方案，无法修改解决方案名");
    }

    ImGui::Separator();

    if (ImGui::Button("关闭解决方案名调试页面")) {
        this->OpenSolutionNameDebugWindow = false;
        ImGui::End();
        return;
    }

    ImGui::End();
}


//播放相关

//切换相关

bool SolutionManager::Playing_SetSolution(Solution* const solution) {
    //这里只需要设置，播放结束解决方案本身自动归位
    if (!solution) {
        this->ISys().LogError("找不到目标解决方案，可能是空指针");
        return false;
    }
    this->Playing_pSolution = solution;
    switch (solution->Playmode) {
    case PlaybackMode::Orchestration:
        this->Playing_SetTimeSchema(this->AL3D->GetTime());//偏移时间轴播放
        this->ISys().LogInfo("偏移时间轴播放，偏移时间设置为：" + std::to_string(this->AL3D->GetTime()));
        break;
    case PlaybackMode::Activation:
        this->Playing_SetTimeSchema(0);
        break;
    }
    this->ISys().LogInfo("已经切换至解决方案：" + solution->Name);
    return true;
}
bool SolutionManager::Playing_SetSolution(const std::string& SolutionName) {
    Solution* pSolution = this->Solution_Get(SolutionName);
    if (!pSolution) {
        return false;
    }
    return this->Playing_SetSolution(pSolution);
}


//启动相关

void SolutionManager::Playing_Enable() {
    if (!this->Playing_pSolution) {
        this->ISys().LogError("无法开启播放：未设置要播放的解决方案！");
        return;
    }
    this->Playing = true;
    this->ISys().LogInfo("已开启播放");
}
void SolutionManager::Playing_Disable() {
    this->Playing = false;
    this->AL3D->ClearViewOverride();
    this->ISys().LogInfo("已关闭播放");
}
void SolutionManager::Playing_SetTimeSchema(const float Time) {
    this->Playing_pSolution->SetSolutionOffset(Time);
    return;
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

    IO.SolutionTime = this->AL3D->GetTime();
    IO.FrameGameTime = this->AL3D->GetTime();
    IO.PlayBackRate = this->PlaybackRate;
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
    else if (this->Config.PlayingDraw) {
        this->CamDrawer->DrawFrameCamera(IO.Frame, "解决方案插值摄像机");
    }
}