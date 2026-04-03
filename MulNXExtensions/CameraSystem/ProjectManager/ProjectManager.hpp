#pragma once

#include "ProjectConfig.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/Project/Project.hpp>

class ElementManager;
class SolutionManager;

//项目管理器，用于管理项目
class ProjectManager final :public MulNX::ModuleBase {
private:
    ElementManager* EManager = nullptr;
    SolutionManager* SManager = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    bool OpenProjectKCPackDebugWindow = false;
    bool OpenProjectNameDebugWindow = false;

    std::vector<std::shared_ptr<Project>> Projects{};
public:
    //当前操作项目指针（操作对象）
    std::shared_ptr<Project> ControllingProject = nullptr;
    //当前活跃对象
    std::shared_ptr<Project> ActiveProject = nullptr;

    ProjectConfig Config{};

    //构造函数
    ProjectManager();
    //析构函数
    ~ProjectManager();

    //项目管理器基本函数
    //初始化函数

    // 初始化
    bool Init()override;
    // UI
    bool UINodeFunc(MulNXUINode* node);
    // 逻辑主函数
    void VirtualMain()override;
    // 遍历
    void Traversal();



    //获取项目对应的迭代器
    std::vector<std::shared_ptr<Project>>::iterator Project_GetIterator(const std::string& Name);
    //获取项目指针
    std::shared_ptr<Project> Project_Get(const std::string& Name);
    //卸载项目（从内存中移除）
    bool Project_Delete(const std::string& Name);
    //清空项目（从内存中移除）
    bool Project_ClearAll();


    //创建项目
    bool Project_Create(const std::string& Name);
    //保存活跃项目
    bool Project_Save();
    //刷新活跃项目
    bool Project_Refresh();
    //保存所有项目
    //bool Project_SaveAll();
    //切换项目，重载解决方案和元素（删除先删解决方案再删元素，加载先加载元素再加载解决方案）
    bool Project_Apply(const std::shared_ptr<Project> Project);
    //从文件加载项目
    bool Project_Load(const std::filesystem::path& ProjectPath, const std::string& Name);



    //展示某项目信息
    void Project_ShowMsg(const std::string& Name);
    //展示所有项目信息
    void Project_ShowAll();
    //展示单个项目信息在一行上
    void Project_ShowInLine(std::shared_ptr<Project> Project);
    //一次展示所有项目信息到每一行
    void Project_ShowAllInLines();
private:
    //项目调试窗口及菜单
    void Project_DebugWindow();
    //快捷键修改缓存
    MulNX::KeyCheckPack* Buffer_KCPack = nullptr;
    //快捷键修改窗口
    void Project_KCPack_DebugWindow();
    //名称修改缓存
    std::string Buffer_Name{};
    //名称修改窗口
    void Project_Name_DebugWindow();
public:
    const std::vector<std::string>* Active_GetRoundStart();
    const std::vector<std::string>* Active_GetRoundEnd();

    //通过信息设置当前播放的解决方案（0：默认游戏时间轴播放，1：偏移时间轴播放）
    bool Playing_AutoCall(const MulNX::Message& Msg);
};