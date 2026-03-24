#include"ProjectManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CameraSystem/SolutionManager/SolutionManager.hpp>

//构造与析构函数

ProjectManager::ProjectManager() {
    this->Buffer_KCPack = new MulNX::KeyCheckPack();
}
ProjectManager::~ProjectManager() {
    delete this->Buffer_KCPack;
}

//项目管理器基本函数

bool ProjectManager::UINodeFunc(MulNXUINode* node) {
    if (this->ShowWindow.load(std::memory_order_acquire)) {
        //项目调试窗口
        this->Project_DebugWindow();
        if (this->OpenProjectKCPackDebugWindow) {
            //项目按键绑定调试窗口
            this->Project_KCPack_DebugWindow();
        }
        if (this->OpenProjectNameDebugWindow) {
            //项目名称调试窗口
            this->Project_Name_DebugWindow();
        }
    }
    return true;
}

bool ProjectManager::Init() {
    this->NeedUINode = true;
    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("CurrentProject", {},
        [this](MulNX::PathManager* PathManager)->bool {
            auto NewProjectPath = PathManager->PathGetFromKey("CurrentProject");
            // 检验文件夹是否已存在
            if (!std::filesystem::exists(NewProjectPath)) {
                this->ISys().LogInfo("指定的项目文件夹不存在，需创建新的项目文件夹！  路径：" + NewProjectPath.string());
                //创建文件夹
                try {
                    std::filesystem::create_directory(NewProjectPath);
                    //创建子文件夹
                    std::filesystem::create_directory(NewProjectPath / "Elements");
                    std::filesystem::create_directory(NewProjectPath / "Solutions");
                }
                catch (const std::filesystem::filesystem_error& e) {
                    this->ISys().LogError("创建项目文件夹失败，错误信息：" + std::string(e.what()));
                    return false;
                }
                this->ISys().LogSucc("成功创建项目文件夹，路径：" + NewProjectPath.string());
                return true;
            }
            this->ISys().LogSucc("成功设置项目路径为：" + NewProjectPath.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("CurrentProject", "CurrentWorkspace");
    }

    return true;
}
void ProjectManager::InjectDependence(ElementManager* EManager, SolutionManager* SManager) {
    //系统服务
    this->EManager = EManager;
    this->SManager = SManager;
}
void ProjectManager::VirtualMain() {
    this->Traversal();
    return;
}
void ProjectManager::Traversal() {
    if (!this->Config.ProjectShortcutEnable)return;
    for (const auto& Project : this->Projects) {
        if (this->KT->CheckWithPack(Project->KCPack)) {
            this->Project_Apply(Project);
        }
    }
}


std::vector<std::shared_ptr<Project>>::iterator ProjectManager::Project_GetIterator(const std::string& Name) {
    return std::find_if(this->Projects.begin(), this->Projects.end(),
        [&Name](const std::shared_ptr<Project>& project) {
            return project->Name == Name;
        });
}
std::shared_ptr<Project> ProjectManager::Project_Get(const std::string& Name) {
    auto it = this->Project_GetIterator(Name);
    if (it == this->Projects.end()) {
        return nullptr;
    }
    return (*it);
}
bool ProjectManager::Project_Delete(const std::string& Name) {
    std::vector<std::shared_ptr<Project>>::iterator it = this->Project_GetIterator(Name);
    if (it == this->Projects.end()) {
        return true;
    }
    this->Projects.erase(it);
    return true;
}
bool ProjectManager::Project_ClearAll() {
    this->Projects.clear();
    this->ActiveProject = nullptr;
    return true;
}



bool ProjectManager::Project_Create(const std::string& Name) {
    //检查是否已存在同名项目
    if (this->Project_Get(Name)) {
        this->ISys().LogError("项目名已占用！ 项目名：" + Name);
        return false;
    }
    //创建项目指针
    std::shared_ptr<Project> CreateProject = std::make_shared<Project>(Name);
    CreateProject->Refresh();
    //添加进项目组
    this->Projects.push_back(std::move(CreateProject));
    this->ISys().LogSucc("成功创建项目：" + Name);
    return true;
}
bool ProjectManager::Project_Refresh() {
    //先验证有没有活跃项目
    if (!this->ActiveProject) {
        return false;
    }
    ////获取元素名称列表
    //this->ActiveProject->ElementNames = std::move(this->EManager->Element_GetNames());
    ////获取解决方案名称列表
    //this->ActiveProject->SolutionNames = std::move(this->SManager->Solution_GetNames());
    this->ActiveProject->Refresh();
    //刷新完毕
    return true;
}
bool ProjectManager::Project_Save() {
    if (!this->Project_Refresh()) {
        return false;
    }
    //保存所有元素和解决方案到磁盘
    this->EManager->Element_SaveAll();
    this->SManager->Solution_SaveAll();
    //保存项目到磁盘
    std::filesystem::path Path = this->ISys().PathManager()->PathGetFromKey("CurrentWorkspace") / this->ActiveProject->Name;
    auto [ok, msg] = this->ActiveProject->Save(Path);
    if (ok) {
        this->ISys().LogSucc(std::move(msg));
        return true;
    }
    else {
        this->ISys().LogError(std::move(msg));
        return false;
    }
}
//bool ProjectManager::Project_SaveAll() {
//	if (this->Projects.empty()) {
//		this->ISys().LogWarning("尝试在没有任何项目的情况下保存");
//		return true;
//	}
//	
//	//遍历所有项目保存
//	for (const auto& Project : this->Projects) {
//		std::filesystem::path Path = this->Core->IPCer().PathGet_CurrentWorkspace() / Project->Name;
//		std::string Ruselt;
//		if (Project->Save(Path, Ruselt)) {
//			this->ISys().LogSucc(Ruselt);
//		}
//		else {
//			this->ISys().LogError(Ruselt);
//			return false;
//		}
//	}
//	this->ISys().LogSucc("成功保存所有项目到文件！");
//	return true;
//}
bool ProjectManager::Project_Apply(const std::shared_ptr<Project> Project) {
    //先尝试保存当前活跃项目
    this->Project_Save();
    //切换项目
    this->ActiveProject = Project;
    //清空旧解决方案，防止冲突
    this->SManager->Solution_ClearAll();
    //清空旧元素，防止冲突
    this->EManager->Element_ClearAll();
    if (!this->ISys().PathManager()->KeySetCurrent("CurrentProject", Project->Name)) {
        this->ISys().LogError("尝试切换到项目时出现问题，设置项目文件夹路径失败！");
        return false;
    }
    //获取元素文件夹路径
    std::filesystem::path ElementsPath = this->ISys().PathManager()->PathGetFromKey("Elements");
    std::vector<std::string>Elements = this->Core->IPCer().GetFileNamesByPath(ElementsPath);
    //遍历加载元素
    for (const std::string& Element : Elements) {
        this->EManager->Element_Load_Pre(ElementsPath / Element);
    }
    //获取解决方案文件夹路径
    std::filesystem::path SolutionsPath = this->ISys().PathManager()->PathGetFromKey("Solutions");
    std::vector<std::string>Solutions = this->Core->IPCer().GetFileNamesByPath(SolutionsPath);
    //遍历加载解决方案
    for (const std::string& Solution : Solutions) {
        this->SManager->Solution_Load(SolutionsPath / Solution);
    }
    this->ActiveProject = Project;
    this->ISys().LogSucc("已切换至项目" + Project->Name);
    this->ISys().LogSucc("尝试加载元素总数：" + std::to_string(Elements.size()));
    this->ISys().LogSucc("尝试加载解决方案总数：" + std::to_string(Solutions.size()));
    this->ISys().LogSucc("成功加载元素总数：" + std::to_string(this->EManager->Elements.size()));
    this->ISys().LogSucc("成功加载解决方案总数：" + std::to_string(this->SManager->Solutions.size()));
    return true;
}
bool ProjectManager::Project_Load(const std::filesystem::path& ProjectPath, const std::string& yamlName) {
    // 检查文件路径和名称存在性
    if (ProjectPath.empty() || yamlName.empty()) {
        this->ISys().LogError("文件夹路径或文件名为空，无法从文件加载项目！");
        return false;
    }

    // 拼接完整路径
    std::filesystem::path FullPath = ProjectPath / (yamlName + ".yaml");

    // 输出调试信息
    this->ISys().LogInfo("尝试从文件加载项目，文件路径：" + FullPath.string());

    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        std::string loadProjectName = root["name"].as<std::string>();
        //检查是否存在同名项目
        if (this->Project_Get(loadProjectName)) {
            this->ISys().LogError("项目名已占用，无法从文件加载项目！ 项目名：" + std::move(loadProjectName));
            return false;
        }
        auto loadProject = std::make_shared<Project>(loadProjectName);
        loadProject->KCPack = root["KCP"].as<MulNX::KeyCheckPack>();
        loadProject->OnNewRound = root["OnNewRound"].as<std::vector<std::string>>();

        loadProject->Refresh();
        this->ISys().LogSucc("成功从文件加载项目：" + loadProjectName);

        //添加进项目组
        this->Projects.push_back(std::move(loadProject));
        return true;
    }
    catch (const YAML::Exception& e) {
        this->ISys().LogError("在加载项目时出现问题：" + std::string(e.what()));
        return false;
    }
}



void ProjectManager::Project_ShowMsg(const std::string& Name) {
    std::shared_ptr<Project> Project = this->Project_Get(Name);
    if (!Project) {
        return;
    }
    //判断是否需要更新
    if (Project == this->ActiveProject) {
        //如果操作项目和活跃项目是同一个项目，则需要刷新
        this->Project_Refresh();
    }
    this->ISys().LogLine();
    this->ISys().LogInfo(this->ControllingProject->GetMsg());
    this->ISys().LogLine();
    return;
}
void ProjectManager::Project_ShowAll() {
    // 判断是否有项目存在
    if (!this->Projects.empty())return;
    // 先尝试更新活跃项目
    this->Project_Refresh();
    // 隔离线
    this->ISys().LogLine();
    this->ISys().LogLine();
    // 依次展示所有项目信息
    for (const auto& Project : this->Projects) {
        this->ISys().LogInfo(Project->GetMsg());
        this->ISys().LogLine();
    }
    // 隔离线
    this->ISys().LogLine();
    this->ISys().LogLine();

    return;
}
void ProjectManager::Project_ShowInLine(std::shared_ptr<Project> Project) {
    if (!Project) {
        this->ISys().LogError("项目指针为空，无法展示信息！");
        return;
    }
    ImGui::Text("|项目名称：");
    ImGui::SameLine();
    if (ImGui::Button(Project->Name.c_str())) {
        this->ControllingProject = Project;
        this->ShowWindow.store(true, std::memory_order_release);
    }

    return;
}
void ProjectManager::Project_ShowAllInLines() {
    //使用迭代器遍历所有项目
    for (const auto& Project : this->Projects) {
        this->Project_ShowInLine(Project);
    }
}

void ProjectManager::Project_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow("项目调试", this->ShowWindow);
    if (!w)return;
    // 检查当前操作项目
    if (this->ControllingProject) {
        ImGui::Text("当前操作项目：");
        ImGui::SameLine();
        ImGui::Text(this->ControllingProject->Name.c_str());
        if (ImGui::Button("切换到当前项目")) {
            this->Project_Apply(this->ControllingProject);
        }
        if (ImGui::Button("打印项目信息到调试窗口")) {
            this->Project_ShowMsg(this->ControllingProject->Name);
        }
        if (ImGui::Button("卸载当前项目")) {
            this->Project_Delete(this->ControllingProject->Name);
            this->ShowWindow.store(false, std::memory_order_release);
            return;
        }
        if (ImGui::Button("修改按键绑定")) {
            *this->Buffer_KCPack = this->ControllingProject->KCPack;//缓存
            this->OpenProjectKCPackDebugWindow = true;//打开窗口
        }
        if (ImGui::Button("修改项目名称")) {
            this->Buffer_Name = this->ControllingProject->Name;//缓存
            this->OpenProjectNameDebugWindow = true;//打开窗口
        }
        ImGui::Separator();
        ImGui::Separator();
        if (ImGui::TreeNode("进入新回合")) {
            if (!this->ControllingProject->OnNewRound.empty()) {
                for (const std::string SolutionName : this->ControllingProject->OnNewRound) {
                    ImGui::Text(SolutionName.c_str());
                    ImGui::SameLine();
                    if (ImGui::Button((std::string("删除##OnNewRound") + SolutionName).c_str())) {
                        auto it = std::find(this->ControllingProject->OnNewRound.begin(), this->ControllingProject->OnNewRound.end(), SolutionName);
                        if (it != this->ControllingProject->OnNewRound.end()) {
                            this->ControllingProject->OnNewRound.erase(it);
                        }
                    }
                }
            }
            else {
                ImGui::Text("空");
            }



            ImGui::TreePop();
        }
        if (ImGui::TreeNode("回合结束")) {
            if (!this->ControllingProject->OnRoundEnd.empty()) {
                for (const std::string SolutionName : this->ControllingProject->OnRoundEnd) {
                    ImGui::Text(SolutionName.c_str());
                }
            }
            else {
                ImGui::Text("空");
            }


            ImGui::TreePop();
        }

    }
    else {
        ImGui::Text("当前未选择任何项目");
    }
    if (ImGui::Button("关闭项目调试页面")) {
        this->ShowWindow.store(false, std::memory_order_release);
    }
}
void ProjectManager::Project_KCPack_DebugWindow() {
    ImGui::Begin("按键绑定##Project", &this->OpenProjectKCPackDebugWindow);

    if (this->ControllingProject) {
        ImGui::Text(("当前绑键：" + this->ControllingProject->KCPack.GetMsg()).c_str());
        ImGui::Separator();

        // 修饰键复选框
        ImGui::Checkbox("Ctrl", &this->Buffer_KCPack->Ctrl);
        ImGui::SameLine();
        ImGui::Checkbox("Shift", &this->Buffer_KCPack->Shift);
        ImGui::SameLine();
        ImGui::Checkbox("Alt", &this->Buffer_KCPack->Alt);

        // 按键选择下拉菜单
        static const char* keyItems[] = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"
        };

        // 查找当前键码对应的索引
        int currentKeyIndex = 0;
        for (int i = 0; i < IM_ARRAYSIZE(keyItems); i++) {
            if (i < 26) { // A-Z
                if (this->Buffer_KCPack->vkCode == 0x41 + i) {
                    currentKeyIndex = i;
                    break;
                }
            }
            else { // F1-F12
                if (this->Buffer_KCPack->vkCode == 0x70 + (i - 26)) {
                    currentKeyIndex = i;
                    break;
                }
            }
        }

        ImGui::Text("按键:");
        ImGui::SameLine();
        if (ImGui::Combo("##ProjectKey", &currentKeyIndex, keyItems, IM_ARRAYSIZE(keyItems))) {
            // 更新键码
            if (currentKeyIndex < 26) {
                this->Buffer_KCPack->vkCode = 0x41 + currentKeyIndex; // A-Z
            }
            else {
                this->Buffer_KCPack->vkCode = 0x70 + (currentKeyIndex - 26); // F1-F12
            }
        }

        // 连击数输入
        int comboClick = static_cast<int>(this->Buffer_KCPack->ComboClick);
        ImGui::Text("连击数:");
        ImGui::SameLine();
        if (ImGui::InputInt("##ProjectComboClick", &comboClick, 1, 5)) {
            // 限制在 1-255 范围内
            comboClick = std::clamp(comboClick, 1, 255);
            this->Buffer_KCPack->ComboClick = static_cast<unsigned char>(comboClick);
        }

        if (ImGui::Button("修改项目绑键")) {
            this->ControllingProject->KCPack = *this->Buffer_KCPack;
            this->ControllingProject->KCPack.Refresh();
            this->OpenProjectKCPackDebugWindow = false;
            ImGui::End();
            return;
        }
    }
    else {
        ImGui::Text("当前没有要调试的项目，无法修改按键绑定");
    }

    ImGui::Separator();

    if (ImGui::Button("关闭项目按键绑定调试页面")) {
        this->OpenProjectKCPackDebugWindow = false;
        ImGui::End();
        return;
    }

    ImGui::End();
}
void ProjectManager::Project_Name_DebugWindow() {
    ImGui::Begin("项目重命名", &this->OpenProjectNameDebugWindow);

    if (this->ControllingProject) {
        ImGui::Text(("当前名称：" + this->ControllingProject->Name).c_str());
        ImGui::Separator();

        ImGui::Text("新项目名:");
        ImGui::SameLine();
        ImGui::InputText("##NewName", &this->Buffer_Name);

        //检查是否已经存在该名称
        if (this->Project_Get(this->Buffer_Name)) {
            ImGui::Text("该名称已经被占用");
        }
        else {//没有被使用才允许修改
            if (ImGui::Button("修改名称")) {
                this->ControllingProject->ResetName(this->Buffer_Name);
                this->OpenProjectNameDebugWindow = false;
                ImGui::End();
                return;
            }
        }
    }
    else {
        ImGui::Text("当前没有要调试的项目，无法修改项目名");
    }

    ImGui::Separator();

    if (ImGui::Button("关闭项目名调试页面")) {
        this->OpenProjectNameDebugWindow = false;
        ImGui::End();
        return;
    }

    ImGui::End();
}

