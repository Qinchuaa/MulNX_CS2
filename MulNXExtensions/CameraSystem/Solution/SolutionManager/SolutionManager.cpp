#include "SolutionManager.hpp"

#include "../../CameraDrawer/CameraDrawer.hpp"
#include "../../Element/ElementManager/ElementManager.hpp"
#include "../../Project/ProjectManager/ProjectManager.hpp"

#include <MulNX/MulNX.hpp>

//解决方案管理器

bool SolutionManager::Init() {
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
        if (this->KT->CheckWithPack(pSolution->KCPack)) {
            this->Playing_SetSolution(pSolution.get(), PlaybackMode::Serial);//设置播放，偏移时间轴播放
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
        auto [ok, msg] = solution->SaveToXML(SolutionFolderPath);
        if (!ok) {
            this->ISys().LogError(msg);
            return false;
        }
        this->ISys().LogSucc(msg);
    }
    this->ISys().LogSucc("成功保存所有解决方案到XML文件！");
    return true;
}
bool SolutionManager::Solution_LoadFromXML(const std::string& XMLName, const std::filesystem::path& FolderPath) {
    //检查文件路径和名称存在性
    if (FolderPath.empty() || XMLName.empty()) {
        this->ISys().LogError("文件夹路径或XML文件名为空，无法从XML文件加载解决方案！");
        return false;
    }

    //拼接完整路径
    std::filesystem::path FullPath = FolderPath / (XMLName + ".xml");

    return this->Solution_LoadFromXML(FullPath);
}
bool SolutionManager::Solution_LoadFromXML(const std::filesystem::path& FullPath) {
    //输出调试信息
    this->ISys().LogInfo("尝试从XML文件加载解决方案，文件路径：" + FullPath.string());

    //检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("XML文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    //创建临时XML文件
    pugi::xml_document NewXML;

    //打开XML文件并检验结果
    pugi::xml_parse_result result = NewXML.load_file(FullPath.c_str());
    if (!result) {
        this->ISys().LogError(
            "尝试从XML文件加载解决方案失败，无法加载XML文件！ 文件路径：" + FullPath.string() +
            "\n     错误描述：" + result.description());
        return false;
    }

    //开始获取信息

    //获取Solution节点
    pugi::xml_node node_Solution = NewXML.child("Solution");
    if (!node_Solution) {
        this->ISys().LogError("尝试从XML文件加载解决方案失败，XML文件格式错误，找不到根节点！ 文件路径：" + FullPath.string());
        return false;
    }

    //获取解决方案名称并检查是否为空
    std::string NewSolutionName = node_Solution.attribute("Name").as_string();
    if (NewSolutionName.empty()) {
        this->ISys().LogError("尝试从XML文件加载解决方案失败，解决方案名称为空！");
        return false;
    }

    //检查是否存在同名解决方案
    if (this->Solution_Get(NewSolutionName)) {
        this->ISys().LogError("解决方案名已占用，无法从XML文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName));
        return false;
    }

    //获取SolutionMain节点
    pugi::xml_node node_SolutionMain = node_Solution.child("SolutionMain");
    if (!node_SolutionMain) {
        this->ISys().LogError("尝试从XML文件加载解决方案失败，找不到SolutionMain节点！ 文件路径：" + FullPath.string());
        return false;
    }
    //获取KeyCheckPack节点
    pugi::xml_node node_KeyCheckPack = node_SolutionMain.child("KeyCheckPack");
    if (!node_KeyCheckPack) {
        this->ISys().LogError("尝试从XML文件加载解决方案失败，找不到KeyCheckPack节点！ 文件路径：" + FullPath.string());
        return false;
    }
    //制作解决方案
    std::unique_ptr<Solution>NewSolution = std::make_unique<Solution>(NewSolutionName);
    if (!NewSolution->KCPack.ReadXMLNode(node_KeyCheckPack)) {
        this->ISys().LogError("尝试从XML文件加载解决方案失败，在解析KeyCheckPack节点时发生了错误！ 文件路径：" + FullPath.string());
        return false;
    }

    //获取持续时长信息
    float TargetDurationTime = node_SolutionMain.attribute("DurationTime").as_float();

    //获取Elements节点
    pugi::xml_node node_Elements = node_SolutionMain.child("Elements");
    if (!node_Elements) {
        this->ISys().LogError("找不到Elements节点！ 解决方案名：" + std::move(NewSolutionName));
        return false;
    }

    //获取Elements信息

    //元素总量
    size_t AllCount = node_Elements.attribute("Size").as_ullong();
    //检验完整性
    if (node_Elements.select_nodes("Element").size() != AllCount) {
        this->ISys().LogError("不安全的解决方案！实际元素数量与XML文件描述不符！ 解决方案名：" + std::move(NewSolutionName));
        return false;
    }

    //成功元素个数
    size_t SuccessCount = 0;
    //加载失败元素索引
    std::vector<size_t>ErrorElementsIndexs{};
    //加载失败元素名称
    std::vector<std::string>ErrorElementsNames{};

    //获取第一个Element节点并检验
    pugi::xml_node node_Element = node_Elements.child("Element");
    if (!node_Element) {
        this->ISys().LogError("找不到Element节点！ 解决方案名：" + std::move(NewSolutionName));
        return false;
    }

    //读取流程，已经在前面检验过不会越界
    for (size_t Index = 0; Index < AllCount; node_Element = node_Element.next_sibling("Element"), ++Index) {
        //获取元素名称
        std::string NewElementName = node_Element.attribute("Name").as_string();
        //得到元素指针
        std::shared_ptr<ElementBase> element = this->EManager->Element_Get<ElementBase>(NewElementName);
        //检验是否找到元素
        if (!element) {//未找到元素
            //输出错误信息
            this->ISys().LogError("找不到目标元素   元素名：" + NewElementName);
            //记录加载失败的元素的索引
            ErrorElementsIndexs.push_back(Index);
            //记录加载失败的元素的名称
            ErrorElementsNames.push_back(std::move(NewElementName));
            continue;
        }
        else {
            //获取元素偏移
            float ElementOffset = node_Element.attribute("Offset").as_float();
            //尝试创建带有时间偏移的弱引用指针并添加进新解决方案并判断是否成功
            if (!NewSolution->AddElement(element, ElementOffset)) {
                //如果失败
                //输出错误信息
                this->ISys().LogError("无法添加元素到解决方案，可能是元素已存在于解决方案中   元素名：" + NewElementName);
                //记录加载失败的元素的索引
                ErrorElementsIndexs.push_back(Index);
                //记录加载失败的元素的名称
                ErrorElementsNames.push_back(std::move(NewElementName));
                continue;
            }
            else {
                //成功则输出成功信息
                this->ISys().LogSucc("成功添加元素到解决方案   元素名：" + std::move(NewElementName));
            }
        }
        //增加计数
        ++SuccessCount;
    }

    //刷新
    NewSolution->Refresh();
    //去除脏标记
    NewSolution->Dirty = false;
    //检验时间关系
    if (NewSolution->TotalDurationTime != TargetDurationTime) {
        this->ISys().LogWarning("该解决方案实际持续时长与预估持续时长不同，可能出现问题");
    }

    //复查加载个数
    if (AllCount == SuccessCount) {
        this->ISys().LogSucc("成功从XML文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName) + "  共包含元素个数：" + std::to_string(AllCount));
    }
    else {
        this->ISys().LogWarning("从XML文件加载了解决方案：" + std::move(NewSolutionName) + "  理论包含元素个数：" + std::to_string(AllCount));
        size_t ErrorCount = AllCount - SuccessCount;
        this->ISys().LogWarning("但实际上加载成功元素个数：" + std::to_string(SuccessCount) + " 以下是加载失败的" + std::to_string(ErrorCount) + "个元素");
        for (size_t i = 0; i < ErrorCount; ++i) {
            this->ISys().LogWarning("编号：" + std::to_string(ErrorElementsIndexs[i]) + "  名称：" + ErrorElementsNames[i]);
        }
    }


    //添加进解决方案组
    this->Solutions.push_back(std::move(NewSolution));
    this->ISys().LogSucc("---------------------");
    return true;
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

void SolutionManager::Solution_ShowMsg(const std::string& Name) {
    Solution* solution = this->Solution_Get(Name);
    if (!solution) {
        this->ISys().LogError("未找到解决方案：" + Name);
        return;
    }
    if (!solution->SafeUse) {
        this->ISys().LogError("不安全的解决方案：" + Name);
        return;
    }
    this->ISys().LogLine();
    this->ISys().LogInfo(solution->GetMsg());
    this->ISys().LogLine();
}
void SolutionManager::Solution_ShowAll() {
    size_t Size = this->Solutions.size();
    if (Size) {
        this->ISys().LogLine();
        for (size_t i = 0; i < Size; ++i) {
            this->ISys().LogInfo(" |解决方案编号：" + std::to_string(i) + "   解决方案名称：" + this->Solutions[i]->Name);
        }
        this->ISys().LogLine();
    }
    else {
        this->ISys().LogError("没有找到任何解决方案正存储在内存中！");
        return;
    }
}
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
    if (ImGui::Button(solution->Name.c_str())) {
        this->CurrentSolution = solution;
        this->OpenSolutionDebugWindow = true;
    }
    ImGui::SameLine();
    ImGui::Text(("   元素数量：" + std::to_string(solution->Size_Elements) + "   总时长：" + std::to_string(solution->TotalDurationTime)).data());

}
void SolutionManager::Solution_ShowAllInLines() {
    //使用迭代器遍历所有项目
    for (const auto& Solution : this->Solutions) {
        this->Solution_ShowInLine(Solution.get());
    }
    return;
}

//调试窗口及菜单

void SolutionManager::Windows() {
    if (this->OpenSolutionDebugWindow) {
        this->Solution_DebugWindow();
        if (this->OpenSolutionKCPackDebugWindow) {
            this->Solution_KCPack_DebugWindow();
        }
        if (this->OpenSolutionNameDebugWindow) {
            this->Solution_Name_DebugWindow();
        }
    }

}
void SolutionManager::Solution_DebugWindow() {
    ImGui::Begin("解决方案调试", &this->OpenSolutionDebugWindow);
    // 检查当前是否操作解决方案
    if (this->CurrentSolution) {
        ImGui::Text(("当前操作解决方案名称：" + this->CurrentSolution->Name + "   元素数量：" + std::to_string(this->CurrentSolution->Size_Elements) + "   总时长：" + std::to_string(this->CurrentSolution->GetMsg().empty() ? 0.0f : this->CurrentSolution->TotalDurationTime)).data());
        if (ImGui::Button("打印解决方案信息到调试窗口")) {
            this->ISys().LogLine();
            this->ISys().LogInfo(this->CurrentSolution->GetMsg());
            this->ISys().LogLine();
        }

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

        if (ImGui::Button("清空所有元素")) {
            this->CurrentSolution->Clear();
            this->ISys().LogSucc("成功清空解决方案所有元素");
        }

        if (ImGui::Button("保存到XML文件")) {
            auto [ok, msg] = this->CurrentSolution->SaveToXML(this->ISys().PathManager()->PathGetFromKey("Solutions"));
            if (ok) {
                this->ISys().LogSucc(std::move(msg));
            }
            else {
                this->ISys().LogError(std::move(msg));
            }
        }

        if (ImGui::Button("删除当前解决方案")) {
            this->Solution_Delete(this->CurrentSolution);
            this->OpenSolutionKCPackDebugWindow = false;
            ImGui::End();
            return;
        }

        if (ImGui::Button("激活当前解决方案（并行，自定义偏移无效）")) {
            this->Playing_SetSolution(this->CurrentSolution, PlaybackMode::Parallel);
            this->Playing_Enable();
        }
        if (ImGui::Button("按组合模式生成时间偏移")) {
            this->CurrentSolution->TimeLineGenerate();
        }
        if (ImGui::Button("播放当前解决方案（串行，元素按解决方案指定偏移进行播放，元素本身时间头被忽略）")) {
            this->Playing_SetSolution(this->CurrentSolution, PlaybackMode::Serial);
            this->Playing_Enable();
        }

        if (ImGui::Button("修改按键绑定")) {
            this->Buffer_KCPack = this->CurrentSolution->KCPack;//缓存
            this->OpenSolutionKCPackDebugWindow = true;//打开窗口
        }

        ImGui::Separator();
        ImGui::Separator();
        ImGui::Separator();




        static int IndexForReset = 0;
        static int PreIndex = -1;
        ImGui::InputInt("要调整的本解决方案的元素", &IndexForReset);
        if (0 <= IndexForReset && IndexForReset < this->CurrentSolution->Size_Elements) {
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
            ImGui::Text("索引无效！请输入 0 到 %d 之间的值", this->CurrentSolution->Size_Elements - 1);
        }
        PreIndex = IndexForReset;
    }


    else {
        ImGui::Text("当前未选择任何解决方案");
    }

    if (ImGui::Button("关闭调试页面")) {
        this->OpenSolutionDebugWindow = false;
    }

    ImGui::End();
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

bool SolutionManager::Playing_SetSolution(Solution* const solution, const PlaybackMode Playmode) {
    //这里只需要设置，播放结束解决方案本身自动归位
    if (!solution) {
        this->ISys().LogError("找不到目标解决方案，可能是空指针");
        return false;
    }
    this->Playing_pSolution = solution;
    this->Playmode = Playmode;
    solution->Playmode = Playmode;
    switch (Playmode) {
    case PlaybackMode::Serial:
        this->Playing_SetTimeSchema(this->AL3D->GetTime());//偏移时间轴播放
        this->ISys().LogInfo("偏移时间轴播放，偏移时间设置为：" + std::to_string(this->AL3D->GetTime()));
        break;
    case PlaybackMode::Parallel:
        this->Playing_SetTimeSchema(0);
        break;
    }
    this->ISys().LogInfo("已经切换至解决方案：" + solution->Name);
    return true;
}
bool SolutionManager::Playing_SetSolution(const std::string& SolutionName, const PlaybackMode Playmode) {
    Solution* pSolution = this->Solution_Get(SolutionName);
    if (!pSolution) {
        return false;
    }
    return this->Playing_SetSolution(pSolution, Playmode);
}


//启动相关

void SolutionManager::Playing_Enable() {
    if (!this->Playing_pSolution) {
        this->ISys().LogError("无法开启播放：未设置要播放的解决方案！");
        return;
    }
    this->Playing = true;
    this->GlobalVars->CampathPlaying = true;
    this->ISys().LogInfo("已开启播放");
}
void SolutionManager::Playing_Disable() {
    this->Playing = false;
    this->GlobalVars->CampathPlaying = false;
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
    std::unique_ptr<CameraSystemIO>IO = std::make_unique<CameraSystemIO>();

    IO->SolutionTime = this->AL3D->GetTime();
    IO->FrameGameTime = this->AL3D->GetTime();
    IO->PlaybackMode = this->Playmode;
    IO->PlayBackRate = this->PlaybackRate;
    bool bPlaying = this->Playing;
    IO->isPlaying = &bPlaying;// 这里让解决方案拿到关闭播放控制权

    if (!this->Playing_pSolution->Call(IO.get())) {
        // 这里不关闭播放，因为解决方案可能还有内容
        // 不应该由管理器因为仅仅没有结果就关闭
        if (bPlaying == false) {
            // 如果解决方案自己关闭了播放
            this->Playing_Disable();
            return;
        }
        return;
    }
    if (this->Config.PlayingOverride) {
        this->AL3D->CameraSystemIOOverride(IO.get());
    }
    else if (this->Config.PlayingDraw) {
        this->CamDrawer->DrawFrameCamera(IO->Frame, "解决方案插值摄像机");
    }
}