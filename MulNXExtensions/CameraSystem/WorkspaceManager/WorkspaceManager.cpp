#include "WorkspaceManager.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CameraSystem/SolutionManager/SolutionManager.hpp>
#include <MulNXExtensions/CameraSystem/ProjectManager/ProjectManager.hpp>

#include <MulNXThirdParty/All_pugixml.hpp>

bool WorkspaceManager::MenuWorkspace(MulNX::UINode* node) {
    // 顶部：工作区信息（始终显示）
    ImGui::BeginChild("工作区面板", ImVec2(0, 150), true); {
        // 工作区状态
        ImGui::Text("工作区状态: %s", this->InWorkspace ? "已进入" : "未进入");
        // 未进入工作区允许打开默认工作区
        if (!this->InWorkspace) {
            ImGui::SameLine();
            if (ImGui::Button("打开默认工作区")) {
                this->Workspace_Set("DefaultWorkspace");
            }
        }
        // 打开工作区后允许保存工作区
        if (this->InWorkspace) {
            ImGui::Text("当前工作区: %s", this->CurrentWorkspace->Name.c_str());
            ImGui::SameLine();
            if (ImGui::Button("保存工作区")) {
                this->Workspace_Save();
            }
        }
        // 详情信息
        ImGui::Separator();
        ImGui::BeginChild("工作区详情菜单");
        if (ImGui::CollapsingHeader("详情信息")) {
            static std::string TargetWorkspaceName{};
            ImGui::Text("工作区名：");
            ImGui::SameLine();
            ImGui::InputText("##TargetWorkspaceName", &TargetWorkspaceName);
            ImGui::SameLine();
            if (ImGui::Button("切换")) {
                if (TargetWorkspaceName.empty()) {
                    this->ISys().LogError("请输入工作区名！");
                    return true;
                }
                if (!this->Workspace_Set(TargetWorkspaceName))return true;

            }

            if (!this->CurrentWorkspace) {// 无工作区
                ImGui::Text("当前未打开任何工作区");
                return true;
            }
            this->InWorkspace = true;
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    return true;
}

bool WorkspaceManager::Init() {
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");
    this->pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");

    this->SendUINode("MenuWorkspace", [this](MulNX::UINode* node) {return this->MenuWorkspace(node);});

    return true;
}

bool WorkspaceManager::Workspace_Save() {
    //生成配置文件
    if (!this->Workspace_ConfigGenerate()) {
        return false;
    }
    //保存配置文件
    if (!this->Workspace_ConfigSave()) {
        return false;
    }
    //保存当前活跃项目
    if (!this->PManager->Project_Save()) {
        return false;
    }
    this->ISys().LogSucc("工作区保存成功");
    return true;
}
bool WorkspaceManager::Workspace_Set(const std::string& Name) {
    this->InWorkspace = false;
    // 清空旧存储
    this->EManager->Element_ClearAll();
    this->SManager->Solution_ClearAll();
    this->PManager->Project_ClearAll();
    // 制作指针
    this->CurrentWorkspace = std::make_unique<Workspace>(Name);
    auto* PathManager = this->ISys().PathManager();

    this->ISys().PathManager()->KeySetCurrent("CurrentProject", {});
    this->ISys().PathManager()->KeySetCurrent("CurrentWorkspace", Name);
    // 获取配置信息
    if (this->Workspace_ConfigLoad(PathManager->PathGetFromKey("CurrentWorkspace"))) {
        this->Workspace_ConfigApply();
    }
    // 进入工作区
    this->InWorkspace = true;
    // 自动加载所有项目到内存中
    auto ProPath = PathManager->PathGetFromKey("CurrentWorkspace");
    std::vector<std::string> ProjectsNames = this->pIPCer->GetProjectsNames(ProPath);
    if (!ProjectsNames.empty()) {
        for (const auto& ProjectName : ProjectsNames) {
            std::filesystem::path ProjectPath = PathManager->PathGetFromKey("CurrentWorkspace") / ProjectName;
            this->PManager->Project_Load(ProjectPath, ProjectName);
        }
    }
    return true;
}




//config相关

bool WorkspaceManager::Workspace_ConfigGenerate() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    WorkspaceConfig& Config = this->CurrentWorkspace->Config;
    Config.ElementCfg = this->EManager->Config;
    Config.ProjectCfg = this->PManager->Config;
    Config.SolutionCfg = this->SManager->Config;
    return true;
}
bool WorkspaceManager::Workspace_ConfigSave() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    auto [ok, msg] = this->CurrentWorkspace->Save(this->ISys().PathManager()->PathGetFromKey("CurrentWorkspace"));
    if (!ok) {
        this->ISys().LogError(std::move(msg));
        return false;
    }
    this->ISys().LogSucc(std::move(msg));
    return true;
}
bool WorkspaceManager::Workspace_ConfigLoad(const std::filesystem::path& WorkspacePath) {
    if (!this->CurrentWorkspace) {
        return false;
    }
    // 检查文件路径和名称存在性
    if (WorkspacePath.empty()) {
        this->ISys().LogError("文件夹路径为空，无法从文件加载工作区配置！");
        return false;
    }
    // 拼接完整路径
    std::filesystem::path FullPath = WorkspacePath / ("WorkspaceConfig.yaml");
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogWarning("文件不存在！文件路径：" + FullPath.string());
        return false;
    }
    // 输出调试信息
    this->ISys().LogInfo("尝试从文件加载工作区配置，文件路径：" + FullPath.string());

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        WorkspaceConfig& Config = this->CurrentWorkspace->Config;

        auto config = root["config"];

        auto elements = config["elements"];
        Config.ElementCfg.PreviewDraw = elements["PreviewDraw"].as<bool>();
        Config.ElementCfg.PreviewOverride = elements["PreviewOverride"].as<bool>();

        auto solutions = config["solutions"];
        Config.SolutionCfg.SolutionShortcutEnable = solutions["shortcutEnable"].as<bool>();
        Config.SolutionCfg.PlayingDraw = solutions["PlayingDraw"].as<bool>();
        Config.SolutionCfg.PlayingOverride = solutions["PlayingOverride"].as<bool>();

        auto projects = config["projects"];
        Config.ProjectCfg.ProjectShortcutEnable = projects["ProjectShortcutEnable"].as<bool>();

        this->ISys().LogSucc("成功从文件加载工作区配置！ 文件路径：" + FullPath.string());
        return true;
    }
    catch (const YAML::Exception& e) {
        this->ISys().LogError("在加载工作区时发生异常：" + std::string(e.what()));
        return false;
    }
}
bool WorkspaceManager::Workspace_ConfigApply() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    WorkspaceConfig& Config = this->CurrentWorkspace->Config;
    this->EManager->Config = Config.ElementCfg;
    this->PManager->Config = Config.ProjectCfg;
    this->SManager->Config = Config.SolutionCfg;
    return true;
}