//信息接口

const std::vector<std::string>* ProjectManager::Active_GetRoundStart() {
    if (!this->ActiveProject) {
        return nullptr;
    }
    return &this->ActiveProject->OnRoundStart;
}
const std::vector<std::string>* ProjectManager::Active_GetRoundEnd() {
    if (!this->ActiveProject) {
        return nullptr;
    }
    return &this->ActiveProject->OnRoundEnd;
}

bool ProjectManager::Playing_AutoCall(const MulNX::Message& Msg) {
    this->ISys().LogInfo("项目管理器正在处理消息！");
    if (!this->ActiveProject) {
        this->ISys().LogWarning("无活跃项目，无法执行自动操作");
        return false;
    }
    switch (Msg.type) {
    case "Game/NewRound"_hash: {
        const std::vector<std::string>& OnNewRound = this->ActiveProject->OnNewRound;
        if (OnNewRound.empty()) {
            this->ISys().LogWarning("无新回合解决方案可尝试调用");
            return false;
        }
        int temp = rand() % OnNewRound.size();
        return this->SManager->Playing_SetSolution(OnNewRound[temp], PlaybackMode::Orchestration);
    }
    case "Game/RoundEnd"_hash: {
        const std::vector<std::string>& OnEnd = this->ActiveProject->OnRoundEnd;
        if (OnEnd.empty()) {
            this->ISys().LogWarning("无回合结束解决方案可尝试调用");
            return false;
        }
        int temp = rand() % OnEnd.size();
        return this->SManager->Playing_SetSolution(OnEnd[temp], PlaybackMode::Orchestration);
    }
    }
    return false;
}