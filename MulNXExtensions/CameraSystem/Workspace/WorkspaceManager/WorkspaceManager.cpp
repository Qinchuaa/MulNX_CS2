#include "WorkspaceManager.hpp"

#include "../../Element/ElementManager/ElementManager.hpp"
#include "../../Solution/SolutionManager/SolutionManager.hpp"
#include "../../Project/ProjectManager/ProjectManager.hpp"

#include <MulNX/MulNX.hpp>

bool WorkspaceManager::Init() {
    return true;
}
void WorkspaceManager::InjectDependence(ElementManager* EManager, SolutionManager* SManager, ProjectManager* PManager) {
    this->EManager = EManager;
    this->SManager = SManager;
    this->PManager = PManager;
}
void WorkspaceManager::VirtualMain() {
    return;
}

bool WorkspaceManager::Workspace_Create(const std::string& Name) {
    std::string strResult{};
    if (this->Core->IPCer().PathCreate_Workspace(Name)) {
        this->ISys().LogSucc(strResult);
        return true;
    }
    else {
        this->ISys().LogError(strResult);
        return false;
    }
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
bool WorkspaceManager::Workspace_TrySetPath(const std::string& Name) {
    return this->Core->IPCer().PathSet_Workspace(Name);
}
bool WorkspaceManager::Workspace_Set(const std::string& Name) {
    //尝试设置路径
    if (!this->Workspace_TrySetPath(Name)) {
        return false;
    }
    this->InWorkspace = false;
    //清空旧存储
    this->EManager->Element_ClearAll();
    this->SManager->Solution_ClearAll();
    this->PManager->Project_ClearAll();
    //制作指针
    this->CurrentWorkspace = std::make_unique<Workspace>(Name);
    MulNX::IPCer& IPCer = this->Core->IPCer();
    //获取配置信息
    if (!this->Workspace_ConfigLoad(IPCer.PathGet_CurrentWorkspace())) {
        return false;
    }
    if (!this->Workspace_ConfigApply()) {
        return false;
    }
    //进入工作区
    this->InWorkspace = true;
    //自动加载所有项目到内存中
    std::vector<std::string> ProjectsNames = IPCer.GetProjectsNames();
    if (!ProjectsNames.empty()) {
        for (const auto& ProjectName : ProjectsNames) {
            std::filesystem::path ProjectPath = IPCer.PathGet_CurrentWorkspace() / ProjectName;
            this->PManager->Project_LoadFromXML(ProjectPath, ProjectName);
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
    std::string strResult{};
    if (this->CurrentWorkspace->SaveConfigToXML(this->Core->IPCer().PathGet_CurrentWorkspace(), strResult)) {
        this->ISys().LogSucc(strResult);
        return true;
    }
    else {
        this->ISys().LogError(strResult);
        return false;
    }
}
bool WorkspaceManager::Workspace_ConfigLoad(const std::filesystem::path& WorkspacePath) {
    if (!this->CurrentWorkspace) {
        return false;
    }
    //检查文件路径和名称存在性
    if (WorkspacePath.empty()) {
        this->ISys().LogError("文件夹路径为空，无法从XML文件加载工作区配置！");
        return false;
    }

    //拼接完整路径
    std::filesystem::path FullPath = WorkspacePath / ("WorkspaceConfig.xml");

    //输出调试信息
    this->ISys().LogInfo("尝试从XML文件加载工作区配置，文件路径：" + FullPath.string());

    //检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("XML文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    //创建临时XML文件
    pugi::xml_document LoadXML;

    //打开XML文件并检验结果
    pugi::xml_parse_result result = LoadXML.load_file(FullPath.c_str());
    if (!result) {
        this->ISys().LogError("尝试从XML文件加载工作区配置失败，无法加载XML文件！ 文件路径：" + FullPath.string() +
            "\n     错误描述：" + result.description());
        return false;
    }

    //开始获取信息
    //获取WorkspaceConfig节点
    pugi::xml_node node_WorkspaceConfig = LoadXML.child("WorkspaceConfig");
    if (!node_WorkspaceConfig) {
        this->ISys().LogError("尝试从XML文件加载工作区配置失败，XML文件格式错误，找不到根节点！ 文件路径：" + FullPath.string());
        return false;
    }
    WorkspaceConfig& Config = this->CurrentWorkspace->Config;
    try {
        //获取ElementConfig信息
        {
            pugi::xml_node node_ElementConfig = node_WorkspaceConfig.child("ElementConfig");
            pugi::xml_node node_PreviewDraw = node_ElementConfig.child("PreviewDraw");
            Config.ElementCfg.PreviewDraw = node_PreviewDraw.attribute("Enable").as_bool();
            pugi::xml_node node_PreviewOverride = node_ElementConfig.child("PreviewOverride");
            Config.ElementCfg.PreviewOverride = node_PreviewOverride.attribute("Enable").as_bool();
        }
        //获取SolutionConfig信息
        {
            pugi::xml_node node_SolutionConfig = node_WorkspaceConfig.child("SolutionConfig");

            pugi::xml_node node_SolutionShortcutEnable = node_SolutionConfig.child("SolutionShortcutEnable");
            Config.SolutionCfg.SolutionShortcutEnable = node_SolutionShortcutEnable.attribute("Enable").as_bool();
            pugi::xml_node node_PlayingDraw = node_SolutionConfig.child("PlayingDraw");
            Config.SolutionCfg.PlayingDraw = node_PlayingDraw.attribute("Enable").as_bool();
            pugi::xml_node node_PlayingOverride = node_SolutionConfig.child("PlayingOverride");
            Config.SolutionCfg.PlayingOverride = node_PlayingOverride.attribute("Enable").as_bool();
        }
        //获取ProjectConfig信息
        {
            pugi::xml_node node_ProjectConfig = node_WorkspaceConfig.child("ProjectConfig");

            pugi::xml_node node_ProjectShortcutEnable = node_ProjectConfig.child("ProjectShortcutEnable");
            Config.ProjectCfg.ProjectShortcutEnable = node_ProjectShortcutEnable.attribute("Enable").as_bool();

            pugi::xml_node node_ApplyShouldClear = node_ProjectConfig.child("ApplyShouldClear");
        }
    }
    catch (...) {
        this->ISys().LogError("尝试从XML文件加载工作区配置失败，XML文件格式错误，读取节点时发生异常！ 文件路径：" + FullPath.string());
        return false;
    }
    this->ISys().LogSucc("成功从XML文件加载工作区配置！ 文件路径：" + FullPath.string());
    return true;
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