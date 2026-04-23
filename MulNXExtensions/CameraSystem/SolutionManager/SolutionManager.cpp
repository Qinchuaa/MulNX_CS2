#include "SolutionManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>

bool SolutionManager::MenuSolution(MulNX::UINode* node) {
    ImGui::Text(std::format("播放状态：{}  活跃解决方案名：{}",
        this->Playing ? "播放中" : "关闭",
        this->Playing_pSolution ? this->Playing_pSolution->GetName() : "无"
    ).c_str());
    ImGui::Separator();
    // 解决方案总设置
    if (ImGui::CollapsingHeader("解决方案设置")) {
        ImGui::Checkbox("解决方案快捷键检测系统", &this->Config.SolutionShortcutEnable);
        ImGui::Checkbox("解决方案插值摄像机绘制", &this->Config.PlayingDraw);
        ImGui::Checkbox("解决方案插值覆盖", &this->Config.PlayingOverride);
    }
    // 创建解决方案
    if (ImGui::CollapsingHeader("解决方案创建")) {
        ImGui::Text("新解决方案名：");
        ImGui::SameLine();
        static std::string CreateSolutionName = "";
        ImGui::InputText("##新解决方案名", &CreateSolutionName);
        ImGui::SameLine();
        // 创建成功则清空输入框
        if (ImGui::Button("创建解决方案")) {
            if (CreateSolutionName.empty()) {
                this->ISys().LogError("请输入解决方案名！");
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Create"_hash);
            rp->str1 = std::move(CreateSolutionName);
            this->ISys().PublishAsync(std::move(msg));
            CreateSolutionName.clear();
        }
    }
    // 展示修改解决方案
    if (ImGui::CollapsingHeader("解决方案列表")) {
        // 输出是否打开了解决方案调试窗口
        ImGui::Text(("解决方案调试窗口状态：" + std::string(this->ShowWindow.load(std::memory_order_acquire) ? "打开" : "关闭")).c_str());
        for (const auto& [name, solution] : this->solutions) {
            this->Solution_ShowInLine(solution.get());
        }
    }

    return true;
}
void SolutionManager::Solution_ShowInLine(Solution* solution) {
    ImGui::Text("|解决方案名称：");
    ImGui::SameLine();
    if (ImGui::Selectable(solution->name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentSolution = solution;
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->name).c_str())) {
        if (ImGui::MenuItem("复制名称")) {
            ImGui::SetClipboardText(solution->name.c_str());
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
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Delete"_hash);
            rp->str1 = solution->name;
            this->ISys().PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text(std::format(" 元素数量：{},   总时长：{}",
        solution->elements.size(), solution->totalDurationTime).c_str());
}
bool SolutionManager::UINodeFunc(MulNX::UINode* node) {
    if (this->needDrawCamera.load(std::memory_order_acquire)) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, "解决方案插值摄像机");
    }

    if (!this->ShowWindow.load(std::memory_order_acquire))return true;
    this->Solution_DebugWindow();
    if (!this->CurrentSolution) {
        this->OpenSolutionKCPackDebugWindow = false;
    }
    if (!this->OpenSolutionKCPackDebugWindow)return true;
    if (this->Buffer_KCPack.DebugWindow(this->OpenSolutionKCPackDebugWindow)) {
        this->OpenSolutionKCPackDebugWindow.store(false, std::memory_order_release);
        if (!this->Buffer_KCPack.Usable) {
            this->ISys().LogError("当前按键绑定不可用，无法使用这个绑键播放解决方案！");
        }
        else {
            this->CurrentSolution->KCPack = this->Buffer_KCPack;//更新绑键
        }
    }
    return true;
}
void SolutionManager::Solution_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow("解决方案调试", this->ShowWindow);
    // 检查当前是否操作解决方案
    if (this->CurrentSolution) {
        ImGui::Text(std::format("当前操作解决方案名称：{}   元素数量：{}   总时长：{}   播放模式：{}",
            this->CurrentSolution->name,
            this->CurrentSolution->elements.size(),
            this->CurrentSolution->totalDurationTime,
            PlaybackModeToString(this->CurrentSolution->playmode)).c_str());

        if (ImGui::Button("切换到激活模式")) {
            this->CurrentSolution->playmode = PlaybackMode::Activation;
        }
        ImGui::SameLine();
        if (ImGui::Button("切换到编排模式")) {
            this->CurrentSolution->playmode = PlaybackMode::Orchestration;
        }

        if (ImGui::Button("使能当前解决方案")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
            rp->str1 = this->CurrentSolution->name;
            this->ISys().PublishAsync(std::move(msg));
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
            auto it = this->EManager->elements.find(NewElementName);
            if (it == this->EManager->elements.end()) {
                this->ISys().LogError("找不到目标元素   元素名：" + NewElementName);
            }
            else {
                if (!this->CurrentSolution->AddElement(it->second, 0)) {
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
        if (0 <= IndexForReset && IndexForReset < this->CurrentSolution->elements.size()) {
            std::shared_ptr<ElementBase> element = this->CurrentSolution->elements.at(IndexForReset).Element;
            if (element) {
                const float& Offset = this->CurrentSolution->elements.at(IndexForReset).Offset;
                ImGui::Text(std::format("元素信息： 编号：{}  元素名称：{}  元素持续时间：{}  元素偏移时间：{}",
                    IndexForReset, element->Name, element->DurationTime, Offset).c_str());
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
            ImGui::Text(std::format("索引无效！请输入 0 到 {} 之间的值", this->CurrentSolution->elements.size() - 1).c_str());
        }
        PreIndex = IndexForReset;
    }
    else {
        ImGui::Text("当前未选择任何解决方案");
    }
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
    }
    case "CameraSystem/Solution/Delete"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Solution_Delete(name)) {
            this->ISys().LogError(std::format("删除解决方案失败：{}", name));
        }
    }
    case "CameraSystem/Solution/Play"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        this->Playing_Solution(name);
    }
    case "CameraSystem/Element/Deleted"_hash: {
        //全部刷新用于清理失效元素
        for (auto& [name, pSolution] : this->solutions) {
            pSolution->Refresh();
        }
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
    //遍历
    for (const auto& [name, pSolution] : this->solutions) {
        //快捷键播放处理
        if (this->pInputSystem->CheckWithPack(pSolution->KCPack)) {
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
    std::unique_ptr<Solution> newSolution = std::make_unique<Solution>(name);
    this->solutions[name] = std::move(newSolution);

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
        auto newSolution = std::make_unique<Solution>(NewSolutionName);
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

    switch (this->Playing_pSolution->playmode) {
    case PlaybackMode::Orchestration:
        this->Playing_pSolution->SetSolutionOffset(this->AL3D->Time()->GetReal());//偏移时间轴播放
        this->ISys().LogInfo(std::format("偏移时间轴播放，偏移时间设置为：{}", this->AL3D->Time()->GetReal()));
        break;
    case PlaybackMode::Activation:
        this->Playing_pSolution->SetSolutionOffset(0);
        break;
    }
